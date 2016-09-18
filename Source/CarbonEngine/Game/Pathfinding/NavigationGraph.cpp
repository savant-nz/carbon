/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Game/Pathfinding/NavigationEdge.h"
#include "CarbonEngine/Game/Pathfinding/NavigationGraph.h"
#include "CarbonEngine/Game/Pathfinding/NavigationNode.h"
#include "CarbonEngine/Math/Matrix3.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

void NavigationGraph::clear()
{
    while (!edges_.empty())
        delete edges_.popBack();

    while (!nodes_.empty())
        delete nodes_.popBack();
}

int NavigationGraph::getClosestNodeIndexToPoint(const Vec3& p) const
{
    if (nodes_.empty())
        return -1;

    auto index = 0;
    auto closestDistance = p.distance(nodes_[0]->getPosition());

    for (auto i = 1U; i < nodes_.size(); i++)
    {
        auto d = p.distance(nodes_[i]->getPosition());
        if (d < closestDistance)
        {
            closestDistance = d;
            index = i;
        }
    }

    return index;
}

void NavigationGraph::addToSceneImmediateGeometry(Scene& scene, const Vec3& offset, float scale, const Color& nodeColor,
                                                  const Color& edgeColor, const Vector<NavigationNode*>* path,
                                                  const Color& pathColor) const
{
    for (auto edge : edges_)
    {
        if (edge->isTraversable() && !edge->isOrphaned())
        {
            scene.addImmediateGeometry(edge->getNode(0)->getPosition() * scale + offset,
                                       edge->getNode(1)->getPosition() * scale + offset, edgeColor);
        }
    }

    for (auto node : nodes_)
    {
        if (!node->isTraversable())
            continue;

        scene.addImmediateGeometry(AABB(Vec3(-0.1f * scale), Vec3(0.1f * scale)),
                                   SimpleTransform(offset + node->getPosition() * scale), nodeColor);
    }

    if (path)
    {
        for (auto i = 1U; i < path->size(); i++)
        {
            scene.addImmediateGeometry((*path)[i - 1]->getPosition() * scale + offset,
                                       (*path)[i]->getPosition() * scale + offset, pathColor);
        }
    }
}

Vector<SimpleTransform> NavigationGraph::createTransformsFromPath(const Vector<NavigationNode*>& path, float scale)
{
    if (path.size() < 2)
        return {};

    auto transforms = Vector<SimpleTransform>();
    auto direction = Vec3();

    for (auto i = 0U; i < path.size(); i++)
    {
        if (i + 1 < path.size())
            direction = (path[i + 1]->getPosition() - path[i]->getPosition()).normalized();

        auto v0 = Vec3();
        auto v1 = Vec3();
        direction.constructBasis(v0, v1);

        auto orientation = Quaternion::createFromRotationMatrix(
            {v0.x, v1.x, direction.x, v0.y, v1.y, direction.y, v0.z, v1.z, direction.z});

        // Move to the new position
        transforms.emplace(path[i]->getPosition() * scale,
                           transforms.size() ? transforms.back().getOrientation() : orientation);

        // Rotate on the spot if needed
        if (i > 0 && transforms.back().getOrientation().getZVector().dot(direction) < 1.0f - Math::Epsilon)
            transforms.emplace(transforms.back().getPosition(), orientation);
    }

    return transforms;
}

NavigationNode& NavigationGraph::addNode()
{
    nodes_.append(new NavigationNode);
    nodes_.back()->index_ = nodes_.size() - 1;

    return *nodes_.back();
}

NavigationEdge& NavigationGraph::addEdge(NavigationNode* from, NavigationNode* to, float costToTraverse)
{
    edges_.append(new NavigationEdge(from, to, costToTraverse));

    return *edges_.back();
}

bool NavigationGraph::removeNode(NavigationNode* node)
{
    auto index = nodes_.find(node);
    if (index == -1)
        return false;

    // Delete all edges to this node
    for (auto edge : node->edges_)
        removeEdge(edge);

    assert(node->edges_.empty() && "Node's edges have not been fully removed");

    delete node;
    nodes_.erase(index);

    // Update the NavigationNode::index_ value on the nodes affected by this node removal
    for (auto i = uint(index); i < nodes_.size(); i++)
        nodes_[i]->index_ = i;

    return true;
}

bool NavigationGraph::removeEdge(NavigationEdge* edge)
{
    auto index = edges_.find(edge);
    if (index == -1)
        return false;

    // Remove edge from the list of edges on its two nodes
    edge->nodes_[0]->edges_.eraseValue(edge);
    edge->nodes_[1]->edges_.eraseValue(edge);

    delete edge;
    edges_.erase(index);

    return true;
}

void NavigationGraph::debugTrace()
{
    LOG_DEBUG << "Navigation graph node count: " << nodes_.size();
    for (auto i = 0U; i < nodes_.size(); i++)
        LOG_DEBUG << "    Node " << i << " is at position " << nodes_[i]->getPosition();

    LOG_DEBUG << "Navigation graph edge count: " << edges_.size();
    for (auto i = 0U; i < edges_.size(); i++)
    {
        LOG_DEBUG << "    Edge " << i << " connects node " << edges_[i]->getNode(0)->getIndex() << " to node "
                  << edges_[i]->getNode(1)->getIndex();
    }
}

}
