/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Scene/Camera.h"
#include "CarbonEngine/Scene/IntersectionResult.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Sound/SoundInterface.h"

namespace Carbon
{

const auto CameraVersionInfo = VersionInfo(1, 0);

Camera::~Camera()
{
    onDestruct();
    clear();
}

void Camera::clear()
{
    isOrthographic_ = false;
    fieldOfView_ = Math::degreesToRadians(60.0f);
    nearPlaneDistance_ = 1.0f;
    farPlaneDistance_ = 15000.0f;
    orthographicSize_.setXY(0.0f, 0.0f);

    lastAspectRatio_ = 1.0f;
    projectionMatrixDirty_ = true;

    viewport_ = Rect::One;

    Entity::clear();
}

void Camera::save(FileWriter& file) const
{
    Entity::save(file);

    file.beginVersionedSection(CameraVersionInfo);

    file.write(fieldOfView_, nearPlaneDistance_, farPlaneDistance_);
    file.write(isOrthographic_, orthographicSize_, viewport_);

    file.endVersionedSection();
}

void Camera::load(FileReader& file)
{
    try
    {
        clear();

        Entity::load(file);

        file.beginVersionedSection(CameraVersionInfo);

        file.read(fieldOfView_, nearPlaneDistance_, farPlaneDistance_);
        file.read(isOrthographic_, orthographicSize_, viewport_);

        file.endVersionedSection();
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

void Camera::setFieldOfView(float fov)
{
    fieldOfView_ = fov;
    projectionMatrixDirty_ = true;
}

Vec2 Camera::getOrthographicSize(float aspectRatio) const
{
    if (aspectRatio == 0.0f)
        aspectRatio = getDefaultAspectRatio();

    if (orthographicSize_.x == 0.0f && orthographicSize_.y > 0.0f)
        return {orthographicSize_.y * aspectRatio, orthographicSize_.y};

    if (orthographicSize_.y == 0.0f && orthographicSize_.x > 0.0f)
        return {orthographicSize_.x, orthographicSize_.x / aspectRatio};

    if (orthographicSize_.y > 0.0f && orthographicSize_.x > 0.0f)
        return orthographicSize_;

    return {getScreenSpaceViewport().getHeight() * aspectRatio, getScreenSpaceViewport().getHeight()};
}

void Camera::setOrthographicSize(float width, float height)
{
    isOrthographic_ = true;
    orthographicSize_.setXY(width, height);
    projectionMatrixDirty_ = true;
}

Rect Camera::getWorldSpaceOrthographicExtents(float aspectRatio) const
{
    auto v = getOrthographicSize(aspectRatio);

    auto localSpaceOrthographicRect = Rect(0.0f, 0.0f, v.x, v.y);

    return {localSpaceOrthographicRect, getWorldTransform()};
}

void Camera::setNearPlaneDistance(float d)
{
    nearPlaneDistance_ = d;
    projectionMatrixDirty_ = true;
}

void Camera::setFarPlaneDistance(float d)
{
    farPlaneDistance_ = d;
    projectionMatrixDirty_ = true;
}

const Matrix4& Camera::getProjectionMatrix(float aspectRatio) const
{
    if (aspectRatio == 0.0f)
        aspectRatio = getDefaultAspectRatio();

    if (projectionMatrixDirty_ || aspectRatio != lastAspectRatio_)
    {
        if (isOrthographic())
        {
            auto orthographicSize = getOrthographicSize(aspectRatio);

            if (getScene()->is2D())
            {
                // 2D orthographic scenes have the camera position in the bottom left corner
                projectionMatrix_ = Matrix4::getOrthographicProjection(Rect(0.0f, 0.0f, orthographicSize.x, orthographicSize.y),
                                                                       nearPlaneDistance_, farPlaneDistance_);
            }
            else
            {
                // 3D orthographic scenes have the camera position at the center of the screen

                auto width = orthographicSize.x * 0.5f;
                auto height = orthographicSize.y * 0.5f;

                projectionMatrix_ = Matrix4::getOrthographicProjection(Rect(-width, -height, width, height), nearPlaneDistance_,
                                                                       farPlaneDistance_);
            }
        }
        else
        {
            projectionMatrix_ =
                Matrix4::getPerspectiveProjection(fieldOfView_, aspectRatio, nearPlaneDistance_, farPlaneDistance_);
        }

        lastAspectRatio_ = aspectRatio;
        projectionMatrixDirty_ = false;
    }

    return projectionMatrix_;
}

Rect Camera::getScreenSpaceViewport() const
{
    return viewport_ * Vec2(platform().getWindowWidthf(), platform().getWindowHeightf());
}

void Camera::setViewport(const Rect& viewport)
{
    viewport_ = viewport;
    viewport_.clamp();
}

float Camera::getDefaultAspectRatio() const
{
    return getAspectRatio(platform().getWindowWidthf(), platform().getWindowHeightf(), platform().getFinalDisplayAspectRatio());
}

float Camera::getAspectRatio(float targetWidth, float targetHeight, float targetFinalDisplayAspectRatio) const
{
    return (viewport_ * Vec2(targetWidth, targetHeight)).getAspectRatio() /
        ((targetWidth / targetHeight) / targetFinalDisplayAspectRatio);
}

Ray Camera::getRayThroughPixel(const Vec2& pixel) const
{
    if (!isPixelInViewport(pixel))
        return {Vec3::Zero, -Vec3::UnitZ};

    auto target = Matrix4::unproject(pixel, getWorldTransform().getInverse(), getProjectionMatrix(), getScreenSpaceViewport());

    if (isOrthographic())
        return {target - getDirection() * nearPlaneDistance_, getDirection()};

    return {getWorldPosition(), (target - getWorldPosition()).normalized()};
}

Vec2 Camera::worldToScreen(const Vec3& p, bool clamp) const
{
    // Transform point into local camera space
    auto localPoint = worldToLocal(p);

    // Check the point is in front of the camera
    if (localPoint.z > 0.0f)
        return {-1.0f, -1.0f};

    // Multiply by projection matrix and do a perspective division
    auto& projection = getProjectionMatrix();

    auto w = projection[3] * localPoint.x + projection[7] * localPoint.y + projection[11] * localPoint.z + projection[15];
    auto clipSpacePoint = (projection * localPoint).toVec2() / w;

    if (clipSpacePoint.x < -1.0f || clipSpacePoint.x > 1.0f || clipSpacePoint.y < -1.0f || clipSpacePoint.y > 1.0f)
        return {-1.0f, -1.0f};

    // Return the screen space coordinate
    return getScreenSpaceViewport().getPoint(clipSpacePoint.x * 0.5f + 0.5f, clipSpacePoint.y * 0.5f + 0.5f);
}

Vec3 Camera::screenToWorld(const Vec3& p) const
{
    auto viewport = getScreenSpaceViewport();

    // Calculate clip space point, clamping inside the viewport
    auto clipSpace = Vec3();

    clipSpace.x = Math::clamp01((p.x - viewport.getLeft()) / viewport.getWidth()),
    clipSpace.y = Math::clamp01((p.y - viewport.getBottom()) / viewport.getHeight());
    clipSpace.z = Math::clamp01(p.z);

    clipSpace *= 2.0f;
    clipSpace -= Vec3::One;

    // Convert from clip space to world space
    auto inverseViewProjection = Matrix4();
    (getProjectionMatrix() * getWorldTransform().getInverse().getMatrix()).getInverse(inverseViewProjection);

    return inverseViewProjection * clipSpace;
}

void Camera::rotateAroundCenter(float radians)
{
    if (isOrthographic())
        rotateAroundPoint(localToWorld(getOrthographicSize() * 0.5f), Quaternion::createRotationZ(radians));
    else
        rotateAroundZ(radians);
}

void Camera::removeRoll()
{
    auto direction = getDirection();

    setWorldOrientation(Quaternion::createRotationXY(direction.getPitch(), direction.getYaw()));
}

void Camera::ensureEntityIsVisible(const Entity* entity, float horizontalMargin, float verticalMargin)
{
    if (!entity || !entity->getScene() || !entity->getScene()->is2D())
        return;

    auto aabb = entity->getWorldAABB();
    auto orthographicSize = getOrthographicSize();

    // Clamp margins
    horizontalMargin = std::min(horizontalMargin, (orthographicSize.x - aabb.getWidth()) * 0.5f);
    verticalMargin = std::min(verticalMargin, (orthographicSize.y - aabb.getHeight()) * 0.5f);

    // Move camera as needed
    auto movement = Vec2();

    if (getWorldPosition().x + horizontalMargin > aabb.getMinimum().x)
        movement.x = aabb.getMinimum().x - (getWorldPosition().x + horizontalMargin);
    else if (getWorldPosition().x + orthographicSize.x - horizontalMargin < aabb.getMaximum().x)
        movement.x = aabb.getMaximum().x - (getWorldPosition().x + orthographicSize.x - horizontalMargin);

    if (getWorldPosition().y + verticalMargin > aabb.getMinimum().y)
        movement.y = aabb.getMinimum().y - (getWorldPosition().y + verticalMargin);
    else if (getWorldPosition().y + orthographicSize.y - verticalMargin < aabb.getMaximum().y)
        movement.y = aabb.getMaximum().y - (getWorldPosition().y + orthographicSize.y - verticalMargin);

    move(movement);
}

Renderer::Camera Camera::getRendererCamera(const Vec2& targetDimensions, float targetFinalDisplayAspectRatio,
                                           GraphicsInterface::OutputDestination outputDestination) const
{
    if (outputDestination == GraphicsInterface::OutputDefault)
    {
        auto aspectRatio = getAspectRatio(targetDimensions.x, targetDimensions.y, targetFinalDisplayAspectRatio);
        auto viewport = viewport_ * targetDimensions;

        return {getWorldTransform(), viewport, getProjectionMatrix(aspectRatio), getNearPlaneDistance(), getFarPlaneDistance()};
    }
    else if (outputDestination == GraphicsInterface::OutputOculusRiftLeftEye ||
             outputDestination == GraphicsInterface::OutputOculusRiftRightEye)
    {
        // Oculus Rift camera setups for 2D and 3D

        auto& riftTextureDimensions = platform().getOculusRiftTextureDimensions();

        if (isOrthographic())
        {
            return {getWorldTransform(), riftTextureDimensions, getProjectionMatrix(riftTextureDimensions.getAspectRatio()),
                    getNearPlaneDistance(), getFarPlaneDistance()};
        }
        else
        {
            if (outputDestination == GraphicsInterface::OutputOculusRiftLeftEye)
            {
                auto& eyeTransform = platform().getOculusRiftTransformLeftEye();

                return {SimpleTransform(getWorldPosition() + eyeTransform.getPosition(), eyeTransform.getOrientation()),
                        riftTextureDimensions,
                        platform().getOculusRiftProjectionMatrixLeftEye(getNearPlaneDistance(), getFarPlaneDistance()),
                        getNearPlaneDistance(), getFarPlaneDistance()};
            }
            else if (outputDestination == GraphicsInterface::OutputOculusRiftRightEye)
            {
                auto& eyeTransform = platform().getOculusRiftTransformRightEye();

                return {SimpleTransform(getWorldPosition() + eyeTransform.getPosition(), eyeTransform.getOrientation()),
                        riftTextureDimensions,
                        platform().getOculusRiftProjectionMatrixRightEye(getNearPlaneDistance(), getFarPlaneDistance()),
                        getNearPlaneDistance(), getFarPlaneDistance()};
            }
        }
    }

    assert(false && "Invalid output destination");

    return {};
}

Camera::operator UnicodeString() const
{
    auto info = Vector<UnicodeString>();

    info.append("");
    if (isOrthographic())
        info.append(UnicodeString() + "orthographic size: " + getOrthographicSize());
    else
        info.append(UnicodeString() + "fov: " + fieldOfView_);

    info.append(UnicodeString() + "near plane: " + nearPlaneDistance_);
    info.append(UnicodeString() + "far plane: " + farPlaneDistance_);
    if (getViewport() != Rect::One)
        info.append(UnicodeString() + "screen space viewport: " + getScreenSpaceViewport());

    return ComplexEntity::operator UnicodeString() << info;
}

}
