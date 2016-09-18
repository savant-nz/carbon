/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventHandler.h"

namespace Carbon
{

/**
 * Handles all the materials in use by scene objects.
 */
class CARBON_API MaterialManager : public EventHandler, private Noncopyable
{
public:

    /**
     * This is the material used by exporters as a fallback for when the mesh being exported doesn't have any material
     * set. This is currently set to "nomaterial".
     */
    static const String ExporterNoMaterialFallback;

    /**
     * Handles events relevant to the material manager.
     */
    bool processEvent(const Event& e) override;

    /**
     * Reloads all the currently loaded material definitions.
     */
    void reloadMaterials();

    /**
     * Returns the specified material. If the material is not loaded then it will be loaded and returned. If the
     * specified material is invalid or failed to load then the fallback material will be returned.
     */
    Material& getMaterial(const String& name, bool requireLoadedMaterial = true);

    /**
     * Returns a vector containing the names of all loaded materials.
     */
    Vector<String> getMaterialNames() const;

    /**
     * Returns whether there is a currently loaded material with the given name.
     */
    bool hasMaterial(const String& material) const;

    /**
     * Creates and returns a new material with the given name, or null if the name is already taken or is invalid.
     */
    Material* createMaterial(const String& name);

    /**
     * Unloads the given material and releases all resources it is holding. Returns success flag.
     */
    bool unloadMaterial(Material* material);

    /**
     * Returns the fallback material that is used when a material does not exist or fails to load.
     */
    Material& getFallbackMaterial();

private:

    MaterialManager();
    ~MaterialManager() override;
    friend class Globals;

    void clear(bool onlyMaterialsLoadedFromFiles = false);

    std::array<Vector<Material*>, 511> materials_;

    const Vector<Material*>& getHashLine(const String& name) const
    {
        return materials_[name.hash() % materials_.size()];
    }
    Vector<Material*>& getHashLine(const String& name) { return materials_[name.hash() % materials_.size()]; }

    Material* fallbackMaterial_ = nullptr;
};

}
