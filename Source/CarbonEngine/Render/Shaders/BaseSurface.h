/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class BaseSurfaceGLSL : public Shader
{
public:

    static const auto PreprocessorDefineCount = 8U;

    class BaseSurfaceProgram : public ManagedShaderProgram
    {
    public:

        BaseSurfaceProgram(const std::array<bool, PreprocessorDefineCount>& enabledDefines)
            : doSkeletalAnimation(enabledDefines[0] || enabledDefines[1]),
              doAmbientOcclusion(enabledDefines[2]),
              doDecalMap(enabledDefines[3]),
              doParallaxMapping(enabledDefines[4]),
              doSpecular(enabledDefines[5]),
              doNormalMapping(enabledDefines[6]),
              doVertexColor(enabledDefines[7])
        {
        }

        const bool doSkeletalAnimation;
        const bool doAmbientOcclusion;
        const bool doDecalMap;
        const bool doParallaxMapping;
        const bool doSpecular;
        const bool doNormalMapping;
        const bool doVertexColor;

        SkeletalAnimationGLSL<1, 4> skeletalAnimation;
        AmbientOcclusionGLSL ambientOcclusion;
        DecalMapGLSL decalMap;
        ParallaxMappingGLSL parallaxMapping;
        SpecularGLSL specular;

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* lightDirection = nullptr;
        ShaderConstant* cameraPosition = nullptr;
        ShaderConstant* sDiffuseMap = nullptr;
        ShaderConstant* sNormalMap = nullptr;
        ShaderConstant* lightColor = nullptr;
        ShaderConstant* lightAmbient = nullptr;
        ShaderConstant* diffuseColor = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(modelViewProjection);
            CACHE_SHADER_CONSTANT(sDiffuseMap);
            CACHE_SHADER_CONSTANT(lightColor);
            CACHE_SHADER_CONSTANT(lightAmbient);
            CACHE_SHADER_CONSTANT(diffuseColor);

            if (doNormalMapping || doSpecular)
            {
                CACHE_SHADER_CONSTANT(sNormalMap);
                CACHE_SHADER_CONSTANT(lightDirection);
            }

            if (doSpecular || doParallaxMapping)
                CACHE_SHADER_CONSTANT(cameraPosition);
            if (doSkeletalAnimation)
                skeletalAnimation.cache(this);
            if (doAmbientOcclusion)
                ambientOcclusion.cache(this);
            if (doDecalMap)
                decalMap.cache(this, doSpecular);
            if (doParallaxMapping)
                parallaxMapping.cache(this);
            if (doSpecular)
                specular.cache(this);
        }
    };

    Vector<BaseSurfaceProgram*> programs;
    BaseSurfaceProgram* currentProgram = nullptr;

    BaseSurfaceGLSL() : Shader("BaseSurface", 100, ShaderProgram::GLSL110) {}

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
        return getShaderProgramIndex<SkeletalAnimationGLSL<1, 2>, SkeletalAnimationGLSL<3, 4>, AmbientOcclusionGLSL,
                                     DecalMapGLSL, ParallaxMappingGLSL, SpecularGLSL, NormalMapping, VertexColor>(
            params, internalParams);
    }

    BaseSurfaceProgram* getProgram(unsigned int programIndex)
    {
        static const auto preprocessorDefines = std::array<String, PreprocessorDefineCount>{
            {"#define SKELETAL_ANIMATION\n#define WEIGHTS_PER_VERTEX 2",
             "#define SKELETAL_ANIMATION\n#undef WEIGHTS_PER_VERTEX\n#define WEIGHTS_PER_VERTEX 4",
             "#define AMBIENT_OCCLUSION", "#define DECAL_MAPPING", "#define PARALLAX_MAPPING", "#define SPECULAR",
             "#define NORMAL_MAPPING", "#define VERTEX_COLOR"}};

        return setupProgramCombination<BaseSurfaceProgram>(programIndex, programs, preprocessorDefines,
                                                           "BaseSurface.glsl.vert", "BaseSurface.glsl.frag");
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

            currentProgram->lightColor->setFloat4(renderer().getDirectionalLightColor());
            currentProgram->lightAmbient->setFloat4(renderer().getAmbientLightColor());

            currentProgram->sDiffuseMap->setInteger(0);

            if (currentProgram->doNormalMapping || currentProgram->doSpecular)
                currentProgram->sNormalMap->setInteger(1);

            if (currentProgram->doSpecular)
                currentProgram->specular.enterShader(2);
            if (currentProgram->doAmbientOcclusion)
                currentProgram->ambientOcclusion.enterShader(3);
            if (currentProgram->doDecalMap)
                currentProgram->decalMap.enterShader(4, 5);
        }

        return true;
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                         const ParameterArray& internalParams, unsigned int pass, unsigned int sortKey) override
    {
        if (!updateCurrentProgram(sortKey))
            return;

        currentProgram->setVertexAttributeArrayConfiguration(geometryChunk);

        setTexture(0, params[Parameter::diffuseMap], renderer().getErrorTexture());

        currentProgram->modelViewProjection->setMatrix4(renderer().getModelViewProjectionMatrix());
        currentProgram->diffuseColor->setFloat4(params);

        if (currentProgram->doSpecular || currentProgram->doParallaxMapping)
            currentProgram->cameraPosition->setFloat3(renderer().getLocalSpaceCameraPosition());

        if (currentProgram->doNormalMapping || currentProgram->doSpecular)
        {
            currentProgram->lightDirection->setFloat3(renderer().getCurrentOrientationInverseMatrix() *
                                                      renderer().getDirectionalLightDirection());

            setTexture(1, params[Parameter::normalMap], renderer().getFlatNormalMap());
        }

        if (currentProgram->doSpecular)
            currentProgram->specular.setShaderParams(geometryChunk, params, 2);

        if (currentProgram->doSkeletalAnimation)
            currentProgram->skeletalAnimation.setShaderParams(geometryChunk, params, internalParams);

        if (currentProgram->doAmbientOcclusion)
            currentProgram->ambientOcclusion.setShaderParams(geometryChunk, params, 3);

        if (currentProgram->doDecalMap)
            currentProgram->decalMap.setShaderParams(geometryChunk, params, 4, 5);

        if (currentProgram->doParallaxMapping)
            currentProgram->parallaxMapping.setShaderParams(geometryChunk, params);

        Blending::setShaderParams(params);
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(BaseSurfaceGLSL, OpenGLBase)

}

}
