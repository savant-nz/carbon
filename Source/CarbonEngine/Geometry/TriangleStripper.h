/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Runnable.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"

namespace Carbon
{

/**
 * Static class that wraps triangle strip generation.
 */
class TriangleStripper
{
public:

    /**
     * A primitive type with attached indices, a vector of these is returned by TriangleStripper::run().
     */
    typedef std::pair<GraphicsInterface::PrimitiveType, Vector<unsigned int>> PrimitiveWithIndices;

    /**
     * Generates triangle strips from the passed triangle list. Returns success flag.
     */
    static bool run(const Vector<unsigned int>& indices, Vector<PrimitiveWithIndices>& result, Runnable& r);
};

}
