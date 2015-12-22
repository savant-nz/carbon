/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Render/Shaders/Shader.h"

namespace Carbon
{

namespace Shaders
{

/**
 * Vertex coloring.
 */
class CARBON_API VertexColor
{
public:

    /**
     * Given a set of shader parameters this method returns whether vertex coloring should be activated.
     */
    static bool isPresent(const ParameterArray& params, const ParameterArray& internalParams)
    {
        return params[Parameter::useVertexColor].getBoolean();
    }
};

}

}
