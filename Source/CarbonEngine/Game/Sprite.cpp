/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Game/ScrollingLayer.h"
#include "CarbonEngine/Game/Sprite.h"
#include "CarbonEngine/Image/ImageFormatRegistry.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/SimpleTimer.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Texture/Texture.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

const UnicodeString Sprite::SpriteDirectory = "Sprites/";
const UnicodeString Sprite::SpriteExtension = ".sprite";

const auto SpriteVersionInfo = VersionInfo(1, 1);

const auto diffuseMapParameter = ParameterArray::Lookup("diffuseMap");
const auto normalMapParameter = ParameterArray::Lookup("normalMap");

Sprite::Sprite() : spriteMaterial_(nullptr)
{
    // Create a material for this sprite
    static auto suffix = 0U;
    material_ = String() + ".sprite." + suffix++;
    spriteMaterial_ = materials().createMaterial(material_);

    spriteMaterial_->setEffect("InternalSprite");
    spriteMaterial_->setParameter(Parameter::blend, true);

    clear();

    // Set a default sprite texture and size
    setSpriteTexture("White");
    setSize(1.0f, 1.0f);
}

Sprite::~Sprite()
{
    onDestruct();
    clear();

    if (!materials().unloadMaterial(spriteMaterial_))
        LOG_WARNING << "Failed unloading material";

    spriteMaterial_ = nullptr;
}

bool Sprite::setSpriteTexture(const String& spriteTexture, unsigned int frameCountX, unsigned int frameCountY)
{
    if (!frameCountX || !frameCountY)
    {
        LOG_ERROR << "Total frame count must be at least one";
        return false;
    }

    if (getSpriteTexture() == spriteTexture && frameCountX_ == frameCountX && frameCountY_ == frameCountY)
        return true;

    frameCountX_ = frameCountX;
    frameCountY_ = frameCountY;

    spriteMaterial_->setParameter(diffuseMapParameter, spriteTexture);

    currentFrame_ = 0;
    isScaleAndOffsetDirty_ = true;

    return true;
}

const String& Sprite::getSpriteTexture() const
{
    return spriteMaterial_->getParameter(diffuseMapParameter).getString();
}

Vec2 Sprite::getSpriteTextureDimensions() const
{
    auto texture = spriteMaterial_->getTextureForParameter(diffuseMapParameter);
    if (!texture)
        return Vec2::Zero;

    return {float(texture->getImage().getWidth()), float(texture->getImage().getHeight())};
}

void Sprite::setSpriteNormalMap(const String& normalMap)
{
    if (getSpriteNormalMap() == normalMap)
        return;

    spriteMaterial_->setParameter(normalMapParameter, normalMap.length() ? normalMap : String::Empty);
}

const String& Sprite::getSpriteNormalMap() const
{
    return spriteMaterial_->getParameter(normalMapParameter).getString();
}

void Sprite::setSpriteCollisionMap(const String& collisionMap)
{
    collisionMap_ = collisionMap;
    collisionMapImage_.clear();
    isCollisionMapImageLoaded_ = false;
}

void Sprite::setSpriteDiffuseColor(const Color& color)
{
    spriteDiffuseColor_ = color;
    spriteMaterial_->setParameter(Parameter::diffuseColor, adjustColorAlpha(spriteDiffuseColor_));
}

bool Sprite::isSpriteLightingAllowed() const
{
    return spriteMaterial_->getParameter(Parameter::isLightingAllowed).getBoolean();
}

void Sprite::setSpriteLightingAllowed(bool allowed)
{
    spriteMaterial_->setParameter(Parameter::isLightingAllowed, allowed);
}

void Sprite::setFrameRate(unsigned int frameRate)
{
    if (!frameRate)
    {
        LOG_ERROR << "Frame rate must be at least one";
        return;
    }

    frameRate_ = frameRate;
}

void Sprite::setReflectedVertically(bool reflected)
{
    isReflectedVertically_ = reflected;
    isScaleAndOffsetDirty_ = true;
}

void Sprite::setReflectedHorizontally(bool reflected)
{
    isReflectedHorizontally_ = reflected;
    isScaleAndOffsetDirty_ = true;
}

