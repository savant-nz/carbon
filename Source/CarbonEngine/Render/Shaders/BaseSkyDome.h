/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class BaseSkyDomeGLSL : public Shader
{
public:

    class BaseSkyDomeProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* sDiffuseMap = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(modelViewProjection);
            CACHE_SHADER_CONSTANT(sDiffuseMap);
        }
    };

    BaseSkyDomeProgram program;

    BaseSkyDomeGLSL() : Shader("BaseSkyDome", 100, ShaderProgram::GLSL110) {}

    bool initialize() override
    {
        return program.setup(ShaderProgram::GLSL110, {"BaseSkyDome.glsl.vert", "BaseSkyDome.glsl.frag"});
    }

    void uninitialize() override { program.clear(); }

    void enterShader() override
    {
        States::StateCacher::push();

        program.activate();

        program.sDiffuseMap->setInteger(0);

        States::DepthTestEnabled = false;
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, const ParameterArray& internalParams,
                         unsigned int pass, unsigned int sortKey) override
    {
        States::DepthWriteEnabled = params.get(Parameter::depthWrite, true).getBoolean();

        program.modelViewProjection->setMatrix4(renderer().getModelViewProjectionMatrix());

        program.setVertexAttributeArrayConfiguration(geometryChunk);

        setTexture(0, params[Parameter::diffuseMap], renderer().getBlackCubemapTexture());
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(BaseSkyDomeGLSL, OpenGLBase)

}

}
