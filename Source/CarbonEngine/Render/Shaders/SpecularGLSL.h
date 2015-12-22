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
 * Implementation of specular for GLSL. This is a shader component that is used by other shaders in order to support specular.
 */
class CARBON_API SpecularGLSL
{
public:

    /**
     * The specular exponent parameter to use when none is specified, currently 256. Note that the specular exponent is
     * modulated by the alpha channel of the gloss map prior to being used in the specular calculation.
     */
    static const Parameter DefaultSpecularExponent;

    /**
     * Returns the specular exponent to use given a set of parameters. If \a parameters contains a specularExponent value then
     * that will be returned, if not then DefaultSpecularExponent will be returned.
     */
    static float getSpecularExponent(const ParameterArray& params);

    /**
     * Given a set of shader parameters this method returns whether specular lighting should be activated.
     */
    static bool isPresent(const ParameterArray& params, const ParameterArray& internalParams);

    /**
     * Prepares this shader compoment for use with the given shader program.
     */
    void cache(Shader::ManagedShaderProgram* program);

    /**
     * Sets up rendering for specular.
     */
    void enterShader(unsigned int textureUnit);

    /**
     * Updates the shader state ready for specular rendering using the given geometry chunk and params.
     */
    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, unsigned int textureUnit);

private:

    ShaderConstant* sGlossMap = nullptr;
    ShaderConstant* specularColor = nullptr;
    ShaderConstant* specularExponent = nullptr;
};

}

}
