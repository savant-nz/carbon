/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class InternalDeferredLightingDirectionalLightGLSL : public Shader
{
public:

    static const auto PreprocessorDefineCount = 2U;

    class InternalDeferredLightingDirectionalLightProgram : public ManagedShaderProgram
    {
    public:

        InternalDeferredLightingDirectionalLightProgram(const std::array<bool, PreprocessorDefineCount>& enabledDefines)
            : doShadowMapping(enabledDefines[0]), doSpecular(enabledDefines[1])
        {
        }

        const bool doShadowMapping;
        const bool doSpecular;

        ShaderConstant* sNormalsTexture = nullptr;
        ShaderConstant* sDepthTexture = nullptr;
        ShaderConstant* sShadowMap = nullptr;
        ShaderConstant* lightDirection = nullptr;
        ShaderConstant* lightColor = nullptr;
        ShaderConstant* cameraPosition = nullptr;
        ShaderConstant* inverseViewProjectionMatrix = nullptr;
        ShaderConstant* specularIntensity = nullptr;
        ShaderConstant* lightViewProjectionMatrix = nullptr;
        ShaderConstant* halfShadowMapTexelSize = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(sNormalsTexture);
            CACHE_SHADER_CONSTANT(lightDirection);
            CACHE_SHADER_CONSTANT(lightColor);

            if (doSpecular)
            {
                CACHE_SHADER_CONSTANT(cameraPosition);
                CACHE_SHADER_CONSTANT(specularIntensity);
            }

            if (doShadowMapping)
            {
                CACHE_SHADER_CONSTANT(lightViewProjectionMatrix);
                CACHE_SHADER_CONSTANT(sShadowMap);
                CACHE_SHADER_CONSTANT(halfShadowMapTexelSize);
            }

            if (doSpecular || doShadowMapping)
            {
                CACHE_SHADER_CONSTANT(sDepthTexture);
                CACHE_SHADER_CONSTANT(inverseViewProjectionMatrix);
            }
        }
    };

    Vector<InternalDeferredLightingDirectionalLightProgram*> programs;
    InternalDeferredLightingDirectionalLightProgram* currentProgram = nullptr;

    InternalDeferredLightingDirectionalLightGLSL()
        : Shader("InternalDeferredLightingDirectionalLight", 100, ShaderProgram::GLSL110)
    {
    }

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
        return (params.has(Parameter::shadowMap) ? 1 : 0) + (params[Parameter::isSpecularEnabled].getBoolean() ? 2 : 0);
    }

    InternalDeferredLightingDirectionalLightProgram* getProgram(unsigned int programIndex)
    {
        static const auto preprocessorDefines =
            std::array<String, PreprocessorDefineCount>{{"#define SHADOW_MAPPING", "#define SPECULAR"}};

        return setupProgramCombination<InternalDeferredLightingDirectionalLightProgram>(
            programIndex, programs, preprocessorDefines, "UnitRectangle.glsl.vert",
            "InternalDeferredLightingDirectionalLight.glsl.frag");
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

            if (currentProgram->doShadowMapping || currentProgram->doSpecular)
                currentProgram->sDepthTexture->setInteger(1);

            if (currentProgram->doShadowMapping)
                currentProgram->sShadowMap->setInteger(2);
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

        currentProgram->lightDirection->setFloat3(params);
        currentProgram->lightColor->setFloat3(params);

        if (currentProgram->doSpecular || currentProgram->doShadowMapping)
        {
            setTexture(1, params[Parameter::depthTexture]);

            // Set the matrix that transforms from projection space to world space
            currentProgram->inverseViewProjectionMatrix->setMatrix4Inverse(renderer().getCamera().getProjectionMatrix() *
                                                                           renderer().getCamera().getViewMatrix());
        }

        if (currentProgram->doSpecular)
        {
            currentProgram->cameraPosition->setFloat3(renderer().getCamera().getPosition());
            currentProgram->specularIntensity->setFloat(params);
        }

        if (currentProgram->doShadowMapping)
        {
            auto shadowMap = params[Parameter::shadowMap].getPointer<Texture>();
            setTexture(2, shadowMap);
            currentProgram->lightViewProjectionMatrix->setMatrix4(params);

            currentProgram->halfShadowMapTexelSize->setFloat(0.5f / shadowMap->getImage().getWidth());
        }
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(InternalDeferredLightingDirectionalLightGLSL, OpenGLBase)

}

}
