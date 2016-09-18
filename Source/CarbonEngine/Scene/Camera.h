/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Matrix4.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Scene/ComplexEntity.h"
#include "CarbonEngine/Scene/EntityController/PlayerEntityController.h"

namespace Carbon
{

/**
 * The main camera entity that is used to define viewpoints in a scene. Like all entities, cameras look down their
 * negative Z axis.
 */
class CARBON_API Camera : public ComplexEntity
{
public:

    Camera() { clear(); }
    ~Camera() override;

    /**
     * Returns the type of projection being used on this camera.
     */
    bool isOrthographic() const { return isOrthographic_; }

    /**
     * Returns the field of view angle in radians to use when this is a perspective projection camera. Defaults to 60
     * degrees.
     */
    float getFieldOfView() const { return fieldOfView_; }

    /**
     * Sets the field of view angle to use when this is a perspective projection camera. The angle is in radians.
     */
    void setFieldOfView(float fov);

    /**
     * Returns the dimensions of the orthographic viewing volume to use when this camera is set to use an orthographic
     * projection, see Camera::setOrthographicSize() for details about how these dimensions are set and calculated. The
     * aspect ratio to use when calculating the orthographic size will be the value of Camera::getDefaultAspectRatio()
     * unless one is supplied in \a aspectRatio.
     */
    Vec2 getOrthographicSize(float aspectRatio = 0.0f) const;

    /**
     * Sets the dimensions of the orthographic viewing volume to use when this camera is in orthographic mode. If both
     * values are zero then the dimensions of this camera's viewport are used, which is the default behavior. If one of
     * either \a width or \a height is zero then the missing value will be calculated from the camera's aspect ratio and
     * the single value that has been provided. If both \a width and \a height are non-zero then they wil be used as-is.
     */
    void setOrthographicSize(float width, float height);

    /**
     * Returns a world space rect around this camera's orthographic rect. The aspect ratio to use when calculating the
     * orthographic size will be the value of Camera::getDefaultAspectRatio() unless one is supplied in \a aspectRatio.
     */
    Rect getWorldSpaceOrthographicExtents(float aspectRatio = 0.0f) const;

    /**
     * Returns the current near plane distance.
     */
    float getNearPlaneDistance() const { return nearPlaneDistance_; }

    /**
     * Sets the near plane distance.
     */
    void setNearPlaneDistance(float d);

    /**
     * Returns the current far plane distance.
     */
    float getFarPlaneDistance() const { return farPlaneDistance_; }

    /**
     * Sets the far plane distance.
     */
    void setFarPlaneDistance(float d);

    /**
     * Returns the projection matrix for this camera. The aspect ratio to use when creating the projection matrix will
     * be the value of Camera::getDefaultAspectRatio() unless one is supplied in \a aspectRatio.
     */
    const Matrix4& getProjectionMatrix(float aspectRatio = 0.0f) const;

    /**
     * Returns the current viewport being used when rendering with this camera. The dimensions of the viewport rectangle
     * are normalized to the range 0-1. The default viewport is a unit rectangle, and so contains the entire screen.
     */
    const Rect& getViewport() const { return viewport_; }

    /**
     * Returns the current viewport being used when rendering with this camera. The dimensions of the viewport rectangle
     * are in the range 0-width and 0-height.
     */
    Rect getScreenSpaceViewport() const;

    /**
     * Sets the current viewport for this camera. The dimensions of the viewport rectangle are clamped to the range 0-1.
     */
    void setViewport(const Rect& viewport);

    /**
     * Returns whether the given screen pixel is contained inside this camera's viewport.
     */
    bool isPixelInViewport(const Vec2& pixel) const { return getScreenSpaceViewport().intersect(pixel); }

    /**
     * Returns the aspect ratio to use with this camera when rendering into the main window.
     */
    float getDefaultAspectRatio() const;

    /**
     * Returns a world-space ray that passes through the given pixel of this camera's screen space viewport. If the
     * given pixel lies outside this camera's screen space viewport then the ray's direction will be a zero vector.
     */
    Ray getRayThroughPixel(const Vec2& pixel) const;

    /**
     * Transforms a point from world space into screen space using this camera's specifications. If the given world
     * space point lies outside this camera's view then a -1, -1 vector is returned.
     */
    Vec2 worldToScreen(const Vec3& p, bool clamp = true) const;

    /**
     * Converts the passed screen position into a world space position, if the passed point lies outside this camera's
     * viewport it is clamped inside it. The z value of the specified point specifies the normalized depth value of the
     * returned point, a z value of zero will return a point on the near clip plane and a value of one will return a
     * point on the far clip plane.
     */
    Vec3 screenToWorld(const Vec3& p) const;

    /**
     * For orthographic cameras this rotates the camera around the centerpoint of its orthographic rectangle, for
     * perspective cameras this method simply passes off to Entity::rotateAroundZ().
     */
    void rotateAroundCenter(float radians);

    /**
     * Pans this camera on the X and Y axes to ensure that the passed entity is in view, the margins indicate the
     * minimum distance the entity can be from the edge of the camera's viewing volume before this method should move
     * this camera. This can be used to make sure character sprites stay on the screen. This currently only works in 2D
     * scenes.
     */
    void ensureEntityIsVisible(const Entity* entity, float horizontalMargin = 0.0f, float verticalMargin = 0.0f);

    /**
     * Pans this camera on the X and Y axes to ensure that the passed entity is centered in the view. This currently
     * only works in 2D scenes.
     */
    void centerOnEntity(const Entity* entity) { ensureEntityIsVisible(entity, FLT_MAX, FLT_MAX); }

    /**
     * Removes any roll present on this camera.
     */
    void removeRoll();

    /**
     * Returns the Renderer::Camera definition for this camera with the given target details and output destination.
     */
    Renderer::Camera getRendererCamera(const Vec2& targetDimensions, float targetFinalDisplayAspectRatio,
                                       GraphicsInterface::OutputDestination outputDestination) const;

    void clear() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;
    operator UnicodeString() const override;

private:

    Rect viewport_;

    float fieldOfView_ = 0.0f;
    float nearPlaneDistance_ = 0.0f;
    float farPlaneDistance_ = 0.0f;

    bool isOrthographic_ = false;
    Vec2 orthographicSize_;

    // Projection matrix caching
    mutable Matrix4 projectionMatrix_;
    mutable bool projectionMatrixDirty_ = true;
    mutable float lastAspectRatio_ = 0.0f;

    // Returns the aspect ratio to use with this camera when rendering into a target with given dimensions and final
    // display aspect ratio
    float getAspectRatio(float targetWidth, float targetHeight, float targetFinalDisplayAspectRatio) const;
};

}
