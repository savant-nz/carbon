/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Color.h"
#include "CarbonEngine/Math/SimpleTransform.h"

namespace Carbon
{

/**
 * Describes a navigation graph that can be used to carry out pathfinding on. This class should not be used directly,
 * one of the provided subclasses that assist with setting up common navigation graph shapes should be used, e.g.
 * GridNavigationGraph or HexagonalNavigationGraph.
 */
class CARBON_API NavigationGraph : private Noncopyable
{
public:

    /**
     * Clears the structure of this navigation graph.
     */
    void clear();

    /**
     * Returns the node closest to the given position. Node positions can be set with NavigationNode::setPosition().
     */
    const NavigationNode* getClosestNodeToPoint(const Vec3& p) const
    {
        auto index = getClosestNodeIndexToPoint(p);

        return index >= 0 ? nodes_[index] : nullptr;
    }

    /**
     * Returns the node closest to the given position. Node positions can be set with NavigationNode::setPosition().
     */
    NavigationNode* getClosestNodeToPoint(const Vec3& p)
    {
        auto index = getClosestNodeIndexToPoint(p);

        return index >= 0 ? nodes_[index] : nullptr;
    }

    /**
     * Returns the number of nodes in this navigation graph
     */
    unsigned int getNodeCount() { return nodes_.size(); }

    /**
     * Returns the navigation node at the specified index.
     */
    const NavigationNode& getNode(unsigned int index) const { return *nodes_[index]; }

    /**
     * Returns the navigation node at the specified index.
     */
    NavigationNode& getNode(unsigned int index) { return *nodes_[index]; }

    /**
     * Uses Scene::addImmediateGeometry() on the passed scene to create geometry that visualizes this graph and a single
     * path through it. If \a path is null then it will be skipped. All the geometry can be scaled, offset and colored.
     */
    void addToSceneImmediateGeometry(Scene& scene, const Vec3& offset, float scale, const Color& nodeColor,
                                     const Color& edgeColor, const Vector<NavigationNode*>* path = nullptr,
                                     const Color& pathColor = Color::White) const;

    /**
     * Creates a list of transforms for moving an object along the given path.
     */
    static Vector<SimpleTransform> createTransformsFromPath(const Vector<NavigationNode*>& path, float scale);

    /**
     * Logs the structure of this navigation graph to the debug output.
     */
    void debugTrace();

protected:

    NavigationGraph() {}
    ~NavigationGraph() { clear(); }

    /**
     * This method is used by subclasses to alter the navigation graph, it adds a single node to the nodes array.
     */
    NavigationNode& addNode();

    /**
     * This method is used by subclasses to alter the navigation graph, it adds an edge connecting the two passed nodes.
     */
    NavigationEdge& addEdge(NavigationNode* from, NavigationNode* to, float costToTraverse = 1.0f);

    /**
     * This method is used by subclasses to alter the navigation graph, it removes a single node from the nodes array
     * along with all edges connected to the node. Returns success flag.
     */
    bool removeNode(NavigationNode* node);

    /**
     * This method is used by subclasses to alter the navigation graph, it removes a single edge from the graph. Returns
     * success flag.
     */
    bool removeEdge(NavigationEdge* edge);

private:

    Vector<NavigationNode*> nodes_;
    Vector<NavigationEdge*> edges_;

    int getClosestNodeIndexToPoint(const Vec3& p) const;
};

}
