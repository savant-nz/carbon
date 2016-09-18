/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Game/Pathfinding/NavigationGraph.h"

namespace Carbon
{

/**
 * Automatically sets up a hexagonal navigation graph, the graph is indexable as a 2D grid however every second row is
 * moved half a unit in the X direction to create the hexagonal navigation shape.
 */
class CARBON_API HexagonalNavigationGraph : public NavigationGraph
{
public:

    /**
     * Creates this hexagonal navigation graph with the given dimensions.
     */
    HexagonalNavigationGraph(unsigned int width, unsigned int height);

    /**
     * Returns the width of this hexagonal navigation graph.
     */
    unsigned int getWidth() const { return width_; }

    /**
     * Returns the height of this hexagonal navigation graph.
     */
    unsigned int getHeight() const { return height_; }

    /**
     * Returns the node at the given location in this hexagonal navigation graph.
     */
    NavigationNode& getNode(unsigned int x, unsigned int y) { return NavigationGraph::getNode(y * width_ + x); }

private:

    const unsigned int width_ = 0;
    const unsigned int height_ = 0;
};

}
