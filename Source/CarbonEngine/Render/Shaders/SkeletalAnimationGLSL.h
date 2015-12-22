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
 * Implementation of skeletal animation for GLSL. This is a vertex shader component that is used by other shaders so that they
 * support skeletal animation.
 */
template <unsigned int MinimumWeightsPerVertex, unsigned int MaximumWeightsPerVertex> class CARBON_API SkeletalAnimationGLSL
{
public:

    /**
     * Given a set of shader parameters this method returns whether skeletal animation should be activated.
     */
    static bool isPresent(const ParameterArray& params, const ParameterArray& internalParams)
    {
        if (!internalParams.has(Parameter::boneCount))
            return false;

        auto weightsPerVertex = uint(internalParams[Parameter::weightsPerVertex].getInteger());

        return weightsPerVertex >= MinimumWeightsPerVertex && weightsPerVertex <= MaximumWeightsPerVertex;
    }

    /**
     * Prepares this shader compoment for use with the given shader program.
     */
    void cache(Shader::ManagedShaderProgram* program_) { CACHE_SHADER_CONSTANT(boneTransforms); }

    /**
     * Updates the shader state ready for skeletal animation rendering using the given geometry chunk and params.
     */
    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, const ParameterArray& internalParams)
    {
        auto boneCount = uint(internalParams[Parameter::boneCount].getInteger());
        auto data = internalParams[Parameter::boneTransforms].getPointer<float>();

        if (boneCount > 80)
        {
            LOG_WARNING << "Maximum bone count exceeded";
            return;
        }

        if (data)
            boneTransforms->setArray(4, boneCount * 3, data);
    }

private:

    ShaderConstant* boneTransforms = nullptr;
};

}

}
