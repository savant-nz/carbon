/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Scene/ComplexEntity.h"

namespace Carbon
{

/**
 * A resolution-independent 2D scrolling layer defined by an array of tiles. Multiple scrolling layers can be combined together
 * in a scene to get parallax scrolling effects.
 */
class CARBON_API ScrollingLayer : public ComplexEntity
{
public:

    /**
     * The file extension for scrolling layers, currently ".scrollinglayer".
     */
    static const UnicodeString ScrollingLayerExtension;

    /**
     * When this is true a call to ScrollingLayer::makePhysical() will also add a series of colored lines to the scene that
     * outline the polygons that were generated from this scrolling layer's collision maps. This is useful when debugging
     * scrolling layer physics. The lines are added using the Scene::addImmediateGeometry() method. Defaults to false.
     */
    static bool VisualizeCollisionMapEdges;

    ScrollingLayer() { clear(); }
    ~ScrollingLayer() override;

    /**
     * Sets the dimensions of the tile array for this scrolling layer, existing tile textures are retained where possible when
     * changing the size of a scrolling layer.
     */
    void setGridSize(unsigned int width, unsigned int height);

    /**
     * Returns the number of vertical tiles in this scrolling layer, as set up by ScrollingLayer::setGridSize().
     */
    unsigned int getTileCountX() const { return tileCountX_; }

    /**
     * Returns the number of horizontal tiles in this scrolling layer, as set up by ScrollingLayer::setGridSize().
     */
    unsigned int getTileCountY() const { return tileCountY_; }

    /**
     * Returns the overall tile scale to use when rendering, this can be used to directly scale a scrolling layer's size.
     * Defaults to 1.0.
     */
    float getTileScale() const { return tileScale_; }

    /**
     * Sets the overall tile scale to use when rendering, this can be used to directly scale a scrolling layer's size. Defaults
     * to 1.0.
     */
    void setTileScale(float tileScale);

    /**
     * Returns the aspect ratio to render tiles with. This can be used to define non-square tiles, larger aspect ratios reduce
     * the height of a tile while leaving its width unchanged. Defaults to 1.0.
     */
    float getTileAspectRatio() const { return tileAspectRatio_; }

    /**
     * Sets the aspect ratio to render tiles with. This can be used to define non-square tiles, larger aspect ratios reduce the
     * height of a tile while leaving its width unchanged. Defaults to 1.0.
     */
    void setTileAspectRatio(float tileAspectRatio);

    /**
     * Returns the width of a single tile, this is equal to the tile scale.
     */
    float getTileWidth() const { return tileScale_; }

    /**
     * Returns the height of a single tile, this is equal to the tile scale over the tile aspect ratio.
     */
    float getTileHeight() const { return tileScale_ / tileAspectRatio_; }

    /**
     * Returns the size of a single tile computed from the tile scale and tile aspect ratio.
     */
    Vec2 getTileSize() const { return {getTileWidth(), getTileHeight()}; }

    /**
     * Returns the entire size of this layer computed from the tile scale, tile aspect ratio, and X and Y tile counts.
     */
    Vec2 getLayerSize() const { return {getTileWidth() * float(tileCountX_), getTileHeight() * float(tileCountY_)}; }

    /**
     * Returns the overall scaling of the speed this layer will scroll at as the camera moves across it. Defaults to 1.0.
     */
    float getSpeedScale() const { return speedScale_; }

    /**
     * Sets the overall scaling of the speed this layer will scroll at as the camera moves across it. Defaults to 1.0.
     */
    void setSpeedScale(float speedScale) { speedScale_ = speedScale; }

    /**
     * Returns whether this scrolling layer is repeating in the X axis.
     */
    bool getRepeatX() const { return repeatX_; }

    /**
     * Sets whether this scrolling layer should repeat in the X axis.
     */
    void setRepeatX(bool repeat) { repeatX_ = repeat; }

    /**
     * Returns whether this scrolling layer is repeating in the Y axis.
     */
    bool getRepeatY() const { return repeatY_; }

