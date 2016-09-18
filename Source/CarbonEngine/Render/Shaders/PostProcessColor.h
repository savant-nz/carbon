/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class PostProcessColorGLSL : public Shader
{
public:

    class PostProcessColorProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* sInputTexture = nullptr;
        ShaderConstant* color = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(sInputTexture);
            CACHE_SHADER_CONSTANT(color);
        }
    };

    PostProcessColorProgram program;

    PostProcessColorGLSL() : Shader("PostProcessColor", 100, ShaderProgram::GLSL110) {}

    Shader::ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return PostProcess;
    }

    bool initialize() override
    {
        return program.setup(ShaderProgram::GLSL110, {"UnitRectangle.glsl.vert", "PostProcessColor.glsl.frag"});
    }

    void uninitialize() override { program.clear(); }

    void enterShader() override
    {
        States::StateCacher::push();
        program.activate();
        program.sInputTexture->setInteger(0);
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                         const ParameterArray& internalParams, unsigned int pass, unsigned int sortKey) override
    {
        program.setVertexAttributeArrayConfiguration(geometryChunk);

        program.color->setFloat4(params);

        setTexture(0, internalParams[Parameter::inputTexture]);
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(PostProcessColorGLSL, OpenGLBase)

}

}
