/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Scene/Region.h"

namespace Carbon
{

const String Region::RegionMaterialPrefix = "tools.region.";

const VersionInfo RegionVersionInfo(1, 0);

Region::~Region()
{
    onDestruct();
    clear();
}

void Region::setup(const TriangleArray& triangles)
{
    // Store raw triangles that make up this region
    triangles_.resize(triangles.size());
    for (auto i = 0U; i < triangles.size(); i++)
    {
        triangles_[i].setVertex(0, triangles[i].getVertexPosition(0));
        triangles_[i].setVertex(1, triangles[i].getVertexPosition(1));
        triangles_[i].setVertex(2, triangles[i].getVertexPosition(2));
    }

    // Create BSP tree for doing in/out of region tests
    bspTree_.resize(1);
    buildBSPTree(0, triangles_);
}

Plane Region::chooseSplitPlane(const Vector<RawTriangle>& triangles)
{
    auto bestPlane = Plane();
    auto bestScore = INT_MAX;

    for (auto& candidateTriangle : triangles)
    {
        // Try this triangle as a split plane
        auto testPlane = Plane(candidateTriangle.getVertex(0), candidateTriangle.getNormal());

        auto score = 0;
        for (auto& triangle : triangles)
        {
            auto result = triangle.classify(testPlane);
            if (result == Plane::Back)
                score--;
            else if (result == Plane::Front)
                score++;
        }

        score = abs(score);

        // Update plane to use if this is the best score so far
        if (score < bestScore)
        {
            bestScore = score;
            bestPlane = testPlane;
        }
    }

    return bestPlane;
}

void Region::buildBSPTree(unsigned int node, const Vector<RawTriangle>& triangles)
{
    bspTree_[node].plane = chooseSplitPlane(triangles);

    // Build front and back lists
    auto frontList = Vector<RawTriangle>();
    auto backList = Vector<RawTriangle>();
    for (const auto& triangle : triangles)
    {
        switch (triangle.classify(bspTree_[node].plane))
        {
            case Plane::Front:
                frontList.append(triangle);
                break;

            case Plane::Back:
                backList.append(triangle);
                break;

            case Plane::Spanning:
                frontList.append(triangle);
                backList.append(triangle);
                break;

            case Plane::Coincident:
                break;
        };
    }

    // Recurse on front side if needed
    if (frontList.size())
    {
        bspTree_.enlarge(1);
        bspTree_[node].front = bspTree_.size() - 1;
        buildBSPTree(bspTree_.size() - 1, frontList);
    }

    // Recurse on back side if needed
    if (backList.size())
    {
        bspTree_.enlarge(1);
        bspTree_[node].back = bspTree_.size() - 1;
        buildBSPTree(bspTree_.size() - 1, backList);
    }
}

bool Region::intersect(const Vec3& point) const
{
    if (bspTree_.empty())
        return false;

    // Move point into local space
    auto local = worldToLocal(point);

    // Use the BSP tree to check if the point is inside or outside the region
    auto currentNode = 0;
    while (true)
    {
        switch (bspTree_[currentNode].plane.classify(local))
        {
            case Plane::Coincident:
            case Plane::Front:
                currentNode = bspTree_[currentNode].front;
                if (currentNode == -1)
                    return true;
                break;

            case Plane::Back:
                currentNode = bspTree_[currentNode].back;
                if (currentNode == -1)
                    return false;
                break;

            default:
                return false;
        }
    }
}

void Region::clear()
{
    triangles_.clear();
    bspTree_.clear();

    color_.setRGBA(1.0f, 1.0f, 1.0f, 0.25f);

    Entity::clear();

    // Regions are invisible by default
    setVisible(false);
}

void Region::save(FileWriter& file) const
{
    Entity::save(file);

    file.beginVersionedSection(RegionVersionInfo);
    file.write(triangles_, bspTree_);
    file.endVersionedSection();
}

void Region::load(FileReader& file)
{
    try
    {
        clear();

        Entity::load(file);

        file.beginVersionedSection(RegionVersionInfo);
        file.read(triangles_, bspTree_);
        file.endVersionedSection();
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

}