    /**
     * Sets whether this scrolling layer should repeat in the Y axis.
     */
    void setRepeatY(bool repeat) { repeatY_ = repeat; }

    /**
     * Returns the texture being used on the given tile.
     */
    const String& getTileTexture(unsigned int x, unsigned int y) const;

    /**
     * Sets the texture to use on the given tile.
     */
    void setTileTexture(unsigned int x, unsigned int y, const String& texture);

    /**
     * Sets the specified texture for use on all tiles.
     */
    void setAllTileTextures(const String& texture);

    /**
     * Returns the current texture that will be used on tiles that have not had a texture set explicitly using
     * ScrollingLayer::setTileTexture(). The default is an empty string which will result in TextureError.png being rendered for
     * the tile.
     */
    const String& getDefaultTileTexture() const { return defaultTileTexture_; }

    /**
     * Sets the texture to use on tiles that have not had a texture set explicitly using ScrollingLayer::setTileTexture().
     */
    void setDefaultTileTexture(const String& texture);

    /**
     * Returns the visibility flag for the specified tile. Tiles are visible by default.
     */
    bool isTileVisible(unsigned int x, unsigned int y) const;

    /**
     * Sets the visibility flag on the specified tile. Tiles are visible by default.
     */
    void setTileVisible(unsigned int x, unsigned int y, bool visible);

    /**
     * Returns whether the specified tile is flipped vertically when rendering, defaults to false.
     */
    bool isTileFlippedVertically(unsigned int x, unsigned int y) const;

    /**
     * Sets whether the specified tile should be flipped vertically when rendering, defaults to false.
     */
    void setTileFlippedVertically(unsigned int x, unsigned int y, bool isFlipped);

    /**
     * Returns whether the specified tile is flipped horizontally, defaults to false.
     */
    bool isTileFlippedHorizontally(unsigned int x, unsigned int y) const;

    /**
     * Sets whether the specified tile should be flipped horizontally when rendering, defaults to false.
     */
    void setTileFlippedHorizontally(unsigned int x, unsigned int y, bool isFlipped);

    /**
     * Returns the name of the collision map that will be used on the specified tile. If a specific collision map has been set
     * with ScrollingLayer::setTileCollisionMap() then it will be returned, otherwise the default of "<tile texture>_collision"
     * will be used.
     */
    String getTileCollisionMap(unsigned int x, unsigned int y) const;

    /**
     * Sets the collision map to use for the specified tile, the alpha of this image will be used to create collision geometry
     * when this scrolling layer is made physical. If no collision map is specified for a tile then the default of "<tile
     * texture>_collision" will be used.
     */
    void setTileCollisionMap(unsigned int x, unsigned int y, const String& collisionMap);

    /**
     * Returns the name of the normal map that will be used on the specified tile. If a specific normal map has been set with
     * ScrollingLayer::setTileNormalMap() then it will be returned, otherwise the default of "<tile texture>_normal" will be
     * used.
     */
    const String& getTileNormalMap(unsigned int x, unsigned int y) const;

    /**
     * Sets the normal map to use for the specified tile. If no normal map is specified for a tile then the default of "<tile
     * texture>_normal" will be used.
     */
    void setTileNormalMap(unsigned int x, unsigned int y, const String& normalMap);

    /**
     * Adds an entity to this scrolling layer's scene and treats it as part of the layer which means that it will be
     * automatically repeated along with the tiles that make up this layer (assuming that repeating in the X or Y axes is turned
     * on). Note that this repeating will not duplicate the passed entity, it just gets automatically moved around as the layer
     * scrolls in order to give the appearance of repeating indefinitely. This means that the entity will appear only once even
     * if the layer size is small enough that it would be expected to appear multiple times. The current local position of the
     * passed entity is taken as the layer-space position for the repeating entity, and the
     * ScrollingLayer::setRepeatingEntityPosition() method must be used in order to change this position because calling
     * Entity::setLocalPosition() directly would get overridden by the automatic repositioning done by the layer on the entity.
     * Returns success flag.
     */
    bool addRepeatingEntity(Entity* entity);