void Sprite::clearSpriteDetails()
{
    frameRate_ = 1;
    frameCountX_ = 1;
    frameCountY_ = 1;
    isReflectedVertically_ = false;
    isReflectedHorizontally_ = false;

    textureRegion_ = Rect::One;

    isAnimating_ = false;
    isPaused_ = false;
    isLooping_ = false;
    isAnimationReversed_ = false;
    animationStartTime_.clear();
    animationPausedTime_.clear();
    currentFrame_ = 0;
    removeFromSceneOnAnimationFinish_ = false;
    collisionMap_.clear();
    collisionMapImage_.clear();
    isCollisionMapImageLoaded_ = false;

    isScaleAndOffsetDirty_ = false;
    spriteMaterial_->setParameter(Parameter::scaleAndOffset, 1.0f, 1.0f, 0.0f, 0.0f);
    spriteMaterial_->setParameter(diffuseMapParameter, Parameter::Empty);
    spriteMaterial_->setParameter(normalMapParameter, Parameter::Empty);

    setSpriteBlendingFactors(States::SourceAlpha, States::OneMinusSourceAlpha);
    setSpriteDiffuseColor(Color::White);
    setSpriteLightingAllowed(true);

    // Sprites default to being centered on their local origin
    setCenteredOnLocalOrigin(true);
}

void Sprite::clear()
{
    GUIWindow::clear();
    clearSpriteDetails();
}

bool Sprite::isPerFrameUpdateRequired() const
{
    if (isAnimationPlaying() && !isLooping_)
        return true;

    return GUIWindow::isPerFrameUpdateRequired();
}

void Sprite::update()
{
    if (isAnimationPlaying() && !isLooping_)
    {
        if (!updateCurrentFrame())
            return;
    }

    GUIWindow::update();
}

bool Sprite::gatherGeometry(GeometryGather& gather)
{
    if (!ComplexEntity::gatherGeometry(gather))
        return false;

    if (shouldProcessGather(gather))
    {
        if (!isCulledBy(gather) && getWidth() > 0.0f && getHeight() > 0.0f)
        {
            if (isAnimationPlaying() && isLooping_)
            {
                if (!updateCurrentFrame())
                    return true;
            }

            updateScaleAndOffset();

            gather.changePriority(getRenderPriority());
            gather.changeTransformation(localToWorld(-localToWindow(Vec3::Zero)), getWorldOrientation());
            gather.newMaterial(spriteMaterial_);
            gather.addRectangle(getWidth(), getHeight());
        }
    }

    return true;
}

void Sprite::precache()
{
    GUIWindow::precache();
    spriteMaterial_->precache();
}

void Sprite::invalidateFinalAlpha()
{
    GUIWindow::invalidateFinalAlpha();

    setSpriteDiffuseColor(spriteDiffuseColor_);
}

int Sprite::getRenderPriority() const
{
    return ComplexEntity::getRenderPriority();
}

Sprite::operator UnicodeString() const
{
    auto info = Vector<UnicodeString>();

    info.append("");
    info.append("texture: " + getSpriteTexture());

    if (getSpriteNormalMap().length())
        info.append("normal map: " + getSpriteNormalMap());

    if (getSpriteDiffuseColor() != Color::White)
        info.append(UnicodeString() + "diffuse color: " + getSpriteDiffuseColor());

    if (getTextureRegion() != Rect::One)
        info.append(UnicodeString() + "region: " + getTextureRegion());

    return GUIWindow::operator UnicodeString() << info;
}

void Sprite::setMaterial(const String& material)
{
    LOG_ERROR << "Setting a material on a sprite is not allowed, use Sprite::setSpriteTexture() instead";
}

Color Sprite::getSurfaceColor(const Vec2& localPosition) const
{
    auto u = localPosition.x / getWidth();
    auto v = localPosition.y / getHeight();

    if (isCenteredOnLocalOrigin())
    {
        u += 0.5f;
        v += 0.5f;
    }

    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
        return Color::Zero;

    auto sample = getTextureMatrix() * Vec3(u, v);

    auto surfaceColor = Color();
    spriteMaterial_->sampleTexture(diffuseMapParameter.getName(), sample.x, sample.y, surfaceColor);

    return adjustColorAlpha(surfaceColor) * getSpriteDiffuseColor();
}

