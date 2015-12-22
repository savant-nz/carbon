/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Render/RenderEvents.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/SceneEvents.h"

namespace Carbon
{

const String MaterialManager::ExporterNoMaterialFallback = "nomaterial";

MaterialManager::MaterialManager()
{
    events().addHandler<ShaderChangeEvent>(this);
}

MaterialManager::~MaterialManager()
{
    events().removeHandler(this);

    clear();
}

bool MaterialManager::processEvent(const Event& e)
{
    if (auto sce = e.as<ShaderChangeEvent>())
    {
        // Reload the textures on each material that uses the effect that just had a change of shader
        textures().disableTextureDeletion();
        for (auto& hashLine : materials_)
        {
            for (auto& material : hashLine)
            {
                if (sce->getEffectName() == material->getEffectName() && material->areTexturesLoaded())
                {
                    material->unloadTextures();
                    material->loadTextures();
                }
            }
        }
        textures().enableTextureDeletion();
    }

    return true;
}

void MaterialManager::clear(bool onlyMaterialsLoadedFromFiles)
{
    for (auto& hashLine : materials_)
    {
        hashLine.eraseIf([&](Material* material) {
            if (onlyMaterialsLoadedFromFiles && !material->isLoadedFromFile())
                return false;

            delete material;

            return true;
        });
    }

    fallbackMaterial_ = nullptr;
}

void MaterialManager::reloadMaterials()
{
    LOG_INFO << "Reloading materials";

    // When reloading materials we don't want to have to reload textures that are still going to be in use after the materials
    // are reloaded. By default this would happen because clearing all the current materials would wipe all references to most
    // textures and they would then be deleted, only to immediately be reloaded once the fresh materials are loaded back in. To
    // get around this we disable the automatic texture deletion feature of the texture manager and then re-enable it once the
    // materials are reloaded. But since the material textures are loaded JIT this isn't enough because the new materials
    // wouldn't have been used. To get around this we take a list of the currently loaded materials and ensure that they have
    // their textures re-referenced before we re-enable the texture deletion feature on the texture manager.

    textures().disableTextureDeletion();
    auto loadedMaterials = Vector<std::pair<String, bool>>();
    for (auto& hashLine : materials_)
    {
        for (auto& material : hashLine)
        {
            if (material->isLoadedFromFile())
                loadedMaterials.emplace(material->getName(), material->areTexturesLoaded_);
        }
    }

    clear(true);

    for (auto& loadedMaterial : loadedMaterials)
    {
        if (loadedMaterial.second)
            getMaterial(loadedMaterial.first).loadTextures();
        else
            getMaterial(loadedMaterial.first);
    }

    // Turn the texture manager garbage collector back on, this will delete any textures that are now unused as a result of
    // reloading materials
    textures().enableTextureDeletion();

    LOG_INFO << "Reloaded materials";
}

Material& MaterialManager::getMaterial(const String& name, bool requireLoadedMaterial)
{
    if (name.length() == 0)
        return getFallbackMaterial();

    // Search for the material with the given name
    for (auto material : getHashLine(name))
    {
        if (material->getName() == name)
        {
            if (!requireLoadedMaterial || material->isLoaded())
                return *material;

            return getFallbackMaterial();
        }
    }

    // Create and load the new material
    auto material = createMaterial(name);
    material->load(name);

    if (!requireLoadedMaterial || material->isLoaded())
        return *material;

    return getFallbackMaterial();
}

Vector<String> MaterialManager::getMaterialNames() const
{
    auto names = Vector<String>();

    for (auto& hashLine : materials_)
    {
        for (auto material : hashLine)
            names.append(material->getName());
    }

    return names;
}

bool MaterialManager::hasMaterial(const String& material) const
{
    return getHashLine(material).has([&](const Material* m) { return m->getName() == material; });
}

Material* MaterialManager::createMaterial(const String& name)
{
    if (name.length() == 0)
        return nullptr;

    auto& hashLine = getHashLine(name);
    for (auto material : hashLine)
    {
        if (material->getName() == name)
        {
            LOG_ERROR << "Material name has already been taken: " << name;
            return nullptr;
        }
    }

    auto material = new Material(name);
    hashLine.append(material);

    return material;
}

bool MaterialManager::unloadMaterial(Material* material)
{
    if (!material)
        return false;

    if (!getHashLine(material->getName()).unorderedEraseValue(material))
        return false;

    delete material;

    return true;
}

Material& MaterialManager::getFallbackMaterial()
{
    if (!fallbackMaterial_)
    {
        // The fallback material is created JIT
        fallbackMaterial_ = createMaterial(".Error");
        assert(fallbackMaterial_ && "Failed creating fallback material");

        fallbackMaterial_->setEffect("BaseSurface");
        fallbackMaterial_->setParameter("diffuseMap", "MaterialError");
        fallbackMaterial_->setParameter("diffuseColor", Color::White);
    }

    return *fallbackMaterial_;
}

}
