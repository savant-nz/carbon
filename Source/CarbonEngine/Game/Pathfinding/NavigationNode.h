/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * Describes a single node in a navigation graph, a node can have any number of edges connected to it and has a position
 * associated with it.
 */
class CARBON_API NavigationNode
{
public:

    /**
     * Returns the list of edges directly connected to this node.
     */
    const Vector<NavigationEdge*>& getEdges() const { return edges_; }

    /**
     * Returns the neighbor of this node across the specified edge, \a edgeIndex should be less than the number of edges.
     */
    const NavigationNode* getNeighbor(unsigned int edgeIndex) const;

    /**
     * Returns the neighbor of this node across the specified edge, \a edgeIndex should be less than the number of edges.
     */
    NavigationNode* getNeighbor(unsigned int edgeIndex);

    /**
     * Returns the edge that connects this node to the passed neighbor node, if the passed node is not a direct neighbor of this
     * node then null is returned.
     */
    NavigationEdge* getEdgeToNeighbor(const NavigationNode& node) const;

    /**
     * Returns whether the passed node is a neighbor of this node.
     */
    bool hasNeighbor(const NavigationNode& node) const { return getEdgeToNeighbor(node) != nullptr; }

    /**
     * Returns the position of this navigation node.
     */
    const Vec3& getPosition() const { return position_; }

    /**
     * Sets the position of this navigation node.
     */
    void setPosition(const Vec3& position) { position_ = position; }

    /**
     * Returns whether this node is traversable by pathfinding algorithms, i.e. whether this node can be included in a path
     * through this graph.
     */
    bool isTraversable() const { return isTraversable_; }

    /**
     * Sets whether this node is traversable by pathfinding algorithms, i.e. whether this node can be included in a path through
     * this graph.
     */
    void setTraversable(bool traversable) { isTraversable_ = traversable; }

    /**
     * Returns the index of this node in the main array of nodes stored on the graph this node is part of
     */
    unsigned int getIndex() const { return index_; }

private:

    friend class NavigationGraph;
    friend class NavigationEdge;

    NavigationNode() {}

    Vector<NavigationEdge*> edges_;

    void onEdgeConstruct(NavigationEdge* edge);
    void onEdgeDestruct(NavigationEdge* edge);

    Vec3 position_;
    bool isTraversable_ = true;

    unsigned int index_ = 0;
};

}