    /**
     * Creates a new entity of the specified type and adds it as a repeating entity to this scrolling layer using
     * ScrollingLayer::addRepeatingEntity(). The name of the new entity can be specified, and the new entity's `initialize()`
     * method will be called with any additional arguments that are passed. Returns the new entity or null on failure.
     */
    template <typename EntityType, typename... ArgTypes>
    EntityType* addRepeatingEntity(const String& name = String::Empty,
                                   ArgTypes&&... args CARBON_CLANG_PRE_3_7_PARAMETER_PACK_BUG_WORKAROUND)
    {
        auto entity = SubclassRegistry<Entity>::create<EntityType>();
        if (!entity || !addRepeatingEntity(entity))
        {
            SubclassRegistry<Entity>::destroy(entity);
            return nullptr;
        }

        entity->setName(name);
        initializeIfArgsPassed(entity, std::forward<ArgTypes>(args)...);

        return entity;
    }

    /**
     * Changes the layer-space position of the passed repeating entity, the entity should have been added using
     * ScrollingLayer::addRepeatingEntity().
     */
    bool setRepeatingEntityPosition(Entity* entity, const Vec2& position);

    /**
     * Removes a repeating entity added with ScrollingLayer::addRepeatingEntity(). This is called automatically when a repeating
     * entity is removed from this layer using Entity::removeFromScene() or an equivalent. Returns success flag.
     */
    bool removeRepeatingEntity(Entity* entity);

    /**
     * Saves this scrolling layer to a layer description file. Returns success flag.
     */
    bool save(const String& name) const;

    /**
     * Loads this scrolling layer from the specified layer description file. Returns success flag.
     */
    bool load(const String& name);

    /**
     * Clamps the position of the passed Camera to the bounds of this scrolling layer, this can be used to avoid showing the
     * edges of non-repeating scrolling layers.
     */
    void clampCameraPositionToLayerBounds(Camera* camera, bool negX, bool posX, bool negY, bool posY) const;

    /**
     * Returns the diffuse color for all tiles on this layer, the default color is white.
     */
    const Color& getLayerDiffuseColor(const Color& color) const { return layerDiffuseColor_; }

    /**
     * Sets the diffuse color used by all tiles on this layer, the default color is white.
     */
    void setLayerDiffuseColor(const Color& color);

    void clear() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;
    bool gatherGeometry(GeometryGather& gather) override;
    void precache() override;

protected:

    PhysicsInterface::BodyObject createInternalRigidBody(float mass, bool fixed) override;

private:

    unsigned int tileCountX_ = 0;
    unsigned int tileCountY_ = 0;
    float tileAspectRatio_ = 0.0f;
    float tileScale_ = 0.0f;
    float speedScale_ = 0.0f;
    bool repeatX_ = false;
    bool repeatY_ = false;

    Vec2 lastOrthographicSize_;

    struct Tile
    {
        String texture;
        bool isVisible = true;

        String normalMap;
        mutable String normalMapToUse;

        String collisionMap;
        bool isFlippedVertically = false;
        bool isFlippedHorizontally = false;

        Material* material = nullptr;
    };
    Vector<Tile> tiles_;

    String defaultTileTexture_;
    String defaultTileNormalMap_;

    Color layerDiffuseColor_;

    unsigned int visibleTilesX_ = 0;
    unsigned int visibleTilesY_ = 0;
    GeometryChunk geometryChunk_;
    void clearVisibleTiles();
    void createVisibleTiles();

    Vector<Material*> materials_;
    Material* getMaterialForTile(const Tile& tile);
    void unloadUnusedMaterials();

    void clearLayerDetails();

    struct RepeatingEntity
    {
        Entity* entity = nullptr;

        Vec3 realPosition;

        RepeatingEntity() {}
        RepeatingEntity(Entity* entity_) : entity(entity_) {}
    };
    Vector<RepeatingEntity> repeatingEntities_;
};

}
