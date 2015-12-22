/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Game/Pathfinding/NavigationGraph.h"

namespace Carbon
{

/**
 * Automatically sets up a grid navigation graph with the given dimensions where every adjacent node in the grid is connected.
 * This class can create both 2D and 3D graphs.
 */
class CARBON_API GridNavigationGraph : public NavigationGraph
{
public:

    /**
     * Creates this navigation graph with the given dimensions.
     */
    GridNavigationGraph(unsigned int width, unsigned int height, unsigned int depth = 1, bool linkDiagonals = true);

    /**
     * Returns the width of this grid navigation graph.
     */
    unsigned int getWidth() const { return width_; }

    /**
     * Returns the height of this grid navigation graph.
     */
    unsigned int getHeight() const { return height_; }

    /**
     * Returns the depth of this grid navigation graph, will be 1 for 2D graphs.
     */
    unsigned int getDepth() const { return depth_; }

    /**
     * Returns the node at the given location in this grid navigation graph.
     */
    NavigationNode& getNode(unsigned int x, unsigned int y, unsigned int z = 0)
    {
        return NavigationGraph::getNode(z * (width_ * height_) + y * width_ + x);
    }

    /**
     * Sets whether the node at the given location in this given is traversable by using NavigationNode::setTraversable(). If \a
     * cutCorners is set to false then this method also makes the edges surrounding this node untraversable so that paths
     * through this graph won't use them. Doing this avoids the appearance of paths 'cutting corners' close to untraversable
     * nodes even though the generated path did not actually pass through that node.
     */
    void setTraversable(unsigned int x, unsigned int y, unsigned int z, bool traversable, bool cutCorners = false);

    /**
     * \copydoc setTraversable(unsigned int, unsigned int, unsigned int, bool, bool)
     */
    void setTraversable(unsigned int x, unsigned int y, bool traversable, bool cutCorners = false)
    {
        setTraversable(x, y, 0, traversable, cutCorners);
    }

private:

    const unsigned int width_ = 0;
    const unsigned int height_ = 0;
    const unsigned int depth_ = 0;
};

}
