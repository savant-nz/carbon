/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class InternalGeometryClipmappingGLSL : public Shader
{
public:

    class InternalGeometryClipmappingProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* sHeightfield = nullptr;
        ShaderConstant* clipmapValues = nullptr;
        ShaderConstant* scales = nullptr;
        ShaderConstant* clipmapOrigin = nullptr;
        ShaderConstant* clipmapCameraPosition = nullptr;
        ShaderConstant* sBaseMap = nullptr;
        ShaderConstant* sDetailMap = nullptr;
        ShaderConstant* sNormalMap = nullptr;
        ShaderConstant* lightColor = nullptr;
        ShaderConstant* lightAmbient = nullptr;
        ShaderConstant* lightDirection = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(sHeightfield);
            CACHE_SHADER_CONSTANT(clipmapValues);
            CACHE_SHADER_CONSTANT(scales);
            CACHE_SHADER_CONSTANT(clipmapOrigin);
            CACHE_SHADER_CONSTANT(clipmapCameraPosition);
            CACHE_SHADER_CONSTANT(sBaseMap);
            CACHE_SHADER_CONSTANT(sDetailMap);
            CACHE_SHADER_CONSTANT(sNormalMap);
            CACHE_SHADER_CONSTANT(lightColor);
            CACHE_SHADER_CONSTANT(lightAmbient);
            CACHE_SHADER_CONSTANT(lightDirection);
            CACHE_SHADER_CONSTANT(modelViewProjection);
        }
    };

    InternalGeometryClipmappingProgram program;

    InternalGeometryClipmappingGLSL() : Shader("InternalGeometryClipmapping", 100, ShaderProgram::GLSL110) {}

    bool hasHardwareSupport() const override
    {
        return graphics().isShaderLanguageSupported(ShaderProgram::GLSL110) &&
            graphics().getVertexShaderTextureUnitCount(ShaderProgram::GLSL110) >= 1 &&
            graphics().isPixelFormatSupported(Image::Red32f, GraphicsInterface::Texture2D);
    }

    bool initialize() override
    {
        return program.setup(ShaderProgram::GLSL110,
                             {"InternalGeometryClipmapping.glsl.vert", "InternalGeometryClipmapping.glsl.frag"});
    }

    void uninitialize() override { program.clear(); }

    void enterShader() override
    {
        States::StateCacher::push();

        program.activate();

        program.lightColor->setFloat4(renderer().getDirectionalLightColor());
        program.lightAmbient->setFloat4(renderer().getAmbientLightColor());

        program.sHeightfield->setInteger(0);
        program.sBaseMap->setInteger(1);
        program.sDetailMap->setInteger(2);
        program.sNormalMap->setInteger(3);
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                         const ParameterArray& internalParams, unsigned int pass, unsigned int sortKey) override
    {
        program.setVertexAttributeArrayConfiguration(geometryChunk);

        auto clipmapSize = params[Parameter::clipmapSize].getInteger();
        auto blendRegionSize = clipmapSize / 10;

        program.clipmapValues->setFloat3(1.0f / float(clipmapSize), 1.0f / float(blendRegionSize),
                                         float(clipmapSize / 2 - blendRegionSize - 1));
        program.scales->setFloat4(params);
        program.clipmapOrigin->setFloat3(params);
        program.clipmapCameraPosition->setFloat2(params);

        program.modelViewProjection->setMatrix4(renderer().getModelViewProjectionMatrix());
        program.lightDirection->setFloat3(renderer().getCurrentOrientationInverseMatrix() *
                                          renderer().getDirectionalLightDirection());

        setTexture(0, params[Parameter::heightfieldTexture]);
        setTexture(1, params[Parameter::baseMap], renderer().getErrorTexture());
        setTexture(2, params[Parameter::detailMap], renderer().getErrorTexture());
        setTexture(3, params[Parameter::normalMap], renderer().getFlatNormalMap());
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(InternalGeometryClipmappingGLSL, OpenGLBase)

}

}
