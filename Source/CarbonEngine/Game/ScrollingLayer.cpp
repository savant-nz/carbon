/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Game/ScrollingLayer.h"
#include "CarbonEngine/Image/ImageFormatRegistry.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/SimpleTimer.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Scene/Camera.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

const UnicodeString ScrollingLayer::ScrollingLayerExtension = ".scrollinglayer";
bool ScrollingLayer::VisualizeCollisionMapEdges = false;

const auto ScrollingLayerVersionInfo = VersionInfo(1, 0);

ScrollingLayer::~ScrollingLayer()
{
    onDestruct();
    clear();
}

// Structure used to specify scrolling layer vertex data in createVisibleTiles()
struct ScrollingLayerVertex
{
    Vec3 p;
    Vec2 st;
    Vec3 n;

    void set(float x, float y, float s, float t)
    {
        p.setXYZ(x, y, 1.0f);
        st.setXY(s, t);
        n.setXYZ(0.0f, 0.0f, 1.0f);
    }
};

void ScrollingLayer::createVisibleTiles()
{
    clearVisibleTiles();

    visibleTilesX_ = uint(ceilf(lastOrthographicSize_.length() / getTileWidth()) + 1.0f);
    visibleTilesY_ = uint(ceilf(lastOrthographicSize_.length() / getTileHeight()) + 1.0f);

    // Vertex streams
    geometryChunk_.addVertexStream({VertexStream::Position, 3});
    geometryChunk_.addVertexStream({VertexStream::DiffuseTextureCoordinate, 2});
    geometryChunk_.addVertexStream({VertexStream::Normal, 3});

    geometryChunk_.setVertexCount(visibleTilesX_ * visibleTilesY_ * 4 * 4);

    auto v = geometryChunk_.lockVertexData<ScrollingLayerVertex>();

    auto indices = Vector<unsigned int>();
    auto drawItems = Vector<DrawItem>();

    // Create the geometry for all the tiles, each tile is put into the chunk four times to cover all possible
    // combinations of vertical and horizontal flips
    for (auto y = 0U; y < visibleTilesY_; y++)
    {
        for (auto x = 0U; x < visibleTilesX_; x++)
        {
            // No vertical or horizontal flip
            v++->set(x * getTileWidth(), y * getTileHeight(), 0.0f, 0.0f);
            v++->set((x + 1) * getTileWidth(), y * getTileHeight(), 1.0f, 0.0f);
            v++->set(x * getTileWidth(), (y + 1) * getTileHeight(), 0.0f, 1.0f);
            v++->set((x + 1) * getTileWidth(), (y + 1) * getTileHeight(), 1.0f, 1.0f);

            // Only a horizontal flip
            v++->set(x * getTileWidth(), y * getTileHeight(), 1.0f, 0.0f);
            v++->set((x + 1) * getTileWidth(), y * getTileHeight(), 0.0f, 0.0f);
            v++->set(x * getTileWidth(), (y + 1) * getTileHeight(), 1.0f, 1.0f);
            v++->set((x + 1) * getTileWidth(), (y + 1) * getTileHeight(), 0.0f, 1.0f);

            // Only a vertical flip
            v++->set(x * getTileWidth(), y * getTileHeight(), 0.0f, 1.0f);
            v++->set((x + 1) * getTileWidth(), y * getTileHeight(), 1.0f, 1.0f);
            v++->set(x * getTileWidth(), (y + 1) * getTileHeight(), 0.0f, 0.0f);
            v++->set((x + 1) * getTileWidth(), (y + 1) * getTileHeight(), 1.0f, 0.0f);

            // Both a vertical and a horizontal flip
            v++->set(x * getTileWidth(), y * getTileHeight(), 1.0f, 1.0f);
            v++->set((x + 1) * getTileWidth(), y * getTileHeight(), 0.0f, 1.0f);
            v++->set(x * getTileWidth(), (y + 1) * getTileHeight(), 1.0f, 0.0f);
            v++->set((x + 1) * getTileWidth(), (y + 1) * getTileHeight(), 0.0f, 0.0f);

            // Create drawitems and indices for the above four quads
            for (auto i = 0U; i < 4; i++)
            {
                drawItems.emplace(GraphicsInterface::TriangleStrip, 4, indices.size());

                indices.append(indices.size());
                indices.append(indices.size());
                indices.append(indices.size());
                indices.append(indices.size());
            }
        }
    }

    geometryChunk_.unlockVertexData();

    geometryChunk_.setupIndexData(drawItems, indices);

    if (!geometryChunk_.calculateTangentBases())
        LOG_ERROR << "Failed calculating tangent bases";

    geometryChunk_.registerWithRenderer();
}

