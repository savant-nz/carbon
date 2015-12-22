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
 * Implementation of ambient occlusion for GLSL. This is a shader component that is used by other GLSL shaders in order to
 * support ambient occlusion.
 */
class CARBON_API AmbientOcclusionGLSL
{
public:

    /**
     * Returns whether ambient occlusion should be done under the given params. The params must specify an ambient occlusion
     * texture.
     */
    static bool isPresent(const ParameterArray& params, const ParameterArray& internalParams);

    /**
     * Prepares this shader compoment for use with the given shader program.
     */
    void cache(Shader::ManagedShaderProgram* program);

    /**
     * Sets up rendering for ambient occlusion.
     */
    void enterShader(unsigned int textureUnit);

    /**
     * Updates the shader state ready for ambient occlusion rendering using the given geometry chunk and params.
     */
    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, unsigned int textureUnit);

private:

    ShaderConstant* sAmbientOcclusionMap = nullptr;
};

}

}
