/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Graphics/ShaderConstant.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/DecalMappingGLSL.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"

namespace Carbon
{

namespace Shaders
{

const auto decalMapParameter = ParameterArray::Lookup(Parameter::getHiddenParameterName("decalMap"));
const auto decalGlossMapParameter = ParameterArray::Lookup(Parameter::getHiddenParameterName("decalGlossMap"));

bool DecalMapGLSL::isPresent(const ParameterArray& params, const ParameterArray& internalParams)
{
    return params.has(decalMapParameter);
}

void DecalMapGLSL::cache(Shader::ManagedShaderProgram* program_, bool isSpecularEnabled)
{
    CACHE_SHADER_CONSTANT(sDecalMap);

    if (isSpecularEnabled)
        CACHE_SHADER_CONSTANT(sDecalGlossMap);
}

void DecalMapGLSL::enterShader(unsigned int textureUnit1, unsigned int textureUnit2)
{
    sDecalMap->setInteger(textureUnit1);
    sDecalGlossMap->setInteger(textureUnit2);
}

void DecalMapGLSL::setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                                   unsigned int textureUnit1, unsigned int textureUnit2)
{
    Shader::setTexture(textureUnit1, params[decalMapParameter], renderer().getErrorTexture());

    if (sDecalGlossMap)
        Shader::setTexture(textureUnit2, params[decalGlossMapParameter], renderer().getBlackTexture());
}

}

}