void ScrollingLayer::clearVisibleTiles()
{
    geometryChunk_.clear();
    visibleTilesX_ = 0;
    visibleTilesY_ = 0;
}

void ScrollingLayer::clear()
{
    clearLayerDetails();

    ComplexEntity::clear();

    setLocalSpaceChildCullingEnabled(true);
}

void ScrollingLayer::clearLayerDetails()
{
    clearVisibleTiles();

    tileCountX_ = 0;
    tileCountY_ = 0;
    tileAspectRatio_ = 1.0f;
    tileScale_ = 100.0f;
    speedScale_ = 1.0f;
    repeatX_ = false;
    repeatY_ = false;

    tiles_.clear();
    setDefaultTileTexture(String::Empty);

    layerDiffuseColor_ = Color::White;

    repeatingEntities_.clear();

    for (auto material : materials_)
        materials().unloadMaterial(material);
    materials_.clear();
}

void ScrollingLayer::setGridSize(unsigned int width, unsigned int height)
{
    auto newTiles = Vector<Tile>(width * height);

    for (auto x = 0U; x < tileCountX_; x++)
    {
        for (auto y = 0U; y < tileCountY_; y++)
        {
            if (x < width && y < height)
                newTiles[y * width + x] = tiles_[y * tileCountX_ + x];
        }
    }

    tileCountX_ = width;
    tileCountY_ = height;
    tiles_ = std::move(newTiles);

    clearVisibleTiles();
    unloadUnusedMaterials();
}

bool ScrollingLayer::isValidTileIndex(unsigned int x, unsigned int y, const String& function) const
{
    if (x < tileCountX_ && y < tileCountY_)
        return true;

    CARBON_LOG(Error, function) << "Tile index [" << x << ", " << y << "] is out of bounds, layer size is "
                                << tileCountX_ << "x" << tileCountY_;

    return false;
}

void ScrollingLayer::setTileAspectRatio(float tileAspectRatio)
{
    tileAspectRatio_ = tileAspectRatio;
    clearVisibleTiles();
}

void ScrollingLayer::setTileScale(float tileScale)
{
    tileScale_ = tileScale;
    clearVisibleTiles();
}

const String& ScrollingLayer::getTileTexture(unsigned int x, unsigned int y) const
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return String::Empty;

    auto& tile = tiles_[y * tileCountX_ + x];

    return tile.texture.length() ? tile.texture : defaultTileTexture_;
}

void ScrollingLayer::setTileTexture(unsigned int x, unsigned int y, const String& texture)
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return;

    auto& tile = tiles_[y * tileCountX_ + x];

    if (tile.texture == texture)
        return;

    tile.texture = texture;
    tile.material = nullptr;

    // Refresh the normal map
    setTileNormalMap(x, y, tile.normalMap);
}

void ScrollingLayer::setAllTileTextures(const String& texture)
{
    for (auto y = 0U; y < tileCountY_; y++)
    {
        for (auto x = 0U; x < tileCountX_; x++)
            setTileTexture(x, y, texture);
    }
}

void ScrollingLayer::setDefaultTileTexture(const String& texture)
{
    if (defaultTileTexture_ == texture)
        return;

    defaultTileTexture_ = texture;

    // Get the default tile normal map to use
    defaultTileNormalMap_ = A(ImageFormatRegistry::stripSupportedExtension(defaultTileTexture_) + "_normal");
    if (!Texture::doesTextureFileExist(defaultTileNormalMap_))
        defaultTileNormalMap_ = renderer().getFlatNormalMap()->getName();

    for (auto& tile : tiles_)
    {
        if (!tile.texture.length())
            tile.material = nullptr;
    }
}

bool ScrollingLayer::isTileVisible(unsigned int x, unsigned int y) const
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return false;

    auto& tile = tiles_[y * tileCountX_ + x];

    return tile.isVisible;
}

void ScrollingLayer::setTileVisible(unsigned int x, unsigned int y, bool visible)
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return;

    auto& tile = tiles_[y * tileCountX_ + x];

    tile.isVisible = visible;
}

