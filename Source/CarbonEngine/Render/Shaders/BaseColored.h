/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class BaseColoredGLSL : public Shader
{
public:

    static const auto PreprocessorDefineCount = 1U;

    class BaseColoredProgram : public ManagedShaderProgram
    {
    public:

        BaseColoredProgram(const std::array<bool, PreprocessorDefineCount>& enabledDefines)
            : doSkeletalAnimation(enabledDefines[0])
        {
        }

        const bool doSkeletalAnimation;

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* diffuseColor = nullptr;

        SkeletalAnimationGLSL<1, 4> skeletalAnimation;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(modelViewProjection);
            CACHE_SHADER_CONSTANT(diffuseColor);

            if (doSkeletalAnimation)
                skeletalAnimation.cache(this);
        }
    };

    Vector<BaseColoredProgram*> programs;
    BaseColoredProgram* currentProgram = nullptr;

    BaseColoredGLSL() : Shader("BaseColored", 100, ShaderProgram::GLSL110) {}

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

    BaseColoredProgram* getProgram(unsigned int programIndex)
    {
        static const auto preprocessorDefines = std::array<String, PreprocessorDefineCount>{{"#define SKELETAL_ANIMATION"}};

        return setupProgramCombination<BaseColoredProgram>(programIndex, programs, preprocessorDefines, "BaseColored.glsl.vert",
                                                           "BaseColored.glsl.frag");
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
        currentProgram->diffuseColor->setFloat4(params);

        if (currentProgram->doSkeletalAnimation)
            currentProgram->skeletalAnimation.setShaderParams(geometryChunk, params, internalParams);

        Blending::setShaderParams(params);
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(BaseColoredGLSL, OpenGLBase)

}

}
