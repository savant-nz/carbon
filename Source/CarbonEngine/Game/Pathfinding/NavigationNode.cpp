/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Game/Pathfinding/NavigationEdge.h"
#include "CarbonEngine/Game/Pathfinding/NavigationNode.h"

namespace Carbon
{

const NavigationNode* NavigationNode::getNeighbor(unsigned int edgeIndex) const
{
    return edges_[edgeIndex]->getOtherNode(this);
}

NavigationNode* NavigationNode::getNeighbor(unsigned int edgeIndex)
{
    return edges_[edgeIndex]->getOtherNode(this);
}

NavigationEdge* NavigationNode::getEdgeToNeighbor(const NavigationNode& node) const
{
    return edges_.detect([&](const NavigationEdge* edge) { return edge->getOtherNode(this) == &node; }, nullptr);
}

void NavigationNode::onEdgeConstruct(NavigationEdge* edge)
{
    edges_.append(edge);
}

void NavigationNode::onEdgeDestruct(NavigationEdge* edge)
{
    assert(edges_.has(edge) && "Unknown edge");

    edges_.eraseValue(edge);
}

}
