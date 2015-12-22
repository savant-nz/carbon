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
 * Implementation of parallax mapping for GLSL. This is a shader component that is used by other shaders in order to support
 * parallax mapping.
 */
class CARBON_API ParallaxMappingGLSL
{
public:

    /**
     * Returns whether parallax mapping should be done under the given params. The params must specify parallax scale, step
     * count, and a normal map with height in the alpha.
     */
    static bool isPresent(const ParameterArray& params, const ParameterArray& internalParams);

    /**
     * Prepares this shader compoment for use with the given shader program.
     */
    void cache(Shader::ManagedShaderProgram* program);

    /**
     * Updates the shader state ready for parallax mapping using the given geometry chunk and params.
     */
    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params);

private:

    ShaderConstant* parallaxConstants = nullptr;
};

}

}
