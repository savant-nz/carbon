/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/States/StateTypes.h"
#include "CarbonEngine/Platform/TimeValue.h"
#include "CarbonEngine/Scene/GUI/GUIWindow.h"

namespace Carbon
{

/**
 * Defines a 2D sprite that can be positioned in a scene and have an animated textures applied to it.
 */
class CARBON_API Sprite : public GUIWindow
{
public:

    Sprite();
    ~Sprite() override;

    /**
     * The directory which sprite description files are stored under, currently "Sprites/".
     */
    static const UnicodeString SpriteDirectory;

    /**
     * The file extension for sprite description files, currently ".sprite".
     */
    static const UnicodeString SpriteExtension;

    /**
     * Sets the texture to use on this sprite. If sprite animation is being used then the frame count along each direction
     * should be specified. Returns success flag.
     */
    bool setSpriteTexture(const String& spriteTexture, unsigned int frameCountX = 1, unsigned int frameCountY = 1);

    /**
     * Returns the name of the currently active sprite texture.
     */
    const String& getSpriteTexture() const;

    /**
     * Returns the dimensions of the sprite texture currently applied to this sprite, or a zero vector if an error occurs. Note
     * that this method will trigger a texture load on the main thread if this sprite's texture has not yet been loaded.
     * Applications that need to avoid that scenario should either wait for the texture load thread to be idle before calling
     * this method (by checking that TextureManager::isTextureLoadThreadActive() returns false), or change their logic so as not
     * to use this method.
     */
    Vec2 getSpriteTextureDimensions() const;

    /**
     * Sets the normal map to use on this sprite, this will be used when rendering the scene if the scene has lights in it.
     */
    void setSpriteNormalMap(const String& normalMap);

    /**
     * Returns the normal map that is being used on this sprite.
     */
    const String& getSpriteNormalMap() const;

    /**
     * Returns the collision map for this sprite, which is the same as the diffuse map unless Sprite::setSpriteCollisionMap()
     * has been called to specify a custom collision map.
     */
    const String& getSpriteCollisionMap() const { return collisionMap_.length() ? collisionMap_ : getSpriteTexture(); }

    /**
     * Sets the collision map to use on this sprite when calling Sprite::makePhysical() with the \a fixed parameter set to true.
     * If this is not set then the alpha of this sprite's diffuse map will be used instead. The collision map is also used when
     * doing per-pixel sprite/sprite intersection
     */
    void setSpriteCollisionMap(const String& collisionMap);

    /**
     * Returns the current diffuse color being used on this sprite. This can be changed to apply arbitrary colors and alphas to
     * this sprite. Note that the final diffuse alpha used to render this sprite is the alpha component of this diffuse color
     * multiplied by the alpha value returned by Entity::getFinalAlpha().
     */
    const Color& getSpriteDiffuseColor() const { return spriteDiffuseColor_; }

    /**
     * Sets the diffuse color being used on this sprite. See Sprite::getSpriteDiffuseColor() for details.
     */
    virtual void setSpriteDiffuseColor(const Color& color);

    /**
     * Returns whether or not sprite lighting will be done on this sprite if there are light entities in its scene, this can be
     * disallowed in order to prevent sprites from being lit. Defaults to true.
     */
    bool isSpriteLightingAllowed() const;

    /**
     * Sets whether or not sprite lighting will be done on this sprite if there are light entities in its scene, this can be
     * disallowed in order to prevent sprites from being lit. Defaults to true.
     */
    void setSpriteLightingAllowed(bool allowed);

    /**
     * Returns the frame rate used when playing an animation on this sprite.
     */
    unsigned int getFrameRate() const { return frameRate_; }

    /**
     * Sets the frame rate to use when playing an animation on this sprite.
     */
    void setFrameRate(unsigned int frameRate);

    /**
     * Returns the horizontal frame count for animated sprites that was set by Sprite::setSpriteTexture().
     */
    unsigned int getFrameCountX() const { return frameCountX_; }

    /**
     * Returns the vertical frame count for animated sprites that was set by Sprite::setSpriteTexture().
     */
    unsigned int getFrameCountY() const { return frameCountY_; }

    /**
     * Returns whether the texture on this sprite will be reflected in the X axis when rendering.
     */
    bool isReflectedVertically() const { return isReflectedVertically_; }

    /**
     * Sets whether the texture on this sprite should be reflected in the X axis when rendering.
     */
    void setReflectedVertically(bool reflected);

    /**
     * Returns whether the texture on this sprite will be reflected in the Y axis when rendering.
     */
    bool isReflectedHorizontally() const { return isReflectedHorizontally_; }

    /**
     * Sets whether the texture on this sprite should be reflected in the Y axis when rendering.
     */
    void setReflectedHorizontally(bool reflected);

    /**
     * Starts the sprite animation at the given initial frame, the animation can play in reverse and loop automatically if
     * desired. If \a removeFromSceneOnFinish is true and the animation is not looping then when the animation finishes
     * playing the sprite will automatically call its Entity::removeFromScene() method.
     */
    void startAnimation(bool looping = false, bool reverse = false, unsigned int initialFrame = 0,
                        bool removeFromSceneOnFinish = false);

    /**
     * Stops the sprite animation.
     */
    void stopAnimation();

