/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class SFXBlurGLSL : public Shader
{
public:

    static const auto PreprocessorDefineCount = 1U;

    class SFXBlurProgram : public ManagedShaderProgram
    {
    public:

        SFXBlurProgram(const std::array<bool, PreprocessorDefineCount>& enabledDefines)
            : doSkeletalAnimation(enabledDefines[0])
        {
        }

        const bool doSkeletalAnimation;

        SkeletalAnimationGLSL<1, 4> skeletalAnimation;

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* screenProjection = nullptr;
        ShaderConstant* sRefractionMap = nullptr;
        ShaderConstant* diffuseColor = nullptr;
        ShaderConstant* textureOffsets = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(screenProjection);
            CACHE_SHADER_CONSTANT(sRefractionMap);
            CACHE_SHADER_CONSTANT(diffuseColor);
            CACHE_SHADER_CONSTANT(textureOffsets);
            CACHE_SHADER_CONSTANT(modelViewProjection);

            if (doSkeletalAnimation)
                skeletalAnimation.cache(this);
        }
    };

    Vector<SFXBlurProgram*> programs;
    SFXBlurProgram* currentProgram = nullptr;

    SFXBlurGLSL() : Shader("SFXBlur", 100, ShaderProgram::GLSL110) {}

    Shader::ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return Framebuffer;
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
        return getShaderProgramIndex<SkeletalAnimationGLSL<1, 4>>(params, internalParams);
    }

    SFXBlurProgram* getProgram(unsigned int programIndex)
    {
        static const auto preprocessorDefines =
            std::array<String, PreprocessorDefineCount>{{"#define SKELETAL_ANIMATION"}};

        return setupProgramCombination<SFXBlurProgram>(programIndex, programs, preprocessorDefines, "SFXBlur.glsl.vert",
                                                       "SFXBlur.glsl.frag");
    }

    bool updateCurrentProgram(unsigned int sortKey)
    {
        auto p = getProgram(sortKey);
        if (!p)
            return false;

        if (p != currentProgram)
        {
            currentProgram = p;
            currentProgram->activate();
            currentProgram->sRefractionMap->setInteger(0);
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
        currentProgram->screenProjection->setMatrix4(renderer().getScreenProjectionMatrix());

        setTexture(0, renderer().getRefractionTexture());

        currentProgram->diffuseColor->setFloat4(params);

        auto scale = params[Parameter::scale].getFloat() / 512.0f;
        auto textureOffsets = std::array<float, 12>{{
            -1.0f * scale, 1.0f * scale, 0.3f * scale, 0.5f * scale, -0.6f * scale, 0.2f * scale, 1.0f * scale,
            -0.2f * scale, -0.3f * scale, -0.7f * scale, 0.7f * scale, -1.0f * scale,
        }};
        currentProgram->textureOffsets->setArray(2, 6, textureOffsets.data());

        if (currentProgram->doSkeletalAnimation)
            currentProgram->skeletalAnimation.setShaderParams(geometryChunk, params, internalParams);
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(SFXBlurGLSL, OpenGLBase)

}

}
