/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Game/Pathfinding/HexagonalNavigationGraph.h"
#include "CarbonEngine/Game/Pathfinding/NavigationEdge.h"
#include "CarbonEngine/Game/Pathfinding/NavigationNode.h"

namespace Carbon
{

HexagonalNavigationGraph::HexagonalNavigationGraph(unsigned int width, unsigned int height)
    : width_(width), height_(height)
{
    // Add all the nodes
    for (auto y = 0U; y < height; y++)
    {
        for (auto x = 0U; x < width; x++)
            addNode().setPosition(Vec3(float(x + (y & 1 ? 0.5f : 0.0f)), float(y)));
    }

    // Add all the edges
    for (auto y = 0U; y < height; y++)
    {
        for (auto x = 0U; x < width; x++)
        {
            if (x + 1 < width_)
                addEdge(&getNode(x, y), &getNode(x + 1, y));

            if (y + 1 < height_)
                addEdge(&getNode(x, y), &getNode(x, y + 1));

            if (y & 1)
            {
                if (x + 1 < width_ && y + 1 < height_)
                    addEdge(&getNode(x, y), &getNode(x + 1, y + 1));
            }
            else
            {
                if (x > 0 && y + 1 < height_)
                    addEdge(&getNode(x, y), &getNode(x - 1, y + 1));
            }
        }
    }
}

}
