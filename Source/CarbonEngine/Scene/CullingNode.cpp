/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Scene/Camera.h"
#include "CarbonEngine/Scene/CullingNode.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

const auto CullingNodeVersionInfo = VersionInfo(1, 1);

CullingNode::~CullingNode()
{
    onDestruct();
    clear();
}

void CullingNode::intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry)
{
    for (auto i = 0U; i < getChildCount(); i++)
        getChild(i)->setIsWorldGeometry(isWorldGeometry());

    // Swallow the ray intersection if it doesn't intersect the world extents
    if (!getWorldExtents().intersect(ray))
        return;

    ComplexEntity::intersectRay(ray, intersections, onlyWorldGeometry);
}

bool CullingNode::gatherGeometry(GeometryGather& gather)
{
    if (getScene()->is3D() && !gather.getFrustum().intersect(getWorldExtents()))
        return false;

    return ComplexEntity::gatherGeometry(gather);
}

void CullingNode::save(FileWriter& file) const
{
    // Save complex entity data
    ComplexEntity::save(file);

    // Write header
    file.beginVersionedSection(CullingNodeVersionInfo);

    file.write(41);

    file.endVersionedSection();
}

void CullingNode::load(FileReader& file)
{
    try
    {
        // Read complex entity data
        ComplexEntity::load(file);

        file.beginVersionedSection(CullingNodeVersionInfo);
        file.endVersionedSection();

        setIsWorldGeometry(true);
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

}
