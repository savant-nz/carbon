/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class SFXWaterGLSL : public Shader
{
public:

    class SFXWaterProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* cameraPosition = nullptr;
        ShaderConstant* reflectionRefractionProjection = nullptr;
        ShaderConstant* tilingFactor = nullptr;
        ShaderConstant* sNormalMap = nullptr;
        ShaderConstant* sReflectionMap = nullptr;
        ShaderConstant* sRefractionMap = nullptr;
        ShaderConstant* distortionFactors = nullptr;
        ShaderConstant* reflectionTint = nullptr;
        ShaderConstant* refractionTint = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(cameraPosition);
            CACHE_SHADER_CONSTANT(reflectionRefractionProjection);
            CACHE_SHADER_CONSTANT(tilingFactor);
            CACHE_SHADER_CONSTANT(sNormalMap);
            CACHE_SHADER_CONSTANT(sReflectionMap);
            CACHE_SHADER_CONSTANT(sRefractionMap);
            CACHE_SHADER_CONSTANT(distortionFactors);
            CACHE_SHADER_CONSTANT(reflectionTint);
            CACHE_SHADER_CONSTANT(refractionTint);
            CACHE_SHADER_CONSTANT(modelViewProjection);
        }
    };

    SFXWaterProgram program;

    SFXWaterGLSL() : Shader("SFXWater", 100, ShaderProgram::GLSL110) {}

    Shader::ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return RefractionReflection;
    }

    bool initialize() override { return program.setup(ShaderProgram::GLSL110, {"SFXWater.glsl.vert", "SFXWater.glsl.frag"}); }

    void uninitialize() override { program.clear(); }

    void enterShader() override
    {
        States::StateCacher::push();

        program.activate();

        program.sNormalMap->setInteger(0);
        program.sReflectionMap->setInteger(1);
        program.sRefractionMap->setInteger(2);
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, const ParameterArray& internalParams,
                         unsigned int pass, unsigned int sortKey) override
    {
        program.setVertexAttributeArrayConfiguration(geometryChunk);

        program.modelViewProjection->setMatrix4(renderer().getModelViewProjectionMatrix());
        program.cameraPosition->setFloat3(renderer().getLocalSpaceCameraPosition());
        program.reflectionRefractionProjection->setMatrix4(renderer().getScreenProjectionMatrix());
        program.tilingFactor->setFloat(params);

        setTexture(0, params[Parameter::normalMap], renderer().getFlatNormalMap());
        setTexture(1, renderer().getReflectionTexture());
        setTexture(2, renderer().getRefractionTexture());

        program.distortionFactors->setFloat2(params[Parameter::reflectionDistortion].getFloat(),
                                             params[Parameter::refractionDistortion].getFloat());
        program.reflectionTint->setFloat4(params);
        program.refractionTint->setFloat4(params);
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(SFXWaterGLSL, OpenGLBase)

}

}
