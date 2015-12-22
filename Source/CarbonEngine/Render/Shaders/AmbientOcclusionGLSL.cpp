/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Graphics/ShaderConstant.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/AmbientOcclusionGLSL.h"

namespace Carbon
{

namespace Shaders
{

const auto ambientOcclusionMapParameter = ParameterArray::Lookup(Parameter::getHiddenParameterName("ambientOcclusionMap"));

bool AmbientOcclusionGLSL::isPresent(const ParameterArray& params, const ParameterArray& internalParams)
{
    return params.has(ambientOcclusionMapParameter);
}

void AmbientOcclusionGLSL::cache(Shader::ManagedShaderProgram* program_)
{
    CACHE_SHADER_CONSTANT(sAmbientOcclusionMap);
}

void AmbientOcclusionGLSL::enterShader(unsigned int textureUnit)
{
    sAmbientOcclusionMap->setInteger(textureUnit);
}

void AmbientOcclusionGLSL::setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                                           unsigned int textureUnit)
{
    Shader::setTexture(textureUnit, params[ambientOcclusionMapParameter], renderer().getBlackTexture());
}

}

}
