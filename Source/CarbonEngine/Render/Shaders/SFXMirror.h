/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class SFXMirrorGLSL : public Shader
{
public:

    class SFXMirrorProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* cameraPosition = nullptr;
        ShaderConstant* screenProjection = nullptr;
        ShaderConstant* sDiffuseMap = nullptr;
        ShaderConstant* sReflectanceMap = nullptr;
        ShaderConstant* sReflectionMap = nullptr;
        ShaderConstant* diffuseColor = nullptr;
        ShaderConstant* reflectionColor = nullptr;
        ShaderConstant* fresnelExponent = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(modelViewProjection);
            CACHE_SHADER_CONSTANT(cameraPosition);
            CACHE_SHADER_CONSTANT(screenProjection);
            CACHE_SHADER_CONSTANT(sDiffuseMap);
            CACHE_SHADER_CONSTANT(sReflectanceMap);
            CACHE_SHADER_CONSTANT(sReflectionMap);
            CACHE_SHADER_CONSTANT(diffuseColor);
            CACHE_SHADER_CONSTANT(reflectionColor);
            CACHE_SHADER_CONSTANT(fresnelExponent);
        }
    };

    SFXMirrorProgram program;

    SFXMirrorGLSL() : Shader("SFXMirror", 100, ShaderProgram::GLSL110) {}

    Shader::ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return Reflection;
    }

    bool initialize() override { return program.setup(ShaderProgram::GLSL110, {"SFXMirror.glsl.vert", "SFXMirror.glsl.frag"}); }

    void uninitialize() override { program.clear(); }

    void enterShader() override
    {
        States::StateCacher::push();

        program.activate();

        program.sDiffuseMap->setInteger(0);
        program.sReflectanceMap->setInteger(1);
        program.sReflectionMap->setInteger(2);
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, const ParameterArray& internalParams,
                         unsigned int pass, unsigned int sortKey) override
    {
        program.setVertexAttributeArrayConfiguration(geometryChunk);

        program.modelViewProjection->setMatrix4(renderer().getModelViewProjectionMatrix());
        program.cameraPosition->setFloat3(renderer().getLocalSpaceCameraPosition());
        program.screenProjection->setMatrix4(renderer().getScreenProjectionMatrix());

        program.diffuseColor->setFloat4(params);
        program.reflectionColor->setFloat4(params);
        program.fresnelExponent->setFloat(params);

        setTexture(0, params[Parameter::diffuseMap], renderer().getErrorTexture());
        setTexture(1, params[Parameter::reflectanceMap], renderer().getWhiteTexture());
        setTexture(2, renderer().getReflectionTexture());
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(SFXMirrorGLSL, OpenGLBase)

}

}
