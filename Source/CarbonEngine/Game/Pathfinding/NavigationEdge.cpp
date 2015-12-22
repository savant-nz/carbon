/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Game/Pathfinding/NavigationEdge.h"
#include "CarbonEngine/Game/Pathfinding/NavigationNode.h"

namespace Carbon
{

NavigationEdge::NavigationEdge(NavigationNode* node0, NavigationNode* node1, float costToTraverse)
    : nodes_{{node0, node1}}, costToTraverse_(costToTraverse)
{
    assert(node0 && node1 && "Edge nodes must be specified");

    nodes_[0]->onEdgeConstruct(this);
    nodes_[1]->onEdgeConstruct(this);
}

NavigationEdge::~NavigationEdge()
{
    nodes_[0]->onEdgeDestruct(this);
    nodes_[1]->onEdgeDestruct(this);
}

bool NavigationEdge::isOrphaned() const
{
    return !nodes_[0]->isTraversable() || !nodes_[1]->isTraversable();
}

}
