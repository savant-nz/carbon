/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class InternalDeferredLightingSurfaceGLSL : public Shader
{
public:

    static const auto PreprocessorDefineCount = 4U;

    class InternalDeferredLightingSurfaceProgram : public ManagedShaderProgram
    {
    public:

        InternalDeferredLightingSurfaceProgram(const std::array<bool, PreprocessorDefineCount>& enabledDefines)
            : doSkeletalAnimation(enabledDefines[0]),
              doAmbientOcclusion(enabledDefines[1]),
              doDecalMap(enabledDefines[2]),
              doParallaxMapping(enabledDefines[3])
        {
        }

        const bool doSkeletalAnimation;
        const bool doAmbientOcclusion;
        const bool doDecalMap;
        const bool doParallaxMapping;

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* sDiffuseMap = nullptr;
        ShaderConstant* sNormalMap = nullptr;
        ShaderConstant* sGlossMap = nullptr;
        ShaderConstant* sLightingTexture = nullptr;
        ShaderConstant* cameraPosition = nullptr;
        ShaderConstant* textureProjectionMatrix = nullptr;
        ShaderConstant* diffuseColor = nullptr;
        ShaderConstant* specularColor = nullptr;

        SkeletalAnimationGLSL<1, 4> skeletalAnimation;
        AmbientOcclusionGLSL ambientOcclusion;
        DecalMapGLSL decalMap;
        ParallaxMappingGLSL parallaxMapping;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(sDiffuseMap);
            CACHE_SHADER_CONSTANT(sGlossMap);
            CACHE_SHADER_CONSTANT(sLightingTexture);
            CACHE_SHADER_CONSTANT(textureProjectionMatrix);
            CACHE_SHADER_CONSTANT(diffuseColor);
            CACHE_SHADER_CONSTANT(specularColor);
            CACHE_SHADER_CONSTANT(modelViewProjection);

            if (doParallaxMapping)
            {
                CACHE_SHADER_CONSTANT(sNormalMap);
                CACHE_SHADER_CONSTANT(cameraPosition);
                parallaxMapping.cache(this);
            }

            if (doSkeletalAnimation)
                skeletalAnimation.cache(this);
            if (doAmbientOcclusion)
                ambientOcclusion.cache(this);
            if (doDecalMap)
                decalMap.cache(this, true);
        }
    };

    Vector<InternalDeferredLightingSurfaceProgram*> programs;
    InternalDeferredLightingSurfaceProgram* currentProgram = nullptr;

    InternalDeferredLightingSurfaceGLSL() : Shader("InternalDeferredLightingSurface", 100, ShaderProgram::GLSL110) {}

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
        return getShaderProgramIndex<SkeletalAnimationGLSL<1, 4>, AmbientOcclusionGLSL, DecalMapGLSL,
                                     ParallaxMappingGLSL>(params, internalParams);
    }

    InternalDeferredLightingSurfaceProgram* getProgram(unsigned int programIndex)
    {
        static const auto preprocessorDefines =
            std::array<String, PreprocessorDefineCount>{{"#define SKELETAL_ANIMATION", "#define AMBIENT_OCCLUSION",
                                                         "#define DECAL_MAPPING", "#define PARALLAX_MAPPING"}};

        return setupProgramCombination<InternalDeferredLightingSurfaceProgram>(
            programIndex, programs, preprocessorDefines, "InternalDeferredLightingSurface.glsl.vert",
            "InternalDeferredLightingSurface.glsl.frag");
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

            currentProgram->sDiffuseMap->setInteger(0);
            currentProgram->sGlossMap->setInteger(1);
            currentProgram->sLightingTexture->setInteger(2);

            if (currentProgram->doAmbientOcclusion)
                currentProgram->ambientOcclusion.enterShader(3);

            if (currentProgram->doDecalMap)
                currentProgram->decalMap.enterShader(4, 5);

            if (currentProgram->doParallaxMapping)
                currentProgram->sNormalMap->setInteger(6);
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

        setTexture(0, params[Parameter::diffuseMap], renderer().getErrorTexture());
        setTexture(1, params[Parameter::glossMap], renderer().getWhiteTexture());
        setTexture(2, renderer().getDeferredLightingTexture());

        currentProgram->textureProjectionMatrix->setMatrix4(renderer().getScreenProjectionMatrix());

        currentProgram->diffuseColor->setFloat3(params);

        currentProgram->specularColor->setFloat3(params);

        if (currentProgram->doSkeletalAnimation)
            currentProgram->skeletalAnimation.setShaderParams(geometryChunk, params, internalParams);

        if (currentProgram->doAmbientOcclusion)
            currentProgram->ambientOcclusion.setShaderParams(geometryChunk, params, 3);

        if (currentProgram->doDecalMap)
            currentProgram->decalMap.setShaderParams(geometryChunk, params, 4, 5);

        if (currentProgram->doParallaxMapping)
        {
            setTexture(6, params[Parameter::normalMap], renderer().getFlatNormalMap());
            currentProgram->cameraPosition->setFloat3(renderer().getLocalSpaceCameraPosition());
            currentProgram->parallaxMapping.setShaderParams(geometryChunk, params);
        }
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(InternalDeferredLightingSurfaceGLSL, OpenGLBase)

}

}
