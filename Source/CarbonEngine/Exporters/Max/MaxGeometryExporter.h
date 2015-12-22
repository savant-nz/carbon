/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Exporters/Max/MaxPlugin.h"

namespace Carbon
{

namespace Max
{

/**
 * Helper class that exports geometry data from Max.
 */
class GeometryExporter
{
public:

    /**
     * Exports triangles into the passed triangle set.
     */
    static bool exportGeometry(TriangleArraySet& triangleSet, Runnable& r);

private:

    // Recurses through all scene nodes putting them into the nodes vector
    static bool gatherNodes(INode* node, Vector<INode*>& nodes, Runnable& r);

    // Appends all the triangles in the given geomobject node to the triangles array
    static bool exportGeomObject(INode* node, TriangleArraySet& triangleSet);
};

}

}
