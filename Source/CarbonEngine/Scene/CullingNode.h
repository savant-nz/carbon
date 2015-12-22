/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/ComplexEntity.h"

namespace Carbon
{

/**
 * Culling node entity.
 */
class CARBON_API CullingNode : public ComplexEntity
{
public:

    CullingNode() { clear(); }
    ~CullingNode() override;

    void intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry) override;
    bool gatherGeometry(GeometryGather& gather) override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;
};

}
