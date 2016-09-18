/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Graphics/ShaderConstant.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Render/Shaders/ParallaxMappingGLSL.h"
#include "CarbonEngine/Render/Texture/Texture.h"

namespace Carbon
{

namespace Shaders
{

const auto parallaxScaleParameter = ParameterArray::Lookup("parallaxScale");
const auto parallaxStepCountParameter = ParameterArray::Lookup("parallaxStepCount");

bool ParallaxMappingGLSL::isPresent(const ParameterArray& params, const ParameterArray& internalParams)
{
    return params.has(parallaxScaleParameter) && params.has(parallaxStepCountParameter) &&
        params.has(Parameter::normalMap);
}

void ParallaxMappingGLSL::cache(Shader::ManagedShaderProgram* program_)
{
    CACHE_SHADER_CONSTANT(parallaxConstants);
}

void ParallaxMappingGLSL::setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params)
{
    // The size of the height map influences the amount of sampling that is done in the parallax fragment shader

    auto texelsAcrossDiagonalOfHeightMap = 1.0f;

    auto normalAndHeightMap = params[Parameter::normalMap].getPointer<Texture>();
    if (normalAndHeightMap)
    {
        normalAndHeightMap->ensureImageIsLoaded();

        texelsAcrossDiagonalOfHeightMap = sqrtf(powf(float(normalAndHeightMap->getImage().getWidth()), 2.0f) +
                                                powf(float(normalAndHeightMap->getImage().getHeight()), 2.0f));
    }

    parallaxConstants->setFloat3(params[parallaxScaleParameter].getFloat(),
                                 params[parallaxStepCountParameter].getFloat(), texelsAcrossDiagonalOfHeightMap);
}

}

}
