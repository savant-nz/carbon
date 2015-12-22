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
 * Implementation of shadow mapping for GLSL. This is a shader component that is used by other shaders in order to support
 * directional light shadow mapping.
 */
class CARBON_API ShadowMappingGLSL
{
public:

    /**
     * Prepares this shader compoment for use with the given shader program.
     */
    void cache(Shader::ManagedShaderProgram* program_);

    /**
     * Given a set of shader parameters this method returns whether skeletal animation should be activated.
     */
    static bool isPresent(const ParameterArray& params, const ParameterArray& internalParams);

    /**
     * Sets up rendering for shadow mapping.
     */
    void enterShader(unsigned int textureUnit);

    /**
     * Updates the shader state ready for shadow map rendering using the given geometry chunk and params.
     */
    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, unsigned int textureUnit);

private:

    ShaderConstant* lightViewProjectionMatrix = nullptr;
    ShaderConstant* sShadowMap = nullptr;
};

}

}