    /**
     * Returns the animation frame that is currently being displayed on this sprite.
     */
    unsigned int getCurrentFrame() const { return currentFrame_; }

    /**
     * Explicitly sets the current animation frame to show on this sprite.
     */
    void setCurrentFrame(unsigned int frame);

    /**
     * Returns whether or not this sprite is currently animating, i.e. animation has been started with Sprite::startAnimation()
     * and has not been paused with Sprite::setPaused().
     */
    bool isAnimationPlaying() const { return isAnimating_ && !isPaused_; }

    /**
     * If this sprite is currently animating then this method can be used to pause the animation in progress.
     */
    void setAnimationPaused(bool paused);

    /**
     * Called when an animation completes.
     */
    virtual void onAnimationFinished() {}

    /**
     * Sets the region of the sprite texture, or of each animation frame in the sprite texture when using sprite animation, that
     * should be used to texture the sprite. By default the entire area will be used, which is equivalent to a region rectangle
     * of 0, 0, 1, 1. This region can be adjusted manually using this method if desired. Note that if any of the specified
     * values are outside the 0 - 1 range then they will all be assumed to refer to a texel offset in the current sprite texture
     * rather than a normalized offset. Note that if the texture region extends outside the bounds of the sprite texture it will
     * be clamped.
     */
    void setTextureRegion(float left, float bottom, float right, float top);

    /**
     * \copydoc setTextureRegion(float, float, float, float)
     */
    void setTextureRegion(const Rect& rect)
    {
        setTextureRegion(rect.getLeft(), rect.getBottom(), rect.getRight(), rect.getTop());
    }

    /**
     * Returns the current texture region being used to texture this sprite. The rect will be normalized. See
     * Sprite::setTextureRegion() for details.
     */
    const Rect& getTextureRegion() const { return textureRegion_; }

    /**
     * Returns the current sprite blending factors being used in the passed parameters. See Sprite::setSpriteBlendingFactors()
     * for details.
     */
    void getSpriteBlendingFactors(States::BlendFactor& sourceFactor, States::BlendFactor& destinationFactor) const;

    /**
     * Sets the blending factors to use when rendering this sprite. The standard alpha blending equation applies, i.e. the final
     * color is the sum of the incoming sprite color multiplied by the chosen \a sourceFactor and the current framebuffer color
     * multiplied by the chosen \a destinationFactor. The default source and destination blending factors are \a
     * States::SourceAlpha and \a States::OneMinusSourceAlpha respectively. To disable sprite blending set to \a States::One and
     * \a States::Zero.
     */
    void setSpriteBlendingFactors(States::BlendFactor sourceFactor, States::BlendFactor destinationFactor);

    /**
     * Saves this sprite to the specified sprite description file. Returns success flag.
     */
    bool save(const String& name);

    /**
     * Loads this sprite from the specified sprite description file. Returns success flag.
     */
    bool load(const String& name);

    /**
     * This method is a handy shortcut for creating a character controller for this sprite based on its current size.
     */
    virtual bool useCharacterController()
    {
        if (!isCenteredOnLocalOrigin())
            return false;

        return useCharacterController(getHeight(), getWidth() * 0.5f);
    }

    bool useCharacterController(float height, float radius, float offset = 0.0f) override
    {
        return GUIWindow::useCharacterController(height, radius, offset);
    }

    void setMaterial(const String& material) override;
    Color getSurfaceColor(const Vec2& localPosition) const override;

    void clear() override;
    bool isPerFrameUpdateRequired() const override;
    void update() override;
    bool gatherGeometry(GeometryGather& gather) override;
    void precache() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;
    operator UnicodeString() const override;
    void invalidateFinalAlpha() override;
    int getRenderPriority() const override;
    bool intersect(const Entity* entity) const override;
    bool intersect(const Vec2& position) const override { return GUIWindow::intersect(position); }
    bool intersect(const Vec3& position) const override { return GUIWindow::intersect(position); }

protected:

    /**
     * Extends the default physical behavior to use a collision map if present, the bounding box is still used as a fallback.
     */
    PhysicsInterface::BodyObject createInternalRigidBody(float mass, bool fixed) override;

private:

    unsigned int frameRate_ = 0;
    unsigned int frameCountX_ = 0;
    unsigned int frameCountY_ = 0;
    bool isReflectedVertically_ = false;
    bool isReflectedHorizontally_ = false;

    Rect textureRegion_;

    // The sprite's texture scale and offset to use when rendering is calcuated JIT
    mutable bool isScaleAndOffsetDirty_ = true;
    void updateScaleAndOffset() const;
    Matrix4 getTextureMatrix() const;

    bool isAnimating_ = false;
    bool isPaused_ = false;
    bool isLooping_ = false;
    bool isAnimationReversed_ = false;
    TimeValue animationStartTime_;
    TimeValue animationPausedTime_;
    unsigned int currentFrame_ = 0;
    bool removeFromSceneOnAnimationFinish_ = false;
    bool updateCurrentFrame();

    Color spriteDiffuseColor_;

    Material* spriteMaterial_ = nullptr;

    String collisionMap_;
    mutable Image collisionMapImage_;
    mutable bool isCollisionMapImageLoaded_ = false;
    const Image& getSpriteCollisionMapImage() const;

    void clearSpriteDetails();
};

}
