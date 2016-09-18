/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/BuildInfo.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Image/ImageFormatRegistry.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Noise.h"
#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Physics/PhysicsIntersectResult.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"
#include "CarbonEngine/Scene/Camera.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Scene/Terrain.h"

namespace Carbon
{

const auto TerrainVersionInfo = VersionInfo(1, 0);

Terrain::~Terrain()
{
    onDestruct();
    clear();
}

void Terrain::clear()
{
    heightmapWidth_ = 0;
    heightmapHeight_ = 0;

    heightmap_.clear();
    normals_.clear();

    terrainScale_ = 1.0f;
    heightScale_ = 100.0f;
    textureScale_ = 0.01f;

    algorithm_ = BruteForce;

    geometryChunk_.clear();
    isHeightmapDirty_ = false;

    clipmapSize_ = 255;
    clipmapLevelCount_ = 6;

    for (auto& clipmap : clipmaps_)
    {
        textures().releaseTexture(clipmap.texture);
        textures().releaseTexture(clipmap.normalMap);
    }
    clipmaps_.clear();

    centerChunk_.clear();

    for (auto& trimChunk : trimChunks_)
        trimChunk.clear();

    Entity::clear();
}

bool Terrain::create(unsigned int heightmapWidth, unsigned int heightmapHeight)
{
    try
    {
        clear();

        if (heightmapWidth == 0 || heightmapHeight == 0)
            return false;

        heightmapWidth_ = heightmapWidth;
        heightmapHeight_ = heightmapHeight;

        try
        {
            heightmap_.resize(heightmapWidth_ * heightmapHeight_);
            normals_.resize(heightmapWidth_ * heightmapHeight_);
        }
        catch (const std::bad_alloc&)
        {
            throw Exception("Failed allocating memory for the heightmap and normals");
        }

        isHeightmapDirty_ = true;

        return true;
    }
    catch (const Exception& e)
    {
        clear();

        LOG_ERROR << e;

        return false;
    }
}

bool Terrain::createFromPerlinNoise(unsigned int heightmapWidth, unsigned int heightmapHeight, unsigned int octaves,
                                    float persistence, float zoom)
{
    if (!create(heightmapWidth, heightmapHeight))
        return false;

    auto f = heightmap_.getData();
    for (auto y = 0U; y < heightmapHeight_; y++)
    {
        for (auto x = 0U; x < heightmapWidth_; x++, f++)
            *f = Noise::perlin(x * zoom, y * zoom, octaves, persistence);
    }

    normalize();

    return true;
}

bool Terrain::createFromTexture(const String& name)
{
    try
    {
        // Try and load the requested image
        auto image = Image();
        if (!ImageFormatRegistry::loadImageFile(Texture::TextureDirectory + name, image))
            throw Exception("Failed loading image");

        // Create empty terrain with the dimensions of the image
        if (!create(image.getWidth(), image.getHeight()))
            throw Exception("Failed creating terrain");

        // Convert image to the Luminance8 pixel format
        if (!image.setPixelFormat(Image::Luminance8))
            throw Exception("Failed converting image data");

        // Copy texture data into heightmap
        auto data = image.getDataForFrame(0);
        auto heightmap = heightmap_.begin();
        for (auto i = heightmapHeight_ * heightmapWidth_; i > 0; i--)
            *heightmap++ = Math::byteToFloat(*data++);

        LOG_INFO << "Loaded terrain heightmap: '" << name << "', dimensions: " << image.getWidth() << "x"
                 << image.getHeight();

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name << "' - " << e;

        clear();

        return false;
    }
}

float Terrain::getHeight(unsigned int x, unsigned int y) const
{
    if (x >= heightmapWidth_ || y >= heightmapHeight_)
        return 0.0f;

    return heightmap_[y * heightmapWidth_ + x];
}

bool Terrain::setHeight(unsigned int x, unsigned int y, float height)
{
    if (x >= heightmapWidth_ || y >= heightmapHeight_)
        return false;

    heightmap_[y * heightmapWidth_ + x] = height;

    isHeightmapDirty_ = true;

    return true;
}

void Terrain::setTerrainScale(float terrainScale)
{
    if (terrainScale > 0.0f)
    {
        terrainScale_ = terrainScale;
        isHeightmapDirty_ = true;
    }
}

void Terrain::setHeightScale(float heightScale)
{
    heightScale_ = heightScale;
    isHeightmapDirty_ = true;
}

void Terrain::setTextureScale(float textureScale)
{
    textureScale_ = textureScale;
    isHeightmapDirty_ = true;
}

bool Terrain::setMaterial(const String& material)
{
    material_ = material;

    if (materials().getMaterial(material).getEffectName() == "InternalGeometryClipmapping")
    {
        if (effects().getEffectActiveShader("InternalGeometryClipmapping"))
            algorithm_ = GeometryClipmapping;
        else
        {
            LOG_ERROR << "Geometry clipmapping is not supported on this hardware";
            return false;
        }
    }
    else
        algorithm_ = BruteForce;

    return true;
}

void Terrain::normalize()
{
    if (heightmap_.empty())
        return;

    auto lowest = heightmap_[0];
    auto highest = heightmap_[0];

    for (auto height : heightmap_)
    {
        lowest = std::min(lowest, height);
        highest = std::max(highest, height);
    }

    auto scale = 1.0f / (highest - lowest);

    for (auto& height : heightmap_)
        height = (height - lowest) * scale;

    isHeightmapDirty_ = true;
}

void Terrain::accentuate(float exponent)
{
    for (auto& height : heightmap_)
        height = powf(height, exponent);

    isHeightmapDirty_ = true;
}

void Terrain::smooth(unsigned int repeats)
{
    if (heightmap_.empty())
        return;

    for (auto i = 0U; i < repeats; i++)
    {
        auto newData = heightmap_;

        for (auto y = 1U; y < heightmapHeight_ - 1; y++)
        {
            for (auto x = 1U; x < heightmapWidth_ - 1; x++)
            {
                newData[y * heightmapWidth_ + x] = (getHeight(x - 1, y - 1) + getHeight(x - 1, y + 1) +
                                                    getHeight(x + 1, y - 1) + getHeight(x + 1, y + 1)) /
                        16.0f +
                    (getHeight(x, y - 1) + getHeight(x, y + 1) + getHeight(x + 1, y) + getHeight(x - 1, y)) / 8.0f +
                    getHeight(x, y) / 4.0f;
            }
        }

        swap(heightmap_, newData);
    }

    isHeightmapDirty_ = true;
}

void Terrain::scale(float scale)
{
    for (auto& height : heightmap_)
        height *= scale;

    isHeightmapDirty_ = true;
}

void Terrain::exponentiate(float cutoff, float sharpness, float exponentScale)
{
    cutoff = Math::clamp01(cutoff);
    sharpness = 1.0f - Math::clamp01(sharpness);

    for (auto& height : heightmap_)
        height = 1.0f - powf(sharpness, Math::clamp01(height - cutoff) * exponentScale);

    isHeightmapDirty_ = true;
}

bool Terrain::getHeightmapImage(Image& image) const
{
    if (!image.initialize(heightmapWidth_, heightmapHeight_, 1, Image::Red32f, false, 1))
        return false;

    memcpy(image.getDataForFrame(0), heightmap_.getData(), heightmap_.getDataSize());

    return true;
}

void Terrain::save(FileWriter& file) const
{
    Entity::save(file);

    file.beginVersionedSection(TerrainVersionInfo);

    file.write(heightmapWidth_, heightmapHeight_, heightmap_);
    file.write(terrainScale_, heightScale_, textureScale_, material_);

    file.endVersionedSection();
}

void Terrain::load(FileReader& file)
{
    try
    {
        clear();

        Entity::load(file);

        file.beginVersionedSection(TerrainVersionInfo);

        // Read heightmap data
        file.read(heightmapWidth_, heightmapHeight_, heightmap_);
        if (heightmap_.size() != heightmapWidth_ * heightmapHeight_)
            throw Exception("Terrain heightmap data sizes do not match");

        normals_.resize(heightmapWidth_ * heightmapHeight_);

        auto material = String();
        file.read(terrainScale_, heightScale_, textureScale_, material);

        file.endVersionedSection();

        setMaterial(material);

        isHeightmapDirty_ = true;
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

Vec3 Terrain::getTerrainPoint(unsigned int x, unsigned int y) const
{
    return {float(x) * terrainScale_, getHeight(x, y) * heightScale_, float(y) * terrainScale_};
}

void Terrain::calculateNormals()
{
    for (auto y = 0U; y < heightmapHeight_; y++)
    {
        for (auto x = 0U; x < heightmapWidth_; x++)
        {
            auto& n = normals_[y * heightmapWidth_ + x];

            n = Vec3::Zero;

            if (y > 0)
            {
                if (x > 0)
                {
                    n += Plane::normalFromPoints(getTerrainPoint(x, y), getTerrainPoint(x, y - 1),
                                                 getTerrainPoint(x - 1, y - 1));
                    n += Plane::normalFromPoints(getTerrainPoint(x, y), getTerrainPoint(x - 1, y - 1),
                                                 getTerrainPoint(x - 1, y));
                }

                if (x < heightmapWidth_ - 1)
                {
                    n += Plane::normalFromPoints(getTerrainPoint(x, y), getTerrainPoint(x + 1, y),
                                                 getTerrainPoint(x + 1, y - 1));
                    n += Plane::normalFromPoints(getTerrainPoint(x, y), getTerrainPoint(x + 1, y - 1),
                                                 getTerrainPoint(x, y - 1));
                }
            }

            if (y < heightmapHeight_ - 1)
            {
                if (x > 0)
                {
                    n += Plane::normalFromPoints(getTerrainPoint(x, y), getTerrainPoint(x - 1, y),
                                                 getTerrainPoint(x - 1, y + 1));
                    n += Plane::normalFromPoints(getTerrainPoint(x, y), getTerrainPoint(x - 1, y + 1),
                                                 getTerrainPoint(x, y + 1));
                }

                if (x < heightmapWidth_ - 1)
                {
                    n += Plane::normalFromPoints(getTerrainPoint(x, y), getTerrainPoint(x, y + 1),
                                                 getTerrainPoint(x + 1, y + 1));
                    n += Plane::normalFromPoints(getTerrainPoint(x, y), getTerrainPoint(x + 1, y + 1),
                                                 getTerrainPoint(x + 1, y));
                }
            }

            n.normalize();

            n = Vec3::UnitY;
        }
    }
}

struct BruteForceVertex
{
    Vec3 position;
    Vec2 textureCoordinate;
    Vec3 tangent;
    Vec3 bitangent;
    Vec3 normal;
};

void Terrain::updateBruteForceGeometryChunk()
{
    if (!isHeightmapDirty_)
        return;

    calculateNormals();

    isHeightmapDirty_ = false;

    auto vertex = geometryChunk_.lockVertexData<BruteForceVertex>();

    for (auto y = 0U; y < heightmapHeight_; y++)
    {
        for (auto x = 0U; x < heightmapWidth_; x++)
        {
            vertex->position.x = float(x) * terrainScale_;
            vertex->position.y = getHeight(x, y) * heightScale_;
            vertex->position.z = float(y) * terrainScale_;

            vertex->textureCoordinate.x = vertex->position.x * textureScale_;
            vertex->textureCoordinate.y = vertex->position.z * textureScale_;

            vertex->tangent.setXYZ(1.0f, 0.0f, 0.0f);
            vertex->bitangent.setXYZ(0.0f, 1.0f, 0.0f);
            vertex->normal = normals_[y * heightmapWidth_ + x];

            vertex++;
        }
    }

    geometryChunk_.unlockVertexData();
}

bool Terrain::prepareForRendering()
{
    if (heightmap_.empty())
        return false;

    if (algorithm_ == BruteForce)
    {
        if (geometryChunk_.getVertexCount() == 0)
        {
            // Set the vertex streams on the geometry chunk
            geometryChunk_.addVertexStream({VertexStream::Position, 3});
            geometryChunk_.addVertexStream({VertexStream::DiffuseTextureCoordinate, 2});
            geometryChunk_.addVertexStream({VertexStream::Tangent, 3});
            geometryChunk_.addVertexStream({VertexStream::Bitangent, 3});
            geometryChunk_.addVertexStream({VertexStream::Normal, 3});

            geometryChunk_.setVertexCount(heightmapWidth_ * heightmapHeight_);

            // Setup a triangle strip drawitem for rendering the terrain
            auto indices = Vector<unsigned int>();
            for (auto y = 0U; y < heightmapHeight_ - 1; y++)
            {
                if (indices.size())
                {
                    indices.append(indices.back());
                    indices.append(y * heightmapWidth_);
                }

                for (auto x = 0U; x < heightmapWidth_; x++)
                {
                    indices.append(y * heightmapWidth_ + x);
                    indices.append((y + 1) * heightmapWidth_ + x);
                }
            }

            geometryChunk_.setupIndexData({{GraphicsInterface::TriangleStrip, indices.size(), 0}}, indices);
        }

        updateBruteForceGeometryChunk();
    }
    else if (algorithm_ == GeometryClipmapping)
    {
        try
        {
            if (geometryChunk_.getVertexCount())
                return true;

            if (!effects().getEffectActiveShader("InternalGeometryClipmapping"))
                throw Exception("Hardware does not support geometry clipmapping");

            // Allocate clipmap textures
            auto clipmapImage = Image();
            auto normalMapImage = Image();
            if (!clipmapImage.initialize(clipmapSize_, clipmapSize_, 1, Image::Red32f, false, 1) ||
                !normalMapImage.initialize(clipmapSize_, clipmapSize_, 1, Image::RGB8, false, 1))
                throw Exception("Failed initializing clipmap texture images");

            for (auto i = 0U; i < clipmapLevelCount_; i++)
            {
                auto texture = textures().create2DTexture();

                if (!texture->load(String() + ".TerrainClipmapHeight" + i, clipmapImage) || !texture->upload())
                {
                    textures().releaseTexture(texture);
                    throw Exception("Failed setting up clipmap texture");
                }

                auto normalMap = textures().create2DTexture();

                if (!normalMap->load(String() + ".TerrainClipmapNormal" + i, normalMapImage, "WorldLightmap") ||
                    !normalMap->upload())
                {
                    textures().releaseTexture(texture);
                    textures().releaseTexture(normalMap);
                    throw Exception("Failed setting up normal map texture");
                }

                clipmaps_.emplace(texture, normalMap);
            }
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;
            return false;
        }

        // Fill vertex buffer
        geometryChunk_.addVertexStream({VertexStream::Position, 3});
        geometryChunk_.setVertexCount(clipmapSize_ * clipmapSize_);
        auto data = geometryChunk_.lockVertexData<Vec3>();
        for (auto y = 0U; y < clipmapSize_; y++)
        {
            for (auto x = 0U; x < clipmapSize_; x++)
                (data++)->setXYZ(float(x), 0.0f, float(y));
        }
        geometryChunk_.unlockVertexData();

        // Fill index buffer
        auto m = (clipmapSize_ + 1) / 4;

        auto indices = Vector<unsigned int>();

        for (auto y = 0U; y < clipmapSize_ - 1; y++)
        {
            if (y < m - 1 || y > clipmapSize_ - m - 1)
            {
                // A full triangle strip

                if (!indices.empty())
                {
                    indices.append(indices.back());
                    indices.append(y * clipmapSize_);
                }
                for (auto x = 0U; x < clipmapSize_; x++)
                {
                    indices.append(y * clipmapSize_ + x);
                    indices.append((y + 1) * clipmapSize_ + x);
                }
            }
            else
            {
                // Two separate triangle strips, one each side of the 'hole'

                indices.append(indices.back());
                indices.append(y * clipmapSize_);
                for (auto x = 0U; x < m; x++)
                {
                    indices.append(y * clipmapSize_ + x);
                    indices.append((y + 1) * clipmapSize_ + x);
                }

                indices.append(indices.back());
                indices.append(y * clipmapSize_ + clipmapSize_ - m);
                for (auto x = clipmapSize_ - m; x < clipmapSize_; x++)
                {
                    indices.append(y * clipmapSize_ + x);
                    indices.append((y + 1) * clipmapSize_ + x);
                }
            }
        }
        geometryChunk_.setupIndexData(Vector<DrawItem>{{GraphicsInterface::TriangleStrip, indices.size(), 0}}, indices);

        // Trim chunks
        auto trimSize = clipmapSize_ - 2 * (m - 1);
        for (auto i = 0U; i < 4; i++)
        {
            auto& chunk = trimChunks_[i];

            chunk.addVertexStream({VertexStream::Position, 3});
            chunk.setVertexCount(trimSize * trimSize);
            data = chunk.lockVertexData<Vec3>();
            for (auto y = 0U; y < trimSize; y++)
            {
                for (auto x = 0U; x < trimSize; x++)
                    (data++)->setXYZ(float(x + m - 1), -1.0f, float(y + m - 1));
            }
            chunk.unlockVertexData();

            auto drawItems = Vector<DrawItem>{{GraphicsInterface::TriangleStrip, trimSize * 2, 0}};
            indices.clear();

            if (i == 0)
            {
                // Left
                drawItems.emplace(GraphicsInterface::TriangleList, (trimSize - 1) * 6, 0);
                for (auto y = 0U; y < trimSize - 1; y++)
                {
                    indices.append(y * trimSize);
                    indices.append((y + 1) * trimSize);
                    indices.append(y * trimSize + 1);

                    indices.append(y * trimSize + 1);
                    indices.append((y + 1) * trimSize);
                    indices.append((y + 1) * trimSize + 1);
                }
            }
            else if (i == 1)
            {
                // Right
                drawItems.emplace(GraphicsInterface::TriangleList, (trimSize - 1) * 6, 0);
                for (auto y = 0U; y < trimSize - 1; y++)
                {
                    indices.append(y * trimSize + trimSize - 2);
                    indices.append((y + 1) * trimSize + trimSize - 2);
                    indices.append(y * trimSize + trimSize - 1);

                    indices.append(y * trimSize + trimSize - 1);
                    indices.append((y + 1) * trimSize + trimSize - 2);
                    indices.append((y + 1) * trimSize + trimSize - 1);
                }
            }
            else if (i == 2)
            {
                // Bottom
                drawItems.emplace(GraphicsInterface::TriangleStrip, trimSize * 2, 0);
                for (auto x = 0U; x < trimSize; x++)
                {
                    indices.append(x);
                    indices.append(trimSize + x);
                }
            }
            else if (i == 3)
            {
                // Top
                drawItems.emplace(GraphicsInterface::TriangleStrip, trimSize * 2, 0);
                for (auto x = 0U; x < trimSize; x++)
                {
                    indices.append((trimSize - 2) * trimSize + x);
                    indices.append((trimSize - 1) * trimSize + x);
                }
            }

            chunk.setupIndexData(drawItems, indices);
            chunk.optimizeVertexData();
            chunk.registerWithRenderer();
        }

        // Center chunk
        centerChunk_.addVertexStream({VertexStream::Position, 3});
        centerChunk_.setVertexCount(trimSize * trimSize);
        auto centerData = centerChunk_.lockVertexData<Vec3>();
        for (auto y = 0U; y < trimSize; y++)
        {
            for (auto x = 0U; x < trimSize; x++)
                (centerData++)->setXYZ(float(x + m - 1), 0.0f, float(y + m - 1));
        }
        centerChunk_.unlockVertexData();

        indices.clear();
        for (auto y = 0U; y < trimSize - 1; y++)
        {
            // A full triangle strip

            if (indices.size())
            {
                indices.append(indices.back());
                indices.append(y * trimSize);
            }
            for (auto x = 0U; x < trimSize; x++)
            {
                indices.append(y * trimSize + x);
                indices.append((y + 1) * trimSize + x);
            }
        }

        centerChunk_.setupIndexData(Vector<DrawItem>{{GraphicsInterface::TriangleStrip, indices.size(), 0}}, indices);
        centerChunk_.registerWithRenderer();
    }

    geometryChunk_.registerWithRenderer();

    return true;
}

void Terrain::intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry)
{
    Entity::intersectRay(ray, intersections, onlyWorldGeometry);

    if (!isVisible() || !isPhysical())
        return;

    LOG_DEBUG << "Terrain ray intersection is not implemented";
}

bool Terrain::gatherGeometry(GeometryGather& gather)
{
    if (!Entity::gatherGeometry(gather))
        return false;

    if (shouldProcessGather(gather))
    {
        if (!prepareForRendering())
            return true;

        gather.changePriority(getRenderPriority());

        if (algorithm_ == BruteForce)
        {
            gather.changeTransformation(getWorldTransform());

            auto material = getMaterialRoot() + material_;
            auto overrideParameters = getMaterialOverrideParameters(material);

            gather.changeMaterial(material, overrideParameters);

            updateBruteForceGeometryChunk();
            gather.addGeometryChunk(geometryChunk_);
        }
        else if (algorithm_ == GeometryClipmapping)
        {
            if (isHeightmapDirty_)
                calculateNormals();

            auto halfClipmapSize = int(clipmapSize_ / 2);

            auto clipmapCameraPosition = worldToLocal(gather.getCameraPosition()) / terrainScale_;

            for (auto i = 0U; i < clipmaps_.size(); i++)
            {
                // Get the central sample position of the camera in this clipmap. The '| 1' ensures that clipmaps always
                // align to an odd boundary and so will match up correctly with the next coarsest clipmap
                auto center = Vec2i(int(floorf(clipmapCameraPosition.x)) | 1, int(floorf(clipmapCameraPosition.z)) | 1);

                // Update clipmap if the center has changed
                if (clipmaps_[i].center != center)
                    updateClipmap(i, center);

                // Calculate the scale to use when rendering this clipmap level
                auto clipmapScale = (1U << i) * terrainScale_;

                // Calculate the local space position of the lower left corner of the clipmap
                auto clipmapOrigin =
                    Vec3(float(center.x - halfClipmapSize), 0.0f, float(center.y - halfClipmapSize)) * clipmapScale;

                // Set up parameters to pass through the the geometry clipmapping shader
                auto material = getMaterialRoot() + material_;
                auto overrideParameters = getMaterialOverrideParameters(material);
                overrideParameters[Parameter::clipmapSize].setInteger(clipmapSize_);
                overrideParameters[Parameter::scales].setFloat4(clipmapScale, heightScale_, textureScale_,
                                                                terrainScale_);
                overrideParameters[Parameter::clipmapOrigin].setVec3(clipmapOrigin);
                overrideParameters[Parameter::clipmapCameraPosition].setVec2(clipmapCameraPosition.getXZ());
                overrideParameters[Parameter::heightfieldTexture].setPointer<Texture>(clipmaps_[i].texture);
                overrideParameters[Parameter::normalMap].setPointer<Texture>(clipmaps_[i].normalMap);

                // Add this clipmap level to the queue
                gather.changeTransformation(localToWorld(clipmapOrigin), getWorldOrientation());
                gather.changeMaterial(material, overrideParameters);
                gather.addGeometryChunk(geometryChunk_);

                if (i == 0)
                {
                    // Add the center chunk when rendering the finest level
                    gather.addGeometryChunk(centerChunk_);
                }
                else
                {
                    // Add the approriate trims. Note that xCenter and yCenter are always odd numbers, which means the
                    // positive modulus is either 1 or 3, depending on where the finer layer is sitting inside this
                    // layer

                    // Left/right trim
                    if (Math::positiveModulus(clipmaps_[i - 1].center.x, 4) == 1)
                        gather.addGeometryChunk(trimChunks_[1]);    // Right trim
                    else
                        gather.addGeometryChunk(trimChunks_[0]);    // Left trim

                    // Top/bottom trim
                    if (Math::positiveModulus(clipmaps_[i - 1].center.y, 4) == 1)
                        gather.addGeometryChunk(trimChunks_[3]);    // Top trim
                    else
                        gather.addGeometryChunk(trimChunks_[2]);    // Bottom trim
                }

                clipmapCameraPosition *= 0.5f;
            }

            isHeightmapDirty_ = false;
        }
    }

    return true;
}

void Terrain::precache()
{
    prepareForRendering();

    Entity::precache();
}

PhysicsInterface::BodyObject Terrain::createInternalRigidBody(float mass, bool fixed)
{
    if (heightmap_.empty())
    {
        LOG_ERROR << "This terrain can't be made physical because it has no heightmap";
        return nullptr;
    }

    // Create new heightmap rigid body
    return physics().createHeightmapBodyFromTemplate(
        physics().createBodyTemplateFromHeightmap(heightmapWidth_, heightmapHeight_, heightmap_, true), heightScale_,
        terrainScale_, 0.0f, true, this, getWorldTransform());
}

void Terrain::updateClipmap(unsigned int clipmapIndex, const Vec2i& center)
{
    auto& clipmap = clipmaps_[clipmapIndex];

    auto image = clipmap.texture->lockImageData();
    auto normalMapImage = clipmap.normalMap->lockImageData();

    auto clipmapTexels = reinterpret_cast<float*>(image->getDataForFrame(0));
    auto normalMapTexels = normalMapImage->getDataForFrame(0);

    const auto heightmapStep = 1 << clipmapIndex;
    const auto halfClipmapSize = int(clipmapSize_ / 2);

    // Copy in the height data for this clipmap level
    auto currentClipmapTexel = clipmapTexels;
    auto currentNormalMapTexel = normalMapTexels;
    for (auto y = 0; y < int(clipmapSize_); y++)
    {
        auto yOffset = int((((center.y + y - halfClipmapSize) * heightmapStep) % heightmapHeight_) * heightmapWidth_);

        for (auto x = 0; x < int(clipmapSize_); x++)
        {
            auto xOffset = int(((center.x + x - halfClipmapSize) * heightmapStep) % heightmapWidth_);

            auto heightmapOffset = yOffset + xOffset;

            // Set height
            *currentClipmapTexel++ = heightmap_[heightmapOffset];

            // Set normal
            normals_[heightmapOffset].toNormalizedRGB8(currentNormalMapTexel);
            currentNormalMapTexel += 3;
        }
    }

    // Update textures
    clipmap.texture->unlockImageData();
    clipmap.normalMap->unlockImageData();

    clipmap.center = center;
}

}
