/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Describes a single edge in a navigation graph, each connects two navigation nodes together and has an associated
 * traversal cost that is used when doing pathfinding.
 */
class CARBON_API NavigationEdge
{
public:

    /**
     * Sets up this edge with the given navigation nodes and traversal cost.
     */
    NavigationEdge(NavigationNode* node0, NavigationNode* node1, float costToTraverse);

    ~NavigationEdge();

    /**
     * Returns the nodes that are connected by this edge.
     */
    const NavigationNode* getNode(unsigned int index) const { return nodes_[index]; }

    /**
     * Given one of the nodes on this edge this method returns the other node.
     */
    const NavigationNode* getOtherNode(const NavigationNode* node) const
    {
        assert((node == nodes_[0] || node == nodes_[1]) && "Specified node is not on this edge");

        return nodes_[0] == node ? nodes_[1] : nodes_[0];
    }

    /**
     * Given one of the nodes on this edge this method returns the other node.
     */
    NavigationNode* getOtherNode(NavigationNode* node)
    {
        assert((node == nodes_[0] || node == nodes_[1]) && "Specified node is not on this edge");

        return nodes_[0] == node ? nodes_[1] : nodes_[0];
    }

    /**
     * Returns whether this edge is traversable by pathfinding algorithms, i.e. whether this edge can be used when
     * constructing a path through its graph.
     */
    bool isTraversable() const { return isTraversable_; }

    /**
     * Sets whether this edge is traversable by pathfinding algorithms, i.e. whether this edge can be used when
     * constructing a path through its graph.
     */
    void setTraversable(bool traversable) { isTraversable_ = traversable; }

    /**
     * Returns the cost of traversing this edge.
     */
    float getCostToTraverse() const { return costToTraverse_; }

    /**
     * Sets the cost of traversing this edge.
     */
    void setCostToTraverse(float cost) { costToTraverse_ = cost; }

    /**
     * Returns whether this edge is orphaned, an orphaned edge is one where one of both of the nodes it connects are
     * marked as not being traversable, which effectively means this edge isn't currently a useful part of the graph
     * structure in terms of connectivity. This property is orthogonal to the traversability of an edge.
     */
    bool isOrphaned() const;

private:

    friend class NavigationGraph;

    // The two nodes connected by this edge
    std::array<NavigationNode*, 2> nodes_ = {};

    bool isTraversable_ = true;
    float costToTraverse_ = 0.0f;
};

}
