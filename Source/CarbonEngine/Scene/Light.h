/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Matrix4.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Scene/Entity.h"

namespace Carbon
{

/**
 * Light entity that describes a light source in a scene. Lights can be directional, positional, spot or ambient.
 */
class CARBON_API Light : public Entity, public Renderer::Light
{
public:

    /**
     * Available light types.
     */
    enum LightType
    {
        /**
         * No light type. Lights without a type are ignored.
         */
        UnknownLightType,

        /**
         * An ambient light that lights all surfaces equally regardless of position or direction.
         */
        AmbientLight,

        /**
         * A directional light that simulates parallel light rays coming from a source at infinity. It has only direction and no
         * position.
         */
        DirectionalLight,

        /**
         * A point light that emits light equally in all directions. It has a position, and direction is only relevant when
         * using a projection texture on the light.
         */
        PointLight,

        /**
         * A spot light that emits light in a specific cone down the light's world space Z direction. The size of the spotlight
         * cone can be set using Light::setMinimumConeAngle() and Light::setMaximumConeAngle().
         */
        SpotLight,

        /**
         * The size of this enumeration.
         */
        LastLightType
    };

    Light() { clear(); }
    ~Light() override;

    /**
     * Returns the type of this light.
     */
    LightType getType() const { return type_; }

    /**
     * Sets the type of this light.
     */
    void setType(LightType type) { type_ = type; }

    /**
     * Sets the color of this light, by default the light color will be white.
     */
    void setColor(const Color& color) { color_ = color; }

    /**
     * For point and spot lights, sets the radius of this light. The radius is used as an attenuation cutoff distance.
     */
    void setRadius(float radius)
    {
        radius_ = radius;
        onLocalAABBChanged();
        isProjectionMatrixDirty_ = true;
    }

    /**
     * For spot lights, this returns the angle away from the Z axis at which cone attenuation will start occurring. This value
     * should not be greater than the maximum cone angle, and can't be greater than half Pi. Defaults to Pi / 8.
     */
    float getMinimumConeAngle() const override { return minimumConeAngle_; }

    /**
     * See Light::getMinimumConeAngle().
     */
    void setMinimumConeAngle(float angle) { minimumConeAngle_ = Math::clamp(angle, 0.0f, Math::HalfPi); }

    /**
     * For spot lights, this returns the angle away from the Z axis at which cone attenuation will finish, i.e. there will be no
     * illumination outside this angle. This value should not be less than the minimum cone angle, and can't be greater than
     * half Pi. Defaults to Pi / 4.
     */
    float getMaximumConeAngle() const override { return maximumConeAngle_; }

    /**
     * See Light::getMaximumConeAngle().
     */
    void setMaximumConeAngle(float angle)
    {
        maximumConeAngle_ = Math::clamp(angle, 0.0f, Math::HalfPi);
        isProjectionMatrixDirty_ = true;
    }

    /**
     * Returns the name of the 2D texture currently being projected by this light. This is projected out from positional light
     * sources along the Z axis of this light. The maximum cone angle (see Light::getMaximumConeAngle()) can be used to control
     * the size of the 2D projection.
     */
    const String& getProjectionTextureName() const { return projectionTextureName_; }

    /**
     * Sets the 2D projection texture to use on this light, see Light::getProjectionTextureName() for details.
     */
    void setProjectionTextureName(const String& name);

    /**
     * Returns whether this light has a 2D projection texture applied to it.
     */
    bool hasProjectionTexture() const { return projectionTextureName_.length() > 0; }

    /**
     * Returns the name of the projection cubemap texture currently being used on this light. Light projection cubemaps are
     * projected out from positional light sources onto the surrounding scene and take the light's orientation into account.
     * Only available on point lights and spot lights. Note that it is possible to simulate spot lights using a point light with
     * an appropriate projection cubemap applied to it. If this name is blank then no projection cubemap will be used on this
     * light.
     */
    const String& getProjectionCubemapName() const { return projectionCubemapName_; }

