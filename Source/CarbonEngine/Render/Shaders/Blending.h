/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

namespace Shaders
{

/**
 * Implementation of blending for all hardware. This is a shader component that is used by other shaders in order to
 * support blending.
 */
class CARBON_API Blending
{
public:

    /**
     * Returns whether blending would be enabled for the given set of parameters.
     */
    static bool isPresent(const ParameterArray& params);

    /**
     * Updates the shader state ready for blending using the given parameters.
     */
    static void setShaderParams(const ParameterArray& params);
};

}

}