bool ScrollingLayer::isTileFlippedVertically(unsigned int x, unsigned int y) const
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return false;

    auto& tile = tiles_[y * tileCountX_ + x];

    return tile.isFlippedVertically;
}

void ScrollingLayer::setTileFlippedVertically(unsigned int x, unsigned int y, bool isFlipped)
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return;

    auto& tile = tiles_[y * tileCountX_ + x];

    tile.isFlippedVertically = isFlipped;
}

bool ScrollingLayer::isTileFlippedHorizontally(unsigned int x, unsigned int y) const
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return false;

    auto& tile = tiles_[y * tileCountX_ + x];

    return tile.isFlippedHorizontally;
}

void ScrollingLayer::setTileFlippedHorizontally(unsigned int x, unsigned int y, bool isFlipped)
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return;

    auto& tile = tiles_[y * tileCountX_ + x];

    tile.isFlippedHorizontally = isFlipped;
}

String ScrollingLayer::getTileCollisionMap(unsigned int x, unsigned int y) const
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return String::Empty;

    auto& tile = tiles_[y * tileCountX_ + x];

    if (tile.collisionMap.length())
        return tile.collisionMap;

    return A(ImageFormatRegistry::stripSupportedExtension(getTileTexture(x, y))) + "_collision";
}

void ScrollingLayer::setTileCollisionMap(unsigned int x, unsigned int y, const String& collisionMap)
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return;

    auto& tile = tiles_[y * tileCountX_ + x];

    tile.collisionMap = collisionMap;
}

void ScrollingLayer::setTileNormalMap(unsigned int x, unsigned int y, const String& normalMap)
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return;

    auto& tile = tiles_[y * tileCountX_ + x];

    tile.normalMap = normalMap;
    tile.normalMapToUse = normalMap;
    tile.material = nullptr;

    // The cached normal map is needed to avoid using normal maps that don't exist which would result in missing texture
    // errors. The tile's default normal map is taken from the tile's texture with "_normal" appended, but is only used
    // if such a texture actually exists, if it doesn't exist then a flat normal map provided by the renderer will be
    // used instead so that no error occurs.
    if (!normalMap.length())
    {
        tile.normalMapToUse = A(ImageFormatRegistry::stripSupportedExtension(getTileTexture(x, y))) + "_normal";

        // This map is used to cache whether or not the default "_normal" normal map exists
        static auto normalMapExists = std::unordered_map<String, bool>();

        if (normalMapExists.find(tile.normalMapToUse) == normalMapExists.end())
            normalMapExists[tile.normalMapToUse] = Texture::doesTextureFileExist(tile.normalMapToUse);

        if (!normalMapExists[tile.normalMapToUse])
            tile.normalMapToUse = renderer().getFlatNormalMap()->getName();
    }
}

const String& ScrollingLayer::getTileNormalMap(unsigned int x, unsigned int y) const
{
    if (!isValidTileIndex(x, y, CARBON_CURRENT_FUNCTION))
        return String::Empty;

    auto& tile = tiles_[y * tileCountX_ + x];

    if (!tile.normalMapToUse.length())
        tile.normalMapToUse = renderer().getFlatNormalMap()->getName();

    return tile.normalMapToUse;
}

void ScrollingLayer::clampCameraPositionToLayerBounds(Camera* camera, bool negX, bool posX, bool negY, bool posY) const
{
    auto orthographicSize = camera->getOrthographicSize();

    auto clampWidth = std::max(0.0f, (float(tileCountX_) * getTileWidth() - orthographicSize.x) / speedScale_);
    auto clampHeight = std::max(0.0f, (float(tileCountY_) * getTileHeight() - orthographicSize.y) / speedScale_);

    auto& p = camera->getWorldPosition();

    camera->setWorldPosition(Math::clamp(p.x, negX ? 0.0f : p.x, posX ? clampWidth : p.x),
                             Math::clamp(p.y, negY ? 0.0f : p.y, posY ? clampHeight : p.y));
}

