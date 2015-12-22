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
 * Normal mapping.
 */
class CARBON_API NormalMapping
{
public:

    /**
     * Given a set of shader parameters this method returns whether normal mapping should be activated.
     */
    static bool isPresent(const ParameterArray& params, const ParameterArray& internalParams)
    {
        return params.has(Parameter::normalMap);
    }
};

}

}
