/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class InternalDeferredLightingSetupGLSL : public Shader
{
public:

    static const auto PreprocessorDefineCount = 2U;

    class InternalDeferredLightingSetupProgram : public ManagedShaderProgram
    {
    public:

        InternalDeferredLightingSetupProgram(const std::array<bool, PreprocessorDefineCount>& enabledDefines)
            : doSkeletalAnimation(enabledDefines[0]), doParallaxMapping(enabledDefines[1])
        {
        }

        const bool doSkeletalAnimation;
        const bool doParallaxMapping;

        SkeletalAnimationGLSL<1, 4> skeletalAnimation;
        ParallaxMappingGLSL parallaxMapping;

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* sNormalMap = nullptr;
        ShaderConstant* sGlossMap = nullptr;
        ShaderConstant* cameraPosition = nullptr;
        ShaderConstant* objectOrientation = nullptr;
        ShaderConstant* specularExponent = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(sNormalMap);
            CACHE_SHADER_CONSTANT(sGlossMap);
            CACHE_SHADER_CONSTANT(objectOrientation);
            CACHE_SHADER_CONSTANT(specularExponent);
            CACHE_SHADER_CONSTANT(modelViewProjection);

            if (doParallaxMapping)
            {
                parallaxMapping.cache(this);
                CACHE_SHADER_CONSTANT(cameraPosition);
            }

            if (doSkeletalAnimation)
                skeletalAnimation.cache(this);
        }
    };

    Vector<InternalDeferredLightingSetupProgram*> programs;
    InternalDeferredLightingSetupProgram* currentProgram = nullptr;

    InternalDeferredLightingSetupGLSL() : Shader("InternalDeferredLightingSetup", 100, ShaderProgram::GLSL110) {}

    bool initialize() override
    {
        programs.resize(1 << PreprocessorDefineCount, nullptr);
        return true;
    }

    void uninitialize() override
    {
        while (!programs.empty())
            delete programs.popBack();
    }

    void precache() override
    {
        for (auto i = 0U; i < programs.size(); i++)
            getProgram(i);
    }

    void enterShader() override
    {
        States::StateCacher::push();
        currentProgram = nullptr;
    }

    unsigned int getSortKey(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return getShaderProgramIndex<SkeletalAnimationGLSL<1, 4>, ParallaxMappingGLSL>(params, internalParams);
    }

    InternalDeferredLightingSetupProgram* getProgram(unsigned int programIndex)
    {
        static const auto preprocessorDefines =
            std::array<String, PreprocessorDefineCount>{{"#define SKELETAL_ANIMATION", "#define PARALLAX_MAPPING"}};

        return setupProgramCombination<InternalDeferredLightingSetupProgram>(
            programIndex, programs, preprocessorDefines, "InternalDeferredLightingSetup.glsl.vert",
            "InternalDeferredLightingSetup.glsl.frag");
    }

    bool updateCurrentProgram(unsigned int sortKey)
    {
        auto p = getProgram(sortKey);
        if (!p)
            return false;

        if (currentProgram != p)
        {
            currentProgram = p;
            currentProgram->activate();

            currentProgram->sNormalMap->setInteger(0);
            currentProgram->sGlossMap->setInteger(1);
        }

        return true;
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                         const ParameterArray& internalParams, unsigned int pass, unsigned int sortKey) override
    {
        if (!updateCurrentProgram(sortKey))
            return;

        currentProgram->setVertexAttributeArrayConfiguration(geometryChunk);

        currentProgram->modelViewProjection->setMatrix4(renderer().getModelViewProjectionMatrix());
        currentProgram->objectOrientation->setMatrix3(renderer().getCurrentTransform().getOrientation());

        setTexture(0, params[Parameter::normalMap], renderer().getFlatNormalMap());
        setTexture(1, params[Parameter::glossMap], renderer().getWhiteTexture());

        currentProgram->specularExponent->setFloat(params);

        if (currentProgram->doSkeletalAnimation)
            currentProgram->skeletalAnimation.setShaderParams(geometryChunk, params, internalParams);

        if (currentProgram->doParallaxMapping)
        {
            currentProgram->cameraPosition->setFloat3(renderer().getLocalSpaceCameraPosition());
            currentProgram->parallaxMapping.setShaderParams(geometryChunk, params);
        }
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(InternalDeferredLightingSetupGLSL, OpenGLBase)

}

}