bool ScrollingLayer::gatherGeometry(GeometryGather& gather)
{
    if (!ComplexEntity::gatherGeometry(gather))
        return false;

    if (shouldProcessGather(gather))
    {
        auto camera = getScene()->getDefaultCamera();

        if (!camera)
        {
            LOG_WARNING << "Scrolling layers require that their scene have a camera";
            return true;
        }

        if (camera->getOrthographicSize() != lastOrthographicSize_ || !geometryChunk_.getVertexCount())
        {
            lastOrthographicSize_ = camera->getOrthographicSize();
            createVisibleTiles();
        }

        auto cameraExtents = camera->getWorldSpaceOrthographicExtents();

        if (tileCountX_ && tileCountY_)
        {
            auto tileSize = getTileSize();

            auto xOffset = int(floorf(speedScale_ * (cameraExtents.getLeft() / tileSize.x)));
            auto yOffset = int(floorf(speedScale_ * (cameraExtents.getBottom() / tileSize.y)));

            if (speedScale_ != 1.0f)
                setLocalPosition(cameraExtents.getMinimum() * (1.0f - speedScale_));

            auto tilesToRender = std::unordered_map<Material*, Vector<unsigned int>>();

            for (auto x = 0U; x < visibleTilesX_; x++)
            {
                for (auto y = 0U; y < visibleTilesY_; y++)
                {
                    auto mx = int(x) + xOffset;
                    auto my = int(y) + yOffset;

                    if (((my >= 0 && my < int(tileCountY_)) || repeatY_) &&
                        ((mx >= 0 && mx < int(tileCountX_)) || repeatX_))
                    {
                        auto& tile = tiles_[Math::positiveModulus(my, tileCountY_) * tileCountX_ +
                                            Math::positiveModulus(mx, tileCountX_)];

                        if (!tile.isVisible)
                            continue;

                        if (!tile.material)
                            tile.material = getMaterialForTile(tile);

                        tilesToRender[tile.material].append((visibleTilesX_ * y + x) * 4 +
                                                            (tile.isFlippedVertically ? 2 : 0) +
                                                            (tile.isFlippedHorizontally ? 1 : 0));
                    }
                }
            }

            // Add all the tiles to the render queue
            gather.changePriority(getRenderPriority());
            gather.changeTransformation(localToWorld(Vec3(xOffset * tileSize.x, yOffset * tileSize.y)));

            for (const auto& tileToRender : tilesToRender)
            {
                gather.newMaterial(tileToRender.first);

                for (auto drawItemIndex : tileToRender.second)
                    gather.addGeometryChunk(geometryChunk_, drawItemIndex);
            }
        }

        // Update the position of repeating entities
        auto layerSize = getLayerSize();
        for (const auto& repeatingEntity : repeatingEntities_)
        {
            // Get unadjusted AABB of the repeating entity
            auto aabb = repeatingEntity.entity->getLocalAABB() + (repeatingEntity.realPosition + getLocalPosition());

            auto movement = Vec2();

            // Calculate shifts in X and Y needed to bring the repeating sprite into line
            if (repeatX_)
            {
                if (aabb.getMaximum().x < cameraExtents.getLeft())
                    movement.x += ceilf((cameraExtents.getLeft() - aabb.getMaximum().x) / layerSize.x);
                else if (aabb.getMinimum().x > cameraExtents.getLeft() + lastOrthographicSize_.x)
                {
                    movement.x -= ceilf((aabb.getMinimum().x - (cameraExtents.getLeft() + lastOrthographicSize_.x)) /
                                        layerSize.x);
                }
            }
            if (repeatY_)
            {
                if (aabb.getMaximum().y < cameraExtents.getBottom())
                    movement.y += ceilf((cameraExtents.getBottom() - aabb.getMaximum().y) / layerSize.y);
                else if (aabb.getMinimum().y > cameraExtents.getBottom() + lastOrthographicSize_.y)
                {
                    movement.y -= ceilf((aabb.getMinimum().y - (cameraExtents.getBottom() + lastOrthographicSize_.y)) /
                                        layerSize.y);
                }
            }

            // Set new position for the repeating entity
            repeatingEntity.entity->setWorldPosition(localToWorld(repeatingEntity.realPosition + movement * layerSize));
        }
    }

    return true;
}