void Sprite::updateScaleAndOffset() const
{
    if (!isScaleAndOffsetDirty_)
        return;

    isScaleAndOffsetDirty_ = false;

    auto currentFrame = currentFrame_;

    if (isReflectedHorizontally_)
        currentFrame = (currentFrame / frameCountX_) * frameCountX_ + (frameCountX_ - 1 - (currentFrame % frameCountX_));
    if (isReflectedVertically_)
        currentFrame = (frameCountY_ - 1 - currentFrame / frameCountX_) * frameCountX_ + (currentFrame % frameCountX_);

    auto oneOverFrameCounts = Vec2(1.0f / float(frameCountX_), 1.0f / float(frameCountY_));

    auto offset = Vec2(float(currentFrame % frameCountX_), float(currentFrame / frameCountX_));
    offset += Vec2(textureRegion_.getLeft(), textureRegion_.getBottom());
    offset *= oneOverFrameCounts;

    if (isReflectedHorizontally_)
        offset.x = 1.0f - offset.x;
    if (isReflectedVertically_)
        offset.y = 1.0f - offset.y;

    // Compute scale
    auto scale = Vec2(isReflectedHorizontally_ ? -1.0f : 1.0f, isReflectedVertically_ ? -1.0f : 1.0f);
    scale *= oneOverFrameCounts;
    scale *= Vec2(textureRegion_.getWidth(), textureRegion_.getHeight());

    // Set the new scale and offset on this sprite's material
    spriteMaterial_->setParameter(Parameter::scaleAndOffset, scale.x, scale.y, offset.x, offset.y);
}

Matrix4 Sprite::getTextureMatrix() const
{
    updateScaleAndOffset();

    auto scaleAndOffset = spriteMaterial_->getParameter(Parameter::scaleAndOffset).getFloat4();

    return Matrix4::getScaleAndTranslation(Vec3(scaleAndOffset[0], scaleAndOffset[1]),
                                           Vec3(scaleAndOffset[2], scaleAndOffset[3]));
}

bool Sprite::updateCurrentFrame()
{
    auto previousCurrentFrame = currentFrame_;

    currentFrame_ = (animationStartTime_.getTimeSince() / TimeValue(1.0f / frameRate_)) % (frameCountX_ * frameCountY_);

    if (isAnimationReversed_)
        currentFrame_ = frameCountX_ * frameCountY_ - currentFrame_ - 1;

    if (currentFrame_ != previousCurrentFrame)
    {
        isScaleAndOffsetDirty_ = true;

        if (!isLooping_ && ((!isAnimationReversed_ && currentFrame_ < previousCurrentFrame) ||
                            (isAnimationReversed_ && currentFrame_ > previousCurrentFrame)))
        {
            currentFrame_ = 0;
            stopAnimation();
            onAnimationFinished();

            if (removeFromSceneOnAnimationFinish_)
            {
                removeFromScene();
                return false;
            }
        }
    }

    return true;
}

void Sprite::startAnimation(bool looping, bool reverse, unsigned int initialFrame, bool removeFromSceneOnFinish)
{
    isAnimating_ = true;
    isPaused_ = false;

    isLooping_ = looping;
    isAnimationReversed_ = reverse;
    removeFromSceneOnAnimationFinish_ = removeFromSceneOnFinish;

    setCurrentFrame(initialFrame);

    recheckIsPerFrameUpdateRequired();
}

void Sprite::stopAnimation()
{
    isAnimating_ = false;
    isPaused_ = false;
    recheckIsPerFrameUpdateRequired();
}

void Sprite::setAnimationPaused(bool paused)
{
    if (!isAnimating_ || isPaused_ == paused)
        return;

    isPaused_ = paused;

    if (isPaused_)
        animationPausedTime_ = platform().getTime();
    else
        animationStartTime_ += platform().getTime() - animationPausedTime_;

    recheckIsPerFrameUpdateRequired();
}

void Sprite::setCurrentFrame(unsigned int frame)
{
    frame %= frameCountX_ * frameCountY_;

    animationStartTime_ = platform().getTime() - float(frame) / float(frameRate_);
    updateCurrentFrame();
}

void Sprite::setTextureRegion(float left, float bottom, float right, float top)
{
    if (left > 1.0f || bottom > 1.0f || right > 1.0f || top > 1.0f)
    {
        auto dimensions = getSpriteTextureDimensions();

        // Normalize the specified texture region
        left /= dimensions.x;
        bottom /= dimensions.y;
        right /= dimensions.x;
        top /= dimensions.y;
    }

    textureRegion_ = Rect(left, bottom, right, top);
    textureRegion_.clamp();

    isScaleAndOffsetDirty_ = true;
}

