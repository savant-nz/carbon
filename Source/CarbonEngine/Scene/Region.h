/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/Entity.h"

namespace Carbon
{

/**
 * The Region entity specifies a concave volume that can be used to define arbitrary volumes in a scene.
 */
class CARBON_API Region : public Entity
{
public:

    /**
     * This is the material prefix that causes the scene geometry it's applied to to be converted into a region entity
     * in the scene compiler.
     */
    static const String RegionMaterialPrefix;

    Region() { clear(); }
    ~Region() override;

    /**
     * Creates this region from the given triangles.
     */
    void setup(const TriangleArray& triangles);

    /**
     * Returns whether the given world space point is inside this region.
     */
    virtual bool intersect(const Vec3& point) const;

    void clear() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;
    bool intersect(const Entity* entity) const override { return Entity::intersect(entity); }

private:

    Vector<RawTriangle> triangles_;

    class BSPTreeNode
    {
    public:

        Plane plane;
        int front = -1;
        int back = -1;

        void save(FileWriter& file) const { file.write(plane, front, back); }
        void load(FileReader& file) { file.read(plane, front, back); }
    };

    Vector<BSPTreeNode> bspTree_;
    Plane chooseSplitPlane(const Vector<RawTriangle>& triangles);
    void buildBSPTree(unsigned int node, const Vector<RawTriangle>& triangles);

    Color color_;
};

}