Material* ScrollingLayer::getMaterialForTile(const Tile& tile)
{
    const auto& texture = tile.texture.length() ? tile.texture : defaultTileTexture_;
    const auto& normalMap = tile.normalMapToUse.length() ? tile.normalMapToUse : defaultTileNormalMap_;

    static const auto diffuseMapParameter = ParameterArray::Lookup("diffuseMap");
    static const auto normalMapParameter = ParameterArray::Lookup("normalMap");

    // Find the material to use for this tile
    for (auto material : materials_)
    {
        if (material->getParameter(diffuseMapParameter).getString() == texture &&
            material->getParameter(normalMapParameter).getString() == normalMap)
            return material;
    }

    // Take this opportunity to do a clear out of any unused tile materials
    unloadUnusedMaterials();

    // Create a new material if none of the existing ones are suitable
    auto material = materials().createMaterial(Math::createGUID());

    material->setEffect("InternalSprite");
    material->setParameter(diffuseMapParameter, texture);
    material->setParameter(normalMapParameter, normalMap);
    material->setParameter(Parameter::diffuseColor, layerDiffuseColor_);
    material->setParameter(Parameter::depthWrite, false);
    material->setParameter(Parameter::blend, true);
    material->setParameter(Parameter::scaleAndOffset, 1.0f, 1.0f, 0.0f, 0.0f);
    material->setParameter(Parameter::isLightingAllowed, true);

    materials_.append(material);

    return material;
}

void ScrollingLayer::unloadUnusedMaterials()
{
    auto unusedMaterials = materials_;

    for (auto& tile : tiles_)
        unusedMaterials.eraseValue(tile.material);

    for (auto unusedMaterial : unusedMaterials)
    {
        materials().unloadMaterial(unusedMaterial);
        materials_.eraseValue(unusedMaterial);
    }
}

void ScrollingLayer::precache()
{
    for (auto& tile : tiles_)
    {
        if (!tile.material)
            tile.material = getMaterialForTile(tile);
    }

    for (auto material : materials_)
        material->precache();

    ComplexEntity::precache();
}

void ScrollingLayer::setLayerDiffuseColor(const Color& color)
{
    layerDiffuseColor_ = color;

    for (auto material : materials_)
        material->setParameter("diffuseColor", layerDiffuseColor_);
}