void Sprite::getSpriteBlendingFactors(States::BlendFactor& sourceFactor, States::BlendFactor& destinationFactor) const
{
    sourceFactor = States::BlendFactor(spriteMaterial_->getParameter(Parameter::blendSourceFactor).getInteger());
    destinationFactor = States::BlendFactor(spriteMaterial_->getParameter(Parameter::blendDestinationFactor).getInteger());
}

void Sprite::setSpriteBlendingFactors(States::BlendFactor sourceFactor, States::BlendFactor destinationFactor)
{
    spriteMaterial_->setParameter(Parameter::blendSourceFactor, sourceFactor);
    spriteMaterial_->setParameter(Parameter::blendDestinationFactor, destinationFactor);
}

void Sprite::save(FileWriter& file) const
{
    GUIWindow::save(file);

    file.beginVersionedSection(SpriteVersionInfo);

    auto sourceFactor = States::BlendFactor();
    auto destinationFactor = States::BlendFactor();
    getSpriteBlendingFactors(sourceFactor, destinationFactor);

    file.write(getSpriteTexture(), getSpriteNormalMap(), collisionMap_, spriteDiffuseColor_, textureRegion_);
    file.write(frameRate_, frameCountX_, frameCountY_, isReflectedVertically_, isReflectedHorizontally_);
    file.writeEnum(sourceFactor);
    file.writeEnum(destinationFactor);
    file.write(isAnimating_, isPaused_, isLooping_, isAnimationReversed_);
    file.write(animationStartTime_.getSecondsSince(), animationPausedTime_.getSecondsSince());
    file.write(isSpriteLightingAllowed());

    file.endVersionedSection();
}

