/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Scene/Entity.h"

namespace Carbon
{

/**
 * A sky dome entity that automatically positions itself around the camera position.
 */
class CARBON_API SkyDome : public Entity
{
public:

    SkyDome() { clear(); }
    ~SkyDome() override;

    /**
     * Returns this skydome's material.
     */
    const String& getMaterial() const { return material_; }

    /**
     * Sets this skydome's material.
     */
    void setMaterial(const String& material) { material_ = material; }

    /**
     * Sets the size of this skydome.
     */
    void setDomeSize(float radius, float height);

    void clear() override;
    bool gatherGeometry(GeometryGather& gather) override;
    void precache() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;

private:

    String material_;
    GeometryChunk geometryChunk_;

    float radius_ = 0.0f;
    float height_ = 0.0f;

    bool createGeometry();
};

}