PhysicsInterface::BodyObject ScrollingLayer::createInternalRigidBody(float mass, bool fixed)
{
    if (!fixed)
    {
        LOG_ERROR << "Scrolling layers can only be made into fixed rigid bodies";
        return nullptr;
    }

    if (speedScale_ != 1.0f)
    {
        LOG_ERROR << "Scrolling layers can only be made physical if their speed scale is 1";
        return nullptr;
    }

    auto tileSize = getTileSize();

    auto finalPolygons = Vector<Vector<Vec2>>();

    auto cachedCollisionGeometry = std::unordered_map<String, Vector<Vector<Vec2>>>();

    for (auto x = 0U; x < tileCountX_; x++)
    {
        for (auto y = 0U; y < tileCountY_; y++)
        {
            auto collisionMap = getTileCollisionMap(x, y);

            // Process this collision map if it hasn't been seen before
            if (cachedCollisionGeometry.find(collisionMap) == cachedCollisionGeometry.end())
            {
                // Try and load the collision map
                auto image = Image();
                if (ImageFormatRegistry::loadImageFile(Texture::TextureDirectory + collisionMap, image))
                {
                    physics().convertImageAlphaTo2DPolygons(image, cachedCollisionGeometry[collisionMap]);

                    // Scale the 2D polygons for the tile size of this scrolling layer
                    for (auto& cachedGeometry : cachedCollisionGeometry[collisionMap])
                    {
                        for (auto& v : cachedGeometry)
                            v *= tileSize;
                    }

                    LOG_INFO << "Collision map loaded - '" << collisionMap;
                }
            }

            // Take the polygons for this collision map, offset and scale them for this tile, and add it to the list of
            // polygons being created

            auto xFlip = isTileFlippedHorizontally(x, y);
            auto yFlip = isTileFlippedVertically(x, y);

            // The scale and offset on the polygons handles any horizontal and vertical flips on this tile
            auto scale = Vec2(xFlip ? -1.0f : 1.0f, yFlip ? -1.0f : 1.0f);
            auto offset = Vec2(float(x + (xFlip ? 1 : 0)) * tileSize.x, float(y + (yFlip ? 1 : 0)) * tileSize.y);

            // Polygon winding is reversed if there is a single flip on this tile, two flips cancel each other out
            auto reverseWinding = xFlip ^ yFlip;

            // Add final polygons
            for (const auto& polygon : cachedCollisionGeometry[collisionMap])
            {
                finalPolygons.append(polygon);

                for (auto& finalPolygon : finalPolygons.back())
                {
                    finalPolygon *= scale;
                    finalPolygon += offset;
                }

                if (reverseWinding)
                    finalPolygons.back().reverse();
            }
        }
    }

    // Merge edges where possible to make larger polygons. This is mainly to get rid of seams and ridges on adjacent
    // collision maps that are more or less flush with each other, it doesn't do the more complicated merges that
    // potentially could be done here.
    const auto mergeThreshold = 2.0f;
    for (auto i = 0U; i < finalPolygons.size(); i++)
    {
        for (auto j = 0U; j < finalPolygons[i].size(); j++)
        {
            auto e0 = finalPolygons[i][j];
            auto e1 = finalPolygons[i][(j + 1) % finalPolygons[i].size()];

            for (auto k = i + 1; k < finalPolygons.size(); k++)
            {
                auto l = 0U;
                for (; l < finalPolygons[k].size(); l++)
                {
                    auto e2 = finalPolygons[k][l];
                    auto e3 = finalPolygons[k][(l + 1) % finalPolygons[k].size()];

                    // Check if the edges e0-e1 and e2-e3 are able to be merged
                    if (e0.distance(e3) < mergeThreshold && e1.distance(e2) < mergeThreshold)
                        break;
                }

                // If there was a merge then the polygon at index k becomes part of the polygon at index i
                if (l != finalPolygons[k].size())
                {
                    for (auto m = 0U; m < finalPolygons[k].size() - 2; m++)
                    {
                        finalPolygons[i].insert((j + 1 + m) % finalPolygons[i].size(),
                                                finalPolygons[k][(l + m + 2) % finalPolygons[k].size()]);
                    }

                    finalPolygons.erase(k--);
                    j--;

                    break;
                }
            }
        }
    }

    // Visualize the collision map edges using immediate geometry, this is useful when debugging
    if (VisualizeCollisionMapEdges)
    {
        for (auto& finalPolygon : finalPolygons)
        {
            for (auto j = 0U; j < finalPolygon.size(); j++)
            {
                getScene()->addImmediateGeometry(finalPolygon[j], finalPolygon[(j + 1) % finalPolygon.size()],
                                                 Color::Red, Color::Green);
            }
        }
    }

    // Convert 2D polygons to an actual triangle mesh that can be used as a collision hull
    auto vertices = Vector<Vec3>();
    auto triangles = Vector<RawIndexedTriangle>();
    physics().convert2DPolygonsToCollisionGeometry(finalPolygons, vertices, triangles);

    LOG_INFO << "Layer '" << getName() << "' is physical, polygon count: " << finalPolygons.size();

    // Create final physics body
    return physics().createGeometryBodyFromTemplate(
        physics().createBodyTemplateFromGeometry(vertices, triangles, true, 0.5f), mass, fixed, this);
}

bool ScrollingLayer::addRepeatingEntity(Entity* entity)
{
    if (!entity->getScene())
    {
        if (!addChild(entity))
            return false;
    }

    repeatingEntities_.emplace(entity);
    setRepeatingEntityPosition(entity, entity->getWorldPosition().toVec2());

    return true;
}

bool ScrollingLayer::setRepeatingEntityPosition(Entity* entity, const Vec2& position)
{
    for (auto& repeatingEntity : repeatingEntities_)
    {
        if (repeatingEntity.entity == entity)
        {
            repeatingEntity.realPosition = position;
            return true;
        }
    }

    return false;
}

bool ScrollingLayer::removeRepeatingEntity(Entity* entity)
{
    return repeatingEntities_.eraseIf([&](const RepeatingEntity& e) { return e.entity == entity; }) != 0;
}

void ScrollingLayer::save(FileWriter& file) const
{
    ComplexEntity::save(file);

    file.beginVersionedSection(ScrollingLayerVersionInfo);

    file.write(tileCountX_, tileCountY_, tileScale_, tileAspectRatio_, speedScale_);
    file.write(repeatX_, repeatY_, layerDiffuseColor_, defaultTileTexture_);

    for (auto y = 0U; y < tileCountY_; y++)
    {
        for (auto x = 0U; x < tileCountX_; x++)
        {
            auto& tile = tiles_[y * tileCountX_ + x];

            file.write(tile.texture, tile.normalMap, tile.collisionMap, tile.isFlippedHorizontally,
                       tile.isFlippedVertically);
        }
    }

    file.endVersionedSection();
}

