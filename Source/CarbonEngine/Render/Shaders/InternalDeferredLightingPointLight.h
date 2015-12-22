/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class InternalDeferredLightingPointLightGLSL : public Shader
{
public:

    static const auto PreprocessorDefineCount = 5U;

    class InternalDeferredLightingPointLightProgram : public ManagedShaderProgram
    {
    public:

        InternalDeferredLightingPointLightProgram(const std::array<bool, PreprocessorDefineCount>& enabledDefines)
            : doSpecular(enabledDefines[0]),
              doSpotLight(enabledDefines[1]),
              doProjectionTexture(enabledDefines[2]),
              doProjectionCubemap(enabledDefines[3]),
              doShadowMapping(enabledDefines[4])
        {
        }

        const bool doSpecular;
        const bool doSpotLight;
        const bool doProjectionTexture;
        const bool doProjectionCubemap;
        const bool doShadowMapping;

        ShaderConstant* sNormalsTexture = nullptr;
        ShaderConstant* sDepthTexture = nullptr;
        ShaderConstant* inverseViewProjectionMatrix = nullptr;
        ShaderConstant* lightPosition = nullptr;
        ShaderConstant* lightRadiusSquared = nullptr;
        ShaderConstant* lightColor = nullptr;
        ShaderConstant* sProjectionTexture = nullptr;
        ShaderConstant* sProjectionCubemap = nullptr;
        ShaderConstant* lightOrientationInverse = nullptr;
        ShaderConstant* spotlightConstants = nullptr;
        ShaderConstant* oneOverCosMaximumConeAngle = nullptr;
        ShaderConstant* lightDirection = nullptr;
        ShaderConstant* cameraPosition = nullptr;
        ShaderConstant* specularIntensity = nullptr;
        ShaderConstant* lightViewProjectionMatrix = nullptr;
        ShaderConstant* sShadowMap = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(sNormalsTexture);
            CACHE_SHADER_CONSTANT(sDepthTexture);
            CACHE_SHADER_CONSTANT(inverseViewProjectionMatrix);
            CACHE_SHADER_CONSTANT(lightPosition);
            CACHE_SHADER_CONSTANT(lightRadiusSquared);
            CACHE_SHADER_CONSTANT(lightColor);

            if (doSpecular)
            {
                CACHE_SHADER_CONSTANT(cameraPosition);
                CACHE_SHADER_CONSTANT(specularIntensity);
            }

            if (doSpotLight)
            {
                CACHE_SHADER_CONSTANT(spotlightConstants);
                CACHE_SHADER_CONSTANT(lightDirection);
            }

            if (doProjectionCubemap)
            {
                CACHE_SHADER_CONSTANT(sProjectionCubemap);
                CACHE_SHADER_CONSTANT(lightOrientationInverse);
            }

            if (doProjectionTexture)
                CACHE_SHADER_CONSTANT(sProjectionTexture);
            if (doShadowMapping)
                CACHE_SHADER_CONSTANT(sShadowMap);
            if (doProjectionTexture || doShadowMapping)
                CACHE_SHADER_CONSTANT(lightViewProjectionMatrix);
        }
    };

    Vector<InternalDeferredLightingPointLightProgram*> programs;
    InternalDeferredLightingPointLightProgram* currentProgram = nullptr;

    InternalDeferredLightingPointLightGLSL() : Shader("InternalDeferredLightingPointLight", 100, ShaderProgram::GLSL110) {}

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
        auto key = 0U;

        if (params[Parameter::isSpecularEnabled].getBoolean())
            key |= 1;
        if (params.has(Parameter::minimumConeAngle) && params.has(Parameter::maximumConeAngle))
            key |= 2;
        if (params.has(Parameter::projectionTexture) && params.has(Parameter::maximumConeAngle))
            key |= 4;
        if (params.has(Parameter::projectionCubemap))
            key |= 8;
        if (params.has(Parameter::shadowMap))
            key |= 16;

        return key;
    }

    InternalDeferredLightingPointLightProgram* getProgram(unsigned int programIndex)
    {
        static const auto preprocessorDefines =
            std::array<String, PreprocessorDefineCount>{{"#define SPECULAR", "#define SPOTLIGHT", "#define PROJECTION_TEXTURE",
                                                         "#define PROJECTION_CUBEMAP", "#define SHADOW_MAPPING"}};

        return setupProgramCombination<InternalDeferredLightingPointLightProgram>(
            programIndex, programs, preprocessorDefines, "UnitRectangle.glsl.vert",
            "InternalDeferredLightingPointLight.glsl.frag");
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

            currentProgram->sNormalsTexture->setInteger(0);
            currentProgram->sDepthTexture->setInteger(1);

            if (currentProgram->doProjectionTexture)
                currentProgram->sProjectionTexture->setInteger(2);

            if (currentProgram->doProjectionCubemap)
                currentProgram->sProjectionCubemap->setInteger(3);

            if (currentProgram->doShadowMapping)
                currentProgram->sShadowMap->setInteger(4);
        }

        return true;
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, const ParameterArray& internalParams,
                         unsigned int pass, unsigned int sortKey) override
    {
        if (!updateCurrentProgram(sortKey))
            return;

        currentProgram->setVertexAttributeArrayConfiguration(geometryChunk);

        setTexture(0, params[Parameter::normalMap], renderer().getFlatNormalMap());
        setTexture(1, params[Parameter::depthTexture]);

        if (currentProgram->doSpecular)
        {
            currentProgram->cameraPosition->setFloat3(renderer().getCamera().getPosition());
            currentProgram->specularIntensity->setFloat(params);
        }

        // Set the matrix that transforms from projection space to world space
        currentProgram->inverseViewProjectionMatrix->setMatrix4Inverse(renderer().getCamera().getProjectionMatrix() *
                                                                       renderer().getCamera().getViewMatrix());

        // Set light properties
        currentProgram->lightPosition->setFloat3(params);
        currentProgram->lightColor->setFloat3(params);
        currentProgram->lightRadiusSquared->setFloat(powf(params[Parameter::lightRadius].getFloat(), 2.0f));

        // Set the light orientation inverse if it is present in the shader
        if (currentProgram->doProjectionCubemap)
        {
            currentProgram->lightOrientationInverse->setMatrix3(
                params[Parameter::lightOrientation].getQuaternion().getInverse());
        }

        // Set the projection texture and cubemap if present in the shader
        if (currentProgram->doProjectionTexture)
            setTexture(2, params[Parameter::projectionTexture], renderer().getErrorTexture());
        if (currentProgram->doProjectionCubemap)
            setTexture(3, params[Parameter::projectionCubemap], renderer().getWhiteCubemapTexture());

        // Set the spotlight constants if present in the shader
        if (currentProgram->doSpotLight)
        {
            auto cosMinimumConeAngle = cosf(params[Parameter::minimumConeAngle].getFloat());
            auto cosMaximumConeAngle = cosf(params[Parameter::maximumConeAngle].getFloat());

            currentProgram->spotlightConstants->setFloat2(cosMinimumConeAngle,
                                                          1.0f / (cosMaximumConeAngle - cosMinimumConeAngle));

            currentProgram->lightDirection->setFloat3(params);
        }

        if (currentProgram->doShadowMapping)
            setTexture(4, params[Parameter::shadowMap]);

        if (currentProgram->doProjectionTexture || currentProgram->doShadowMapping)
            currentProgram->lightViewProjectionMatrix->setMatrix4(params);
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(InternalDeferredLightingPointLightGLSL, OpenGLBase)

}

}