void Sprite::load(FileReader& file)
{
    try
    {
        GUIWindow::load(file);

        clearSpriteDetails();

        auto readVersion = file.beginVersionedSection(SpriteVersionInfo);

        auto texture = String();
        auto normalMap = String();
        auto sourceFactor = States::BlendFactor();
        auto destinationFactor = States::BlendFactor();
        file.read(texture, normalMap, collisionMap_, spriteDiffuseColor_, textureRegion_);
        file.read(frameRate_, frameCountX_, frameCountY_, isReflectedVertically_, isReflectedHorizontally_);
        file.readEnum(sourceFactor);
        file.readEnum(destinationFactor);
        file.read(isAnimating_, isPaused_, isLooping_, isAnimationReversed_);

        auto secondsSinceAnimationStart = 0.0f;
        auto secondsSinceAnimationPaused = 0.0f;
        file.read(secondsSinceAnimationStart, secondsSinceAnimationPaused);
        animationStartTime_ = platform().getTime() - secondsSinceAnimationStart;
        animationPausedTime_ = platform().getTime() - secondsSinceAnimationPaused;

        // v1.1, sprite lighting allowed flag
        if (readVersion.getMinor() >= 1)
        {
            auto isSpriteLightingAllowed = false;
            file.read(isSpriteLightingAllowed);
            setSpriteLightingAllowed(isSpriteLightingAllowed);
        }

        file.endVersionedSection();

        setSpriteTexture(texture, frameCountX_, frameCountY_);
        setSpriteNormalMap(normalMap);
        setSpriteBlendingFactors(sourceFactor, destinationFactor);
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

PhysicsInterface::BodyObject Sprite::createInternalRigidBody(float mass, bool fixed)
{
    if (fixed)
    {
        auto timer = SimpleTimer();

        // Load the collision map
        auto& image = getSpriteCollisionMapImage();
        if (image.isValid2DImage())
        {
            // Convert the alpha to 2D polygons
            auto polygons = Vector<Vector<Vec2>>();
            if (physics().convertImageAlphaTo2DPolygons(image, polygons, isReflectedHorizontally(), isReflectedVertically()))
            {
                // Convert polygons to collision geometry
                auto vertices = Vector<Vec3>();
                auto triangles = Vector<RawIndexedTriangle>();
                physics().convert2DPolygonsToCollisionGeometry(polygons, vertices, triangles);

                // Apply scale and offset
                for (auto& vertex : vertices)
                    vertex = vertex * Vec3(getWidth(), getHeight(), 1.0f) - localToWindow(Vec2::Zero);

                LOG_INFO << "Using sprite collision map: " << getSpriteCollisionMap() << ", time: " << timer;

                // Create final physics body
                return physics().createGeometryBodyFromTemplate(
                    physics().createBodyTemplateFromGeometry(vertices, triangles, true, 0.5f), mass, fixed, this,
                    getWorldTransform());
            }

            LOG_WARNING << "Failed converting alpha channel of image " << getSpriteCollisionMap()
                        << " to collision geometry, falling back to bounding box, sprite name: " << getName();
        }
        else
            LOG_INFO << "Not using collision map on sprite with texture: " << getSpriteTexture();
    }

    return GUIWindow::createInternalRigidBody(mass, fixed);
}

const Image& Sprite::getSpriteCollisionMapImage() const
{
    if (collisionMap_.length())
    {
        // A custom collision map has been specified so try to use it if possible

        // Load collision map JIT
        if (!isCollisionMapImageLoaded_)
        {
            collisionMapImage_.clear();
            ImageFormatRegistry::loadImageFile(Texture::TextureDirectory + collisionMap_, collisionMapImage_);
            isCollisionMapImageLoaded_ = true;
        }

        // Return the collision map if it loaded successfully
        if (collisionMapImage_.isValid2DImage())
            return collisionMapImage_;
    }

    // Default to using the sprite's diffuse map
    return spriteMaterial_->getTextureForParameter(diffuseMapParameter)->getImage();
}

bool Sprite::save(const String& name)
{
    try
    {
        if (!getSpriteTexture().length())
            throw Exception("This sprite has no texture");

        LOG_INFO << "Saving sprite - '" << name << "'";

        auto file = FileWriter();
        fileSystem().open(SpriteDirectory + name + SpriteExtension, file, true);

        file.writeText(UnicodeString() + "Size                " + getWidth() + " " + getHeight());
        file.writeText(UnicodeString() + "Texture             " + getSpriteTexture());
        if (getSpriteNormalMap().length())
            file.writeText(UnicodeString() + "NormalMap           " + getSpriteNormalMap());
        if (frameCountX_ > 1 || frameCountY_ > 1)
            file.writeText(UnicodeString() + "FrameCounts         " + frameCountX_ + " " + frameCountY_);
        if (getSpriteDiffuseColor() != Color::White)
            file.writeText(UnicodeString() + "DiffuseColor        " + getSpriteDiffuseColor());
        if (isReflectedVertically_)
            file.writeText("FlipVertical");
        if (isReflectedHorizontally_)
            file.writeText("FlipHorizontal");
        if (!isSpriteLightingAllowed())
            file.writeText("LightingDisallowed");

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name << "' - " << e;

        return false;
    }
}

bool Sprite::load(const String& name)
{
    try
    {
        clearSpriteDetails();

        // Open the file
        auto lineTokens = Vector<Vector<String>>();
        if (!fileSystem().readTextFile(SpriteDirectory + name + SpriteExtension, lineTokens))
            throw Exception("Failed opening file");

        setName(name);

        for (auto& line : lineTokens)
        {
            if (line[0].asLower() == "size")
            {
                if (line.size() != 3 || !line[1].isFloat() || !line[2].isFloat())
                    throw Exception("Invalid size");

                setSize(line[1].asFloat(), line[2].asFloat());
            }
            else if (line[0].asLower() == "texture")
            {
                if (line.size() != 2)
                    throw Exception("Invalid texture");

                setSpriteTexture(line[1], frameCountX_, frameCountY_);
            }
            else if (line[0].asLower() == "normalmap")
            {
                if (line.size() != 2)
                    throw Exception("Invalid normal map");

                setSpriteNormalMap(line[1]);
            }
            else if (line[0].asLower() == "framecounts")
            {
                if (line.size() != 2 || !line[1].isInteger() || !line[2].isInteger())
                    throw Exception("Invalid frame counts");

                setSpriteTexture(line[1], line[2].asInteger(), line[3].asInteger());
            }
            else if (line[0].asLower() == "diffusecolor")
            {
                if (line.size() != 5 || !line[1].isFloat() || !line[2].isFloat() || !line[3].isFloat() || !line[4].isFloat())
                    throw Exception("Invalid diffuse color");

                setSpriteDiffuseColor(Color(line[1].asFloat(), line[2].asFloat(), line[3].asFloat(), line[4].asFloat()));
            }
            else if (line[0].asLower() == "flipvertical")
            {
                if (line.size() != 1)
                    throw Exception("Invalid flip vertical");

                setReflectedVertically(true);
            }
            else if (line[0].asLower() == "fliphorizontal")
            {
                if (line.size() != 1)
                    throw Exception("Invalid flip horizontal");

                setReflectedHorizontally(true);
            }
            else if (line[0].asLower() == "lightingdisallowed")
            {
                if (line.size() != 1)
                    throw Exception("Invalid lighting disallowed");

                setSpriteLightingAllowed(false);
            }
            else
                LOG_WARNING << "Unrecognized command '" << line[0] << "' in sprite '" << name << "'";
        }

        LOG_INFO << "Loaded sprite: '" << name << "'";

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name << "' - " << e.get();

        clearSpriteDetails();

        return false;
    }
}

bool Sprite::intersect(const Entity* entity) const
{
    if (!GUIWindow::intersect(entity))
        return false;

    if (auto sprite = entity->asEntityType<Sprite>())
    {
        auto visualizePerPixelSpriteIntersection = false;
        auto opaqueAlphaCutoff = 0.1f;

        // Transforms to move between the local space of this sprite and the other sprite
        auto otherSpriteToThisSprite = getWorldTransform().getInverse() * sprite->getWorldTransform();
        auto thisSpriteToOtherSprite = sprite->getWorldTransform().getInverse() * getWorldTransform();

        // Get the bounding planes of the other sprite's rect in the local space of this sprite
        auto planes = std::array<Plane, 4>{{otherSpriteToThisSprite * Plane(sprite->getLocalAABB().getMinimum(), -Vec3::UnitX),
                                            otherSpriteToThisSprite * Plane(sprite->getLocalAABB().getMinimum(), -Vec3::UnitY),
                                            otherSpriteToThisSprite * Plane(sprite->getLocalAABB().getMaximum(), Vec3::UnitX),
                                            otherSpriteToThisSprite * Plane(sprite->getLocalAABB().getMaximum(), Vec3::UnitY)}};

        // Get the corner vertices of this sprite's rect in local space
        auto corners = std::array<Vec3, 4>();
        getLocalAABB().toRect().getCorners(corners, SimpleTransform::Identity);

        auto vertices = Vector<Vec3>(corners);

        // Clip away all parts of this sprite's rect that lie outside the other sprite's rect
        for (auto& plane : planes)
        {
            if (!plane.clipConvexPolygon(vertices))
                return false;
        }

        // Get the collision map images to use
        auto& collisionMap = getSpriteCollisionMapImage();
        auto& collisionMap2 = sprite->getSpriteCollisionMapImage();
        if (!collisionMap.isValid2DImage() || !collisionMap2.isValid2DImage())
            return true;

        // Texture matrices for both sprites, these are needed in order to know which areas of the sprite textures to use
        auto textureMatrix = getTextureMatrix();
        auto textureMatrix2 = sprite->getTextureMatrix();

        // Get local space rectangle around the intersection area on this sprite
        auto localSpaceIntersectionBounds = Rect(vertices);

        // Convert local space rectangle extents into uv texture coordinates
        auto dimensions = Vec2(getWidth(), getHeight());
        auto uvMin = textureMatrix * (localToWindow(localSpaceIntersectionBounds.getMinimum()) / dimensions);
        auto uvMax = textureMatrix2 * (localToWindow(localSpaceIntersectionBounds.getMaximum()) / dimensions);

        // Work out the corresponding texel range based on the texture dimensions
        auto xStart = int(uvMin.x * float(collisionMap.getWidth()));
        auto xEnd = int(uvMax.x * float(collisionMap.getWidth()));
        auto yStart = int(uvMin.y * float(collisionMap.getHeight()));
        auto yEnd = int(uvMax.y * float(collisionMap.getHeight()));

        auto xStep = xEnd > xStart ? 1 : -1;
        auto yStep = yEnd > yStart ? 1 : -1;

        auto localSpaceTexelSize = Vec2(1.0f / (xEnd - xStart), 1.0f / (yEnd - yStart));

        // Visualize intersection result
        if (visualizePerPixelSpriteIntersection)
        {
            const_cast<Scene*>(getScene())->clearImmediateGeometry();
            const_cast<Scene*>(getScene())->addImmediateGeometry(vertices, getWorldTransform(), Color::Green);
        }

        // Iterate through the intersection area seeing if any opaque texels touch
        for (auto y = yStart; y != yEnd; y += yStep)
        {
            for (auto x = xStart; x != xEnd; x += xStep)
            {
                if (collisionMap.getPixelColor(uint(x), uint(y)).a > opaqueAlphaCutoff)
                {
                    // Texel outline in local entity space of the other entity
                    auto texelCorners = std::array<Vec2, 4>{
                        {thisSpriteToOtherSprite *
                             localSpaceIntersectionBounds.getPoint(float(x - xStart) * localSpaceTexelSize.x,
                                                                   float(y - yStart) * localSpaceTexelSize.y),
                         thisSpriteToOtherSprite *
                             localSpaceIntersectionBounds.getPoint(float(x - xStart) * localSpaceTexelSize.x,
                                                                   float(y - yStart + yStep) * localSpaceTexelSize.y),
                         thisSpriteToOtherSprite *
                             localSpaceIntersectionBounds.getPoint(float(x - xStart + xStep) * localSpaceTexelSize.x,
                                                                   float(y - yStart + yStep) * localSpaceTexelSize.y),
                         thisSpriteToOtherSprite *
                             localSpaceIntersectionBounds.getPoint(float(x - xStart + xStep) * localSpaceTexelSize.x,
                                                                   float(y - yStart) * localSpaceTexelSize.y)}};

                    // Build rectangle around the texel
                    auto localSpaceTexelBounds = Rect(texelCorners[0]);
                    for (auto i = 1U; i < 4; i++)
                        localSpaceTexelBounds.addPoint(texelCorners[i]);

                    // If this texel lies outside the other sprite then go no further
                    if (!localSpaceTexelBounds.intersect(sprite->getLocalAABB().toRect()))
                        continue;

                    // Convert local space texel extents into uv texture coordinates
                    auto uvMin2 = textureMatrix2 * (sprite->localToWindow(localSpaceTexelBounds.getMinimum()) /
                                                    Vec2(sprite->getWidth(), sprite->getHeight()));
                    auto uvMax2 = textureMatrix2 * (sprite->localToWindow(localSpaceTexelBounds.getMaximum()) /
                                                    Vec2(sprite->getWidth(), sprite->getHeight()));

                    // Work out the corresponding texel range in this sprite based on the texture dimensions
                    auto xStart2 = int(uvMin2.x * float(collisionMap2.getWidth()));
                    auto xEnd2 = int(uvMax2.x * float(collisionMap2.getWidth()));
                    auto yStart2 = int(uvMin2.y * float(collisionMap2.getHeight()));
                    auto yEnd2 = int(uvMax2.y * float(collisionMap2.getHeight()));

                    // Make sure we test against at least one texel in each dimension
                    if (xStart2 == xEnd2)
                        xEnd2 = xStart2 + int(Math::getSign(uvMax2.x - uvMin2.x));
                    if (yStart2 == yEnd2)
                        yEnd2 = yStart2 + int(Math::getSign(uvMax2.y - uvMin2.y));

                    auto xStep2 = xEnd2 > xStart2 ? 1 : -1;
                    auto yStep2 = yEnd2 > yStart2 ? 1 : -1;

                    // Iterate through the intersection area seeing if it contains any opaque pixels
                    for (auto y2 = yStart2; y2 != yEnd2; y2 += yStep2)
                    {
                        for (auto x2 = xStart2; x2 != xEnd2; x2 += xStep2)
                        {
                            if (collisionMap2.getPixelColor(uint(x2), uint(y2)).a > opaqueAlphaCutoff)
                            {
                                if (visualizePerPixelSpriteIntersection)
                                {
                                    vertices.clear();
                                    vertices.append(texelCorners[0]);
                                    vertices.append(texelCorners[1]);
                                    vertices.append(texelCorners[2]);
                                    vertices.append(texelCorners[3]);
                                    const_cast<Scene*>(getScene())
                                        ->addImmediateGeometry(vertices, sprite->getWorldTransform(), Color::Red);

                                    localSpaceTexelBounds.getCorners(corners, SimpleTransform::Identity);
                                    const_cast<Scene*>(getScene())
                                        ->addImmediateGeometry(corners, sprite->getWorldTransform(), Color::Blue);
                                }

                                return true;
                            }
                        }
                    }
                }
            }
        }

        return false;
    }

    return true;
}

}
