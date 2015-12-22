/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Math/Matrix3.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"
#include "CarbonEngine/Scene/Light.h"

namespace Carbon
{

const VersionInfo LightVersionInfo(3, 0);

Light::~Light()
{
    onDestruct();
    clear();
}

void Light::clear()
{
    Entity::clear();

    type_ = UnknownLightType;
    color_.setRGBA(1.0f, 1.0f, 1.0f, 0.25f);
    setRadius(0.0f);

    maximumConeAngle_ = Math::QuarterPi * 0.5f;
    minimumConeAngle_ = maximumConeAngle_ * 0.5f;

    setProjectionTextureName(String::Empty);
    setProjectionCubemapName(String::Empty);

    isSpecularEnabled_ = false;
    specularIntensity_ = 1.0f;

    isShadowingEnabled_ = false;

    isProjectionMatrixDirty_ = true;
}

void Light::setProjectionTextureName(const String& name)
{
    projectionTextureName_ = name;

    textures().releaseTexture(projectionTexture_);
    projectionTexture_ = nullptr;
}

const Texture* Light::getProjectionTexture() const
{
    if (!projectionTextureName_.length())
        return nullptr;

    if (!projectionTexture_)
    {
        projectionTexture_ =
            textures().setupTexture(GraphicsInterface::Texture2D, projectionTextureName_, "WorldEnvironmentMap");
    }

    return projectionTexture_;
}

void Light::setProjectionCubemapName(const String& name)
{
    projectionCubemapName_ = name;

    textures().releaseTexture(projectionCubemapTexture_);
    projectionCubemapTexture_ = nullptr;
}

const Texture* Light::getProjectionCubemapTexture() const
{
    if (!projectionCubemapName_.length())
        return nullptr;

    if (!projectionCubemapTexture_)
        projectionCubemapTexture_ = textures().setupTexture(GraphicsInterface::TextureCubemap, projectionCubemapName_, "Sky");

    return projectionCubemapTexture_;
}

const Matrix4& Light::getProjectionMatrix() const
{
    if (isProjectionMatrixDirty_)
    {
        projectionMatrix_ = Matrix4::getPerspectiveProjection(maximumConeAngle_ * 2.0f, 1.0f, 0.25f, radius_);
        isProjectionMatrixDirty_ = false;
    }

    return projectionMatrix_;
}

void Light::setAmbientLight(const Color& color)
{
    setType(AmbientLight);
    setColor(color);
}

void Light::setDirectionalLight(const Color& color, const Vec3& direction)
{
    setType(DirectionalLight);
    setColor(color);
    setDirection(direction);
    setSpecularEnabled(false);
    setShadowsEnabled(false);
}

void Light::setPointLight(const Color& color, const Vec3& position, float radius)
{
    setType(PointLight);
    setColor(color);
    setWorldPosition(position);
    setRadius(radius);
    setSpecularEnabled(false);
    setShadowsEnabled(false);
}

void Light::setSpotLight(const Color& color, const Vec3& position, float radius, const Vec3& direction)
{
    setType(SpotLight);
    setColor(color);
    setWorldPosition(position);
    setRadius(radius);
    setDirection(direction);
    setSpecularEnabled(false);
    setShadowsEnabled(false);
}

void Light::calculateLocalAABB() const
{
    Entity::calculateLocalAABB();

    if (type_ == PointLight || type_ == SpotLight)
    {
        localAABB_.addPoint(Vec3(radius_));
        localAABB_.addPoint(Vec3(-radius_));
    }
}

void Light::save(FileWriter& file) const
{
    Entity::save(file);

    file.beginVersionedSection(LightVersionInfo);

    file.writeEnum(type_);
    file.write(color_, radius_, minimumConeAngle_, maximumConeAngle_, projectionTextureName_);
    file.write(projectionCubemapName_, isSpecularEnabled_, specularIntensity_, isShadowingEnabled_);

    file.endVersionedSection();
}

void Light::load(FileReader& file)
{
    try
    {
        clear();

        Entity::load(file);

        auto readVersion = file.beginVersionedSection(LightVersionInfo);

        auto radius = 0.0f;

        if (readVersion.getMajor() == 3)
        {
            file.readEnum(type_, LastLightType);
            file.read(color_, radius, minimumConeAngle_, maximumConeAngle_, projectionTextureName_);
            file.read(projectionCubemapName_, isSpecularEnabled_, specularIntensity_, isShadowingEnabled_);
        }
        else if (readVersion.getMajor() == 2)
        {
            file.readEnum(type_, LastLightType);
            file.read(color_, radius, minimumConeAngle_, maximumConeAngle_, projectionCubemapName_);

            // v2.1, specular properties
            if (readVersion.getMinor() >= 1)
            {
                file.read(isSpecularEnabled_);
                file.read(specularIntensity_);
            }

            // v2.2, projection texture
            if (readVersion.getMinor() >= 2)
                file.read(projectionTextureName_);
        }
        else
            throw Exception() << "Light entity " << readVersion << " is not supported";

        file.endVersionedSection();

        setRadius(radius);
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

Light::operator UnicodeString() const
{
    auto info = Vector<UnicodeString>();

    info.append(UnicodeString() + "color: " + getColor());

    if (getType() == AmbientLight)
        info.prepend("light type: ambient");
    else if (getType() == DirectionalLight)
    {
        info.prepend("light type: directional");
        info.append(UnicodeString() + "direction: " + getDirection());
        info.append(UnicodeString() + "specular: " + isSpecularEnabled());
    }
    else if (getType() == PointLight || getType() == SpotLight)
    {
        info.prepend(getType() == PointLight ? "light type: point" : "light type: spot");
        info.append(UnicodeString() + "radius: " + getRadius());
        info.append(UnicodeString() + "specular: " + isSpecularEnabled());
        if (getType() == SpotLight)
            info.append(UnicodeString() + "direction: " + getDirection());
    }

    info.prepend("");

    return Entity::operator UnicodeString() << info;
}

}
