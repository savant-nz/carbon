/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class PostProcessPassThroughGLSL : public Shader
{
public:

    class PostProcessPassThroughProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* sInputTexture = nullptr;

        void cache() override { CACHE_SHADER_CONSTANT(sInputTexture); }
    };

    PostProcessPassThroughProgram program;

    PostProcessPassThroughGLSL() : Shader("PostProcessPassThrough", 100, ShaderProgram::GLSL110) {}

    Shader::ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return PostProcess;
    }

    bool initialize() override
    {
        return program.setup(ShaderProgram::GLSL110, {"UnitRectangle.glsl.vert", "PostProcessPassThrough.glsl.frag"});
    }

    void uninitialize() override { program.clear(); }

    void enterShader() override
    {
        States::StateCacher::push();
        program.activate();
        program.sInputTexture->setInteger(0);
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, const ParameterArray& internalParams,
                         unsigned int pass, unsigned int sortKey) override
    {
        program.setVertexAttributeArrayConfiguration(geometryChunk);

        setTexture(0, internalParams[Parameter::inputTexture]);
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(PostProcessPassThroughGLSL, OpenGLBase)

}

}