    /**
     * Sets the projection cubemap texture to use on this light, see Light::getProjectionCubemap() for details.
     */
    void setProjectionCubemapName(const String& name);

    /**
     * Returns whether this light has a projection cubemap applied to it.
     */
    bool hasProjectionCubemap() const { return projectionCubemapName_.length() > 0; }

    /**
     * Returns whether specular highlights should be computed when rendering this light source. Defaults to false. Turning
     * specular highlights off may increase rendering performance in applications that are fragment processor bound. They may
     * also be turned off for artistic reasons.
     */
    bool isSpecularEnabled() const override { return isSpecularEnabled_; }

    /**
     * Sets whether specular lighting should be computed when rendering this light, see Light::isSpecularEnabled() for details.
     */
    void setSpecularEnabled(bool enabled) { isSpecularEnabled_ = enabled; }

    /**
     * Returns the current intensity of the specular highlights cast by this light, this is only used when specular is enabled
     * on this light. Defaults to 1.0,
     */
    float getSpecularIntensity() const override { return specularIntensity_; }

    /**
     * Sets the intensity of the specular highlights cast by this light, see Light::getSpecularIntensity() for details.
     */
    void setSpecularIntensity(float intensity) { specularIntensity_ = intensity; }

    /**
     * Sets whether this light should cast shadows. Defaults to false.
     */
    void setShadowsEnabled(bool enabled) { isShadowingEnabled_ = enabled; }

    /**
     * Returns whether this light should cast shadows. Defaults to false.
     */
    bool isShadowingEnabled() const override { return isShadowingEnabled_; }

    /**
     * Shorthand method for setting this light up as an ambient light. This method sets the light type to \a AmbientLight and
     * sets the light color.
     */
    void setAmbientLight(const Color& color);

    /**
     * Shorthand method for setting this light up as a directional light. This method sets the light type to \a
     * DirectionalLight, sets the light direction, and sets the light color.
     */
    void setDirectionalLight(const Color& color, const Vec3& direction);

    /**
     * Shorthand method for setting this light up as a point light. This method sets the light type to \a PointLight, sets the
     * light position, and sets the light color.
     */
    void setPointLight(const Color& color, const Vec3& position, float radius);

    /**
     * Shorthand method for setting this light up as a spot light. This method sets the light type to \a SpotLight, sets the
     * light color, sets the light radius, and sets the light direction.
     */
    void setSpotLight(const Color& color, const Vec3& position, float radius, const Vec3& direction = Vec3::Zero);

    void clear() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;
    operator UnicodeString() const override;

    bool isDirectionalLight() const override { return type_ == DirectionalLight; }
    bool isPointLight() const override { return type_ == PointLight; }
    bool isSpotLight() const override { return type_ == SpotLight; }
    const Color& getColor() const override { return color_; }
    float getRadius() const override { return radius_; }
    const SimpleTransform& getLightTransform() const override { return getWorldTransform(); }
    AABB getLightAABB() const override { return getWorldAABB(); }
    const Texture* getProjectionTexture() const override;
    const Texture* getProjectionCubemapTexture() const override;
    const Matrix4& getProjectionMatrix() const override;

protected:

    void calculateLocalAABB() const override;

private:

    LightType type_ = UnknownLightType;
    Color color_;
    float radius_ = 0.0f;

    // Angles for spotlights
    float minimumConeAngle_ = 0.0f;
    float maximumConeAngle_ = 0.0f;

    // 2D texture that spotlights and point lights project out onto the scene
    String projectionTextureName_;
    mutable const Texture* projectionTexture_ = nullptr;

    // Cubemap that spotlights and point lights project out onto the scene
    String projectionCubemapName_;
    mutable const Texture* projectionCubemapTexture_ = nullptr;

    // Specular properties
    bool isSpecularEnabled_ = false;
    float specularIntensity_ = 1.0f;

    // Shadow properties
    bool isShadowingEnabled_ = false;

    // Projection matrix used for spot lights and 2D projective textures
    mutable bool isProjectionMatrixDirty_ = true;
    mutable Matrix4 projectionMatrix_;
};

}
