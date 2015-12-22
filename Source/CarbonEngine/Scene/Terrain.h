/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Vec2i.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Scene/Entity.h"

namespace Carbon
{

/**
 * The terrain entity creates and manages data for a terrain.
 */
class CARBON_API Terrain : public Entity
{
public:

    Terrain() { clear(); }
    ~Terrain() override;

    /**
     * Creates a flat terrain with the given heightmap size. Returns success flag.
     */
    bool create(unsigned int heightmapWidth, unsigned int heightmapHeight);

    /**
     * Creates this terrain from perlin noise created using the given values. Returns success flag.
     */
    bool createFromPerlinNoise(unsigned int heightmapWidth, unsigned int heightmapHeight, unsigned int octaves = 5,
                               float persistence = 0.3f, float zoom = 0.01f);

    /**
     * Creates this terrain from a texture image. The name of the texture should be given as it would be specified in a material
     * file, i.e. without an extension. The given texture must be 2D. Returns success flag.
     */
    bool createFromTexture(const String& name);

    /**
     * Returns the width of the internal heightmap.
     */
    unsigned int getHeightmapWidth() const { return heightmapWidth_; }

    /**
     * Returns the height height of the internal heightmap.
     */
    unsigned int getHeightmapHeight() const { return heightmapHeight_; }

    /**
     * Returns the height value at the given position in the internal heightmap. Returns zero if the given position is out of
     * range.
     */
    float getHeight(unsigned int x, unsigned int y) const;

    /**
     * Sets the height value at the given position in the internal heightmap. Returns false if the given position is out of
     * range.
     */
    bool setHeight(unsigned int x, unsigned int y, float height);

    /**
     * Returns the scale factor used on the terrain in the X and Z dimensions. Defaults to 1.0f.
     */
    float getTerrainScale() const { return terrainScale_; }

    /**
     * Sets the scale factor used on the terrain in the X and Z dimensions.
     */
    void setTerrainScale(float terrainScale);

    /**
     * Returns the current height scale value. Defaults to 100.0f.
     */
    float getHeightScale() const { return heightScale_; }

    /**
     * Sets the current height scale value.
     */
    void setHeightScale(float heightScale);

    /**
     * Returns the current texture scale value, this determines how much the texture coordinate increases for each world unit of
     * geometry. Defaults to 0.01f.
     */
    float getTextureScale() const { return textureScale_; }

    /**
     * Sets the current texture scale value.
     */
    void setTextureScale(float textureScale);

    /**
     * Returns the material being used to render the terrain.
     */
    const String& getMaterial() const { return material_; }

    /**
     * Sets the material to use when rendering the terrain. If the material uses the InternalGeometryClipmapping effect and the
     * hardware supports that rendering algorithm then it will be used when rendering the terrain. If the hardware doesn't
     * support it then false will be returned. Materials than use any other effects will result in brute force terrain rendering
     * being used. Returns success flag.
     */
    bool setMaterial(const String& material);

    /**
     * Raises each heightmap value to the given exponent. This can be used to accentuate peaks and troughs in the heightmap
     * data.
     */
    void accentuate(float exponent);

    /**
     * Applys a simple weighted 3x3 smoothing filter to the heightmap data. \a repeats specifies the number of times to run the
     * smooth operation.
     */
    void smooth(unsigned int repeats = 1);

    /**
     * Scales all the height values by a given factor.
     */
    void scale(float scale);

    /**
     * Replaces every height value as follows: height = 1.0 - sharpness ^ ((height - cutoff) * exponentScale). Heights below the
     * cutoff will be zeroed.
     */
    void exponentiate(float cutoff, float sharpness, float exponentScale);

    /**
     * Fills the passed \a image with the heightmap data in the Red32f pixel format. Returns success flag.
     */
    bool getHeightmapImage(Image& image) const;

    void clear() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;
    bool gatherGeometry(GeometryGather& gather) override;
    void intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry) override;
    void precache() override;

protected:

    PhysicsInterface::BodyObject createInternalRigidBody(float mass, bool fixed) override;

private:

    unsigned int heightmapWidth_ = 0;
    unsigned int heightmapHeight_ = 0;
    Vector<float> heightmap_;
    Vector<Vec3> normals_;

    // Normalizes all heights into the 0.0 - 1.0 range
    void normalize();

    // The heightmap dirty flag is set whenever the primary heightmap data is altered or any of the scaling constants are
    // changed. Once the changes have been dealt with the flag is reset.
    bool isHeightmapDirty_ = false;

    Vec3 getTerrainPoint(unsigned int x, unsigned int y) const;
    void calculateNormals();

    // Scales
    float terrainScale_ = 0.0f;
    float heightScale_ = 0.0f;
    float textureScale_ = 0.0f;

    String material_;

    // Terrain rendering algorithm being used
    enum TerrainAlgorithm
    {
        BruteForce,
        GeometryClipmapping
    } algorithm_ = BruteForce;

    GeometryChunk geometryChunk_;

    void updateBruteForceGeometryChunk();

    // Geometry clipmapping
    unsigned int clipmapLevelCount_ = 0;
    unsigned int clipmapSize_ = 0;

    // One texture per clipmap
    struct ClipmapInfo
    {
        Texture2D* texture = nullptr;
        Texture2D* normalMap = nullptr;

        Vec2i center = {-1000000, -1000000};

        ClipmapInfo(Texture2D* texture_, Texture2D* normalMap_) : texture(texture_), normalMap(normalMap_) {}
    };
    Vector<ClipmapInfo> clipmaps_;
    void updateClipmap(unsigned int clipmapIndex, const Vec2i& center);

    GeometryChunk centerChunk_;
    std::array<GeometryChunk, 4> trimChunks_;

    bool prepareForRendering();
};

}
