/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Game/Pathfinding/GridNavigationGraph.h"
#include "CarbonEngine/Game/Pathfinding/NavigationEdge.h"
#include "CarbonEngine/Game/Pathfinding/NavigationNode.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

GridNavigationGraph::GridNavigationGraph(unsigned int width, unsigned int height, unsigned int depth,
                                         bool linkDiagonals)
    : width_(width), height_(height), depth_(depth)
{
    // Add all the nodes
    for (auto z = 0U; z < depth; z++)
    {
        for (auto y = 0U; y < height; y++)
        {
            for (auto x = 0U; x < width; x++)
                addNode().setPosition(Vec3(float(x), float(y), float(z)));
        }
    }

    auto diagonalEdgeCostToTraverse = sqrtf(2.0f);

    // Add all the edges
    for (auto z = 0U; z < depth; z++)
    {
        for (auto y = 0U; y < height; y++)
        {
            for (auto x = 0U; x < width; x++)
            {
                // Immediate neighbors
                if (x + 1 < width_)
                    addEdge(&getNode(x, y, z), &getNode(x + 1, y, z));

                if (y + 1 < height_)
                    addEdge(&getNode(x, y, z), &getNode(x, y + 1, z));

                if (z + 1 < depth_)
                    addEdge(&getNode(x, y, z), &getNode(x, y, z + 1));

                if (linkDiagonals)
                {
                    // Diagonal neighbors
                    if (x + 1 < width_ && y + 1 < height_)
                        addEdge(&getNode(x, y, z), &getNode(x + 1, y + 1, z), diagonalEdgeCostToTraverse);

                    if (x + 1 < width_ && y > 0)
                        addEdge(&getNode(x, y, z), &getNode(x + 1, y - 1, z), diagonalEdgeCostToTraverse);
                }

                // Note: 3D diagonal neighbors aren't currently added
            }
        }
    }
}

void GridNavigationGraph::setTraversable(unsigned int x, unsigned int y, unsigned int z, bool traversable,
                                         bool cutCorners)
{
    auto& node = getNode(x, y, z);

    node.setTraversable(traversable);

    auto left = x > 0 ? &getNode(x - 1, y, z) : nullptr;
    auto right = x + 1 < width_ ? &getNode(x + 1, y, z) : nullptr;
    auto bottom = y > 0 ? &getNode(x, y - 1, z) : nullptr;
    auto top = y + 1 < height_ ? &getNode(x, y + 1, z) : nullptr;

    if (left)
    {
        if (top)
            left->getEdgeToNeighbor(*top)->setTraversable(cutCorners);
        if (bottom)
            left->getEdgeToNeighbor(*bottom)->setTraversable(cutCorners);
    }

    if (right)
    {
        if (top)
            right->getEdgeToNeighbor(*top)->setTraversable(cutCorners);
        if (bottom)
            right->getEdgeToNeighbor(*bottom)->setTraversable(cutCorners);
    }
}

}
