/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class InternalFontGLSL : public Shader
{
public:

    class InternalFontProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* sDiffuseMap = nullptr;
        ShaderConstant* diffuseColor = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(modelViewProjection);
            CACHE_SHADER_CONSTANT(diffuseColor);
            CACHE_SHADER_CONSTANT(sDiffuseMap);
        }
    };

    InternalFontProgram program;

    InternalFontGLSL() : Shader("InternalFont", 100, ShaderProgram::GLSL110) {}

    bool initialize() override
    {
        return program.setup(ShaderProgram::GLSL110, {"InternalFont.glsl.vert", "InternalFont.glsl.frag"});
    }

    void uninitialize() override { program.clear(); }

    void enterShader() override
    {
        States::StateCacher::push();

        program.activate();
        program.sDiffuseMap->setInteger(0);

        States::BlendEnabled = true;
        States::BlendFunction = States::BlendFunctionSetup(States::SourceAlpha, States::OneMinusSourceAlpha);
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                         const ParameterArray& internalParams, unsigned int pass, unsigned int sortKey) override
    {
        if (pass == 0)
        {
            program.diffuseColor->setFloat4(params);
            program.setVertexAttributeArrayConfiguration(geometryChunk);
            setTexture(0, params[Parameter::diffuseMap]);
        }
        else
            program.modelViewProjection->setMatrix4(renderer().getModelViewProjectionMatrix());
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(InternalFontGLSL, OpenGLBase)

}

}