void ScrollingLayer::load(FileReader& file)
{
    try
    {
        clear();

        ComplexEntity::load(file);

        file.beginVersionedSection(ScrollingLayerVersionInfo);

        auto defaultTileTexture = String();

        // Read scrolling layer data
        auto tileCountX = 0U;
        auto tileCountY = 0U;
        file.read(tileCountX, tileCountY, tileScale_, tileAspectRatio_, speedScale_);
        file.read(repeatX_, repeatY_, layerDiffuseColor_, defaultTileTexture);

        setDefaultTileTexture(defaultTileTexture);

        setGridSize(tileCountX, tileCountY);

        for (auto y = 0U; y < tileCountY_; y++)
        {
            for (auto x = 0U; x < tileCountX_; x++)
            {
                auto& tile = tiles_[y * tileCountX_ + x];

                auto texture = String();
                auto normalMap = String();
                file.read(texture, normalMap, tile.collisionMap, tile.isFlippedHorizontally, tile.isFlippedVertically);

                // Make sure texture references are taken appropriately on load
                setTileTexture(x, y, texture);
                setTileNormalMap(x, y, normalMap);
            }
        }

        file.endVersionedSection();
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

bool ScrollingLayer::save(const String& name) const
{
    try
    {
        auto file = FileWriter();
        fileSystem().open(UnicodeString(name) + ScrollingLayerExtension, file, true);

        file.writeText(UnicodeString() + "GridSize            " + tileCountX_ + " " + tileCountY_);
        file.writeText(UnicodeString() + "TileScale           " + tileScale_);
        file.writeText(UnicodeString() + "TileAspectRatio     " + tileAspectRatio_);
        file.writeText(UnicodeString() + "SpeedScale          " + speedScale_);
        file.writeText(UnicodeString() + "RepeatX             " + repeatX_);
        file.writeText(UnicodeString() + "RepeatY             " + repeatY_);
        file.writeText(UnicodeString() + "LayerDiffuseColor   " + layerDiffuseColor_);

        if (defaultTileTexture_.length())
            file.writeText("DefaultTileTexture  " + defaultTileTexture_.quoteIfHasSpaces());

        file.writeText(String::Empty);

        for (auto y = 0U; y < tileCountY_; y++)
        {
            for (auto x = 0U; x < tileCountX_; x++)
            {
                const auto& tileTexture = getTileTexture(x, y);

                if (tileTexture.length() && tileTexture != defaultTileTexture_)
                {
                    file.writeText(UnicodeString() + "TileTexture         " + x + " " + y + " " +
                                   tileTexture.quoteIfHasSpaces() +
                                   (isTileFlippedHorizontally(x, y) ? " FlipHorizontal" : "") +
                                   (isTileFlippedVertically(x, y) ? " FlipVertical" : ""));
                }

                if (!tiles_[y * tileCountX_ + x].isVisible)
                    file.writeText(UnicodeString() + "TileVisible         " + x + " " + y + " False");

                const auto& normalMap = tiles_[y * tileCountX_ + x].normalMap;
                if (normalMap.length())
                    file.writeText(UnicodeString() + "TileNormalMap       " + x + " " + y + " " +
                                   normalMap.quoteIfHasSpaces());

                const auto& collisionMap = tiles_[y * tileCountX_ + x].collisionMap;
                if (collisionMap.length())
                {
                    file.writeText(UnicodeString() + "TileCollisionMap    " + x + " " + y + " " +
                                   collisionMap.quoteIfHasSpaces());
                }
            }
        }

        for (auto parameter : getParameters())
        {
            file.writeText(UnicodeString() + "Parameter " + parameter.getName().quoteIfHasSpaces() + " " +
                           getParameter(parameter.getName()).getString().quoteIfHasSpaces());
        }

        LOG_INFO << "Saved scrolling layer - '" << name << "'";

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name << "' - " << e;

        return false;
    }
}

bool ScrollingLayer::load(const String& name)
{
    try
    {
        auto timer = SimpleTimer();

        clearLayerDetails();

        // Open the file
        auto lineTokens = Vector<Vector<String>>();
        if (!fileSystem().readTextFile(UnicodeString(name) + ScrollingLayerExtension, lineTokens))
            throw Exception("Failed opening file");

        setName(name);

        for (auto& line : lineTokens)
        {
            if (line[0].asLower() == "gridsize")
            {
                if (line.size() != 3 || !line[1].isInteger() || !line[2].isInteger())
                    throw Exception("Invalid grid size definition");

                setGridSize(line[1].asInteger(), line[2].asInteger());
            }
            else if (line[0].asLower() == "defaulttiletexture")
            {
                if (line.size() != 2)
                    throw Exception("Invalid default texture");

                setDefaultTileTexture(line[1]);
            }
            else if (line[0].asLower() == "tiletexture")
            {
                if (line.size() < 4 || !line[1].isInteger() || !line[2].isInteger())
                    throw Exception("Invalid tile texture");

                auto x = line[1].asInteger();
                auto y = line[2].asInteger();

                setTileTexture(x, y, line[3]);

                for (auto j = 4U; j < line.size(); j++)
                {
                    if (line[j].asLower() == "fliphorizontal")
                        setTileFlippedHorizontally(x, y, true);
                    else if (line[j].asLower() == "flipvertical")
                        setTileFlippedVertically(x, y, true);
                }
            }
            else if (line[0].asLower() == "tilevisible")
            {
                if (line.size() < 4 || !line[1].isInteger() || !line[2].isInteger() || !line[3].isBoolean())
                    throw Exception("Invalid tile visible");

                setTileCollisionMap(line[1].asInteger(), line[2].asInteger(), line[3].asBoolean());
            }
            else if (line[0].asLower() == "tilenormalmap")
            {
                if (line.size() < 4 || !line[1].isInteger() || !line[2].isInteger())
                    throw Exception("Invalid tile normal map");

                setTileNormalMap(line[1].asInteger(), line[2].asInteger(), line[3]);
            }
            else if (line[0].asLower() == "tilecollisionmap")
            {
                if (line.size() < 4 || !line[1].isInteger() || !line[2].isInteger())
                    throw Exception("Invalid tile collision map");

                setTileCollisionMap(line[1].asInteger(), line[2].asInteger(), line[3]);
            }
            else if (line[0].asLower() == "tileaspectratio")
            {
                if (line.size() != 2 || !line[1].isFloat())
                    throw Exception("Invalid tile aspect ratio");

                setTileAspectRatio(line[1].asFloat());
            }
            else if (line[0].asLower() == "tilescale")
            {
                if (line.size() != 2 || !line[1].isFloat())
                    throw Exception("Invalid tile scale");

                setTileScale(line[1].asFloat());
            }
            else if (line[0].asLower() == "speedscale")
            {
                if (line.size() != 2 || !line[1].isFloat())
                    throw Exception("Invalid speed scale");

                setSpeedScale(line[1].asFloat());
            }
            else if (line[0].asLower() == "repeatx")
            {
                if (line.size() != 2 || !line[1].isBoolean())
                    throw Exception("Invalid repeat x");

                setRepeatX(line[1].asBoolean());
            }
            else if (line[0].asLower() == "repeaty")
            {
                if (line.size() != 2 || !line[1].isBoolean())
                    throw Exception("Invalid repeat y");

                setRepeatY(line[1].asBoolean());
            }
            else if (line[0].asLower() == "layerdiffusecolor")
            {
                if (line.size() != 4 && line.size() != 5)
                    throw Exception("Invalid layer diffuse color");

                setLayerDiffuseColor(Color(line[1].asFloat(), line[2].asFloat(), line[3].asFloat(),
                                           line.size() == 5 ? line[4].asFloat() : 1.0f));
            }
            else if (line[0].asLower() == "parameter")
            {
                if (line.size() != 3)
                    throw Exception("Invalid parameter definition, expeced format is <name> <value>");

                setParameter(line[1], line[2]);
            }
            else
                LOG_WARNING << "Unrecognized command '" << line[0] << "' in scrolling layer '" << name << "'";
        }

        LOG_INFO << "Loaded scrolling layer - '" << getName() << "', time: " << timer;

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name << "' - " << e.get();

        clearLayerDetails();

        return false;
    }
}

}
