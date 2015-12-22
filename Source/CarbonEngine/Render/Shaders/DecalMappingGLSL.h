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
 * Implementation of decal mapping for GLSL. This is a shader component that is used by other shaders in order to support decal
 * mapping.
 */
class CARBON_API DecalMapGLSL
{
public:

    /**
     * Returns whether decal mapping should be done under the given params. The params must specify a decal texture.
     */
    static bool isPresent(const ParameterArray& params, const ParameterArray& internalParams);

    /**
     * Prepares this shader compoment for use with the given shader program.
     */
    void cache(Shader::ManagedShaderProgram* program, bool isSpecularEnabled);

    /**
     * Sets up rendering for decal mapping.
     */
    void enterShader(unsigned int textureUnit1, unsigned int textureUnit2);

    /**
     * Updates the shader state ready for decal mapping rendering using the given geometry chunk and params.
     */
    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, unsigned int textureUnit1,
                         unsigned int textureUnit2);

private:

    ShaderConstant* sDecalMap = nullptr;
    ShaderConstant* sDecalGlossMap = nullptr;
};

}

}
