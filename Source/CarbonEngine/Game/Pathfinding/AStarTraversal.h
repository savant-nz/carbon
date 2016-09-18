/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Provides an implementation of the A* graph search algorithm for use on a NavigationGraph.
 */
class CARBON_API AStarTraversal
{
public:

    /**
     * Runs the A* algorithm on the passed graph with passed start and target nodes. The calculated path is returned in
     * \a path. The return value indicates whether a path between the two nodes was found.
     */
    static bool run(NavigationGraph& graph, NavigationNode& start, NavigationNode& target,
                    Vector<NavigationNode*>& path);
};

}
