/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class SFXEdgeGLSL : public Shader
{
public:

    static const auto PreprocessorDefineCount = 1U;

    class SFXEdgeProgram : public ManagedShaderProgram
    {
    public:

        SFXEdgeProgram(const std::array<bool, PreprocessorDefineCount>& enabledDefines) : doSkeletalAnimation(enabledDefines[0])
        {
        }

        const bool doSkeletalAnimation;

        SkeletalAnimationGLSL<1, 4> skeletalAnimation;

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* cameraPosition = nullptr;
        ShaderConstant* sDiffuseMap = nullptr;
        ShaderConstant* sNormalMap = nullptr;
        ShaderConstant* sEdgeLookupMap = nullptr;
        ShaderConstant* diffuseColor = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(modelViewProjection);
            CACHE_SHADER_CONSTANT(cameraPosition);
            CACHE_SHADER_CONSTANT(sDiffuseMap);
            CACHE_SHADER_CONSTANT(sNormalMap);
            CACHE_SHADER_CONSTANT(sEdgeLookupMap);
            CACHE_SHADER_CONSTANT(diffuseColor);

            if (doSkeletalAnimation)
                skeletalAnimation.cache(this);
        }
    };

    Vector<SFXEdgeProgram*> programs;
    SFXEdgeProgram* currentProgram = nullptr;

    SFXEdgeGLSL() : Shader("SFXEdge", 100, ShaderProgram::GLSL110) {}

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
        return getShaderProgramIndex<SkeletalAnimationGLSL<1, 4>>(params, internalParams);
    }

    SFXEdgeProgram* getProgram(unsigned int programIndex)
    {
        static const auto preprocessorDefines = std::array<String, PreprocessorDefineCount>{{"#define SKELETAL_ANIMATION"}};

        return setupProgramCombination<SFXEdgeProgram>(programIndex, programs, preprocessorDefines, "SFXEdge.glsl.vert",
                                                       "SFXEdge.glsl.frag");
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
            currentProgram->sNormalMap->setInteger(1);
            currentProgram->sEdgeLookupMap->setInteger(2);
        }

        return true;
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, const ParameterArray& internalParams,
                         unsigned int pass, unsigned int sortKey) override
    {
        if (!updateCurrentProgram(sortKey))
            return;

        currentProgram->setVertexAttributeArrayConfiguration(geometryChunk);

        currentProgram->modelViewProjection->setMatrix4(renderer().getModelViewProjectionMatrix());
        currentProgram->cameraPosition->setFloat3(renderer().getLocalSpaceCameraPosition());
        currentProgram->diffuseColor->setFloat4(params);

        setTexture(0, params[Parameter::diffuseMap], renderer().getErrorTexture());
        setTexture(1, params[Parameter::normalMap], renderer().getFlatNormalMap());
        setTexture(2, params[Parameter::edgeLookupMap], renderer().getWhiteTexture());

        if (currentProgram->doSkeletalAnimation)
            currentProgram->skeletalAnimation.setShaderParams(geometryChunk, params, internalParams);

        Blending::setShaderParams(params);
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(SFXEdgeGLSL, OpenGLBase)

}

}
