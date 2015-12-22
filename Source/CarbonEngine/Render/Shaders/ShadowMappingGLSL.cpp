/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Graphics/ShaderConstant.h"
#include "CarbonEngine/Render/Shaders/ShadowMappingGLSL.h"

namespace Carbon
{

namespace Shaders
{

void ShadowMappingGLSL::cache(Shader::ManagedShaderProgram* program_)
{
    CACHE_SHADER_CONSTANT(lightViewProjectionMatrix);
    CACHE_SHADER_CONSTANT(sShadowMap);
}

bool ShadowMappingGLSL::isPresent(const ParameterArray& params, const ParameterArray& internalParams)
{
    return params.has(Parameter::shadowMap);
}

void ShadowMappingGLSL::enterShader(unsigned int textureUnit)
{
    sShadowMap->setInteger(textureUnit);
}

void ShadowMappingGLSL::setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                                        unsigned int textureUnit)
{
    Shader::setTexture(textureUnit, params[Parameter::shadowMap]);

    lightViewProjectionMatrix->setMatrix4(params);
}

}

}
