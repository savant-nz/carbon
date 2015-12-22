/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Game/Pathfinding/AStarTraversal.h"
#include "CarbonEngine/Game/Pathfinding/NavigationEdge.h"
#include "CarbonEngine/Game/Pathfinding/NavigationGraph.h"
#include "CarbonEngine/Game/Pathfinding/NavigationNode.h"

namespace Carbon
{

// This helper class is used by the A star algorithm.
class AStarDetails
{
public:

    NavigationNode* parentNode = nullptr;

    bool inOpenSet = false;
    bool closed = false;

    float getF() const { return F; }
    float getG() const { return G; }
    float getH() const { return H; }

    void setG(float g)
    {
        G = g;
        updateF();
    }

    void setH(float h)
    {
        H = h;
        updateF();
    }

private:

    float G = 0.0f;
    float H = 0.0f;
    float F = 0.0f;

    void updateF()
    {
        if (G == FLT_MAX)
            F = FLT_MAX;
        else
            F = G + H;
    }
};

bool AStarTraversal::run(NavigationGraph& graph, NavigationNode& start, NavigationNode& target, Vector<NavigationNode*>& path)
{
    auto details = Vector<AStarDetails>(graph.getNodeCount());

    auto openSet = Vector<NavigationNode*>();

    openSet.append(&start);
    details[openSet.back()->getIndex()].inOpenSet = true;
    details[openSet.back()->getIndex()].setH(start.getPosition().distance(target.getPosition()));

    while (openSet.size() != 0)
    {
        // Find the node with best F score
        auto bestNodeIndex = 0U;
        for (auto i = 1U; i < openSet.size(); i++)
        {
            if (details[openSet[i]->getIndex()].getF() < details[openSet[bestNodeIndex]->getIndex()].getF())
                bestNodeIndex = i;
        }

        auto bestNode = openSet[bestNodeIndex];

        // If we have reached the target then finish
        if (bestNode == &target)
        {
            path = Vector<NavigationNode*>(1, bestNode);

            while (path[0] != &start)
                path.prepend(details[path[0]->getIndex()].parentNode);

            return true;
        }

        // Move best node from open set to closed set
        openSet.erase(bestNodeIndex);
        details[bestNode->getIndex()].inOpenSet = false;
        details[bestNode->getIndex()].closed = true;

        // Loop over all the neighbors of the best node
        for (auto i = 0U; i < bestNode->getEdges().size(); i++)
        {
            auto neighborIndex = bestNode->getNeighbor(i)->getIndex();

            // If this neighbor is already in the closed list then skip it
            if (details[neighborIndex].closed)
                continue;

            auto neighbor = bestNode->getNeighbor(i);
            auto edge = bestNode->getEdges()[i];

            // Check the neighbour is traversable, if not then we can't even consider moving to it
            if (!neighbor->isTraversable() || !edge->isTraversable())
                continue;

            // Get the edge cost
            auto edgeCost = edge->getCostToTraverse();

            // Calculate G score
            auto gScore = details[neighborIndex].getG() + edgeCost;

            auto useNewGScore = true;

            // If this neighbor isn't in the open set then add it
            if (!details[neighborIndex].inOpenSet)
            {
                openSet.append(neighbor);
                details[neighborIndex].inOpenSet = true;
            }
            else if (gScore >= details[neighborIndex].getG())
            {
                // If this path to the neighbor is longer than an already known path then don't use it
                useNewGScore = false;
            }

            if (useNewGScore)
            {
                details[neighborIndex].parentNode = bestNode;
                details[neighborIndex].setG(gScore);
                details[neighborIndex].setH(neighbor->getPosition().distance(target.getPosition()));
            }
        }
    }

    return false;
}

}
