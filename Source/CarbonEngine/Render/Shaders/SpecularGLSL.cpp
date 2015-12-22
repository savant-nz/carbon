/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Graphics/ShaderConstant.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/SpecularGLSL.h"

namespace Carbon
{

namespace Shaders
{

const Parameter SpecularGLSL::DefaultSpecularExponent(256.0f);

const auto specularExponentParameter = ParameterArray::Lookup("specularExponent");

float SpecularGLSL::getSpecularExponent(const ParameterArray& params)
{
    return params.get(specularExponentParameter, DefaultSpecularExponent).getFloat();
}

bool SpecularGLSL::isPresent(const ParameterArray& params, const ParameterArray& internalParams)
{
    return params.has(Parameter::normalMap) && (params.has(Parameter::specularColor) || params.has(Parameter::glossMap));
}

void SpecularGLSL::cache(Shader::ManagedShaderProgram* program_)
{
    CACHE_SHADER_CONSTANT(sGlossMap);
    CACHE_SHADER_CONSTANT(specularColor);
    CACHE_SHADER_CONSTANT(specularExponent);
}

void SpecularGLSL::enterShader(unsigned int textureUnit)
{
    sGlossMap->setInteger(textureUnit);
}

void SpecularGLSL::setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, unsigned int textureUnit)
{
    specularColor->setFloat3(params);
    specularExponent->setFloat(getSpecularExponent(params));

    Shader::setTexture(textureUnit, params[Parameter::glossMap], renderer().getWhiteTexture());
}

}

}
