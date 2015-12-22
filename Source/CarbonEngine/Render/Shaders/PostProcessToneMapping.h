/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class PostProcessToneMappingGLSL : public Shader
{
public:

    class PostProcessAverageSceneLuminanceProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* sInputTexture = nullptr;
        ShaderConstant* sPreviousAverageSceneLuminanceTexture = nullptr;
        ShaderConstant* weighting = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(sInputTexture);
            CACHE_SHADER_CONSTANT(sPreviousAverageSceneLuminanceTexture);
            CACHE_SHADER_CONSTANT(weighting);
        }
    };

    class PostProcessToneMappingProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* sInputTexture = nullptr;
        ShaderConstant* sAverageSceneLuminanceTexture = nullptr;
        ShaderConstant* exposure = nullptr;
        ShaderConstant* whitePoint = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(sInputTexture);
            CACHE_SHADER_CONSTANT(sAverageSceneLuminanceTexture);
            CACHE_SHADER_CONSTANT(exposure);
            CACHE_SHADER_CONSTANT(whitePoint);
        }
    };

    PostProcessAverageSceneLuminanceProgram averageSceneLuminanceProgram;
    PostProcessToneMappingProgram toneMappingProgram;

    std::array<Texture2D*, 2> averageSceneLuminanceTextures = {};

    PostProcessToneMappingGLSL() : Shader("PostProcessToneMapping", 100, ShaderProgram::GLSL110) {}

    Shader::ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return PostProcess;
    }

    bool hasHardwareSupport() const override
    {
        return graphics().isPixelFormatSupported(Image::RGB16f, GraphicsInterface::Texture2D) && Shader::hasHardwareSupport();
    }

    bool initialize() override
    {
        if (!averageSceneLuminanceProgram.setup(ShaderProgram::GLSL110,
                                                {"PostProcessToneMappingAverageSceneLuminance.glsl.vert",
                                                 "PostProcessToneMappingAverageSceneLuminance.glsl.frag"}))
            return false;

        if (!toneMappingProgram.setup(ShaderProgram::GLSL110, {"UnitRectangle.glsl.vert", "PostProcessToneMapping.glsl.frag"}))
            return false;

        // Create the two intermediate textures used to store the previous frame's luminance and the current frame's luminance,
        // these are combined in order to get smooth transitions when the scene's luminance changes
        for (auto i = 0U; i < 2; i++)
        {
            averageSceneLuminanceTextures[i] = textures().create2DTexture();

            auto image = Image();
            image.initialize(1, 1, 1, Image::RGB16f, false, 1);

            averageSceneLuminanceTextures[i]->load(String() + ".PostProcessToneMapping.AverageSceneLuminance." + i,
                                                   std::move(image));
            averageSceneLuminanceTextures[i]->upload();
        }

        return true;
    }

    void uninitialize() override
    {
        averageSceneLuminanceProgram.clear();
        toneMappingProgram.clear();

        for (auto& texture : averageSceneLuminanceTextures)
        {
            textures().releaseTexture(texture);
            texture = nullptr;
        }
    }

    void enterShader() override { States::StateCacher::push(); }

    unsigned int getPassCount(const ParameterArray& params, const ParameterArray& internalParams) const override { return 2; }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, const ParameterArray& internalParams,
                         unsigned int pass, unsigned int sortKey) override
    {
        if (pass == 0)
        {
            // Pass 1: Calculate the average luminance of the scene texture, weighted against the luminance used in the previous
            // frame, and put the result into a 1x1 texture.

            averageSceneLuminanceProgram.activate();

            averageSceneLuminanceProgram.setVertexAttributeArrayConfiguration(geometryChunk);

            averageSceneLuminanceProgram.sInputTexture->setInteger(0);
            averageSceneLuminanceProgram.sPreviousAverageSceneLuminanceTexture->setInteger(1);

            // Calculate the weighting to give to the scene's actual luminance based on the specified easing value and the
            // current frame rate
            averageSceneLuminanceProgram.weighting->setFloat(
                1.0f - powf(Math::clamp01(params[Parameter::easing].getFloat()), platform().getSecondsPassed()));

            setTexture(0, internalParams[Parameter::inputTexture]);
            setTexture(1, averageSceneLuminanceTextures[0]);

            renderer().setPostProcessIntermediateTargetTexture(averageSceneLuminanceTextures[1]);
        }
        else if (pass == 1)
        {
            // Pass 2: Do the actual tone mapping

            toneMappingProgram.activate();

            toneMappingProgram.setVertexAttributeArrayConfiguration(geometryChunk);

            toneMappingProgram.sInputTexture->setInteger(0);
            toneMappingProgram.sAverageSceneLuminanceTexture->setInteger(1);

            toneMappingProgram.exposure->setFloat(params);
            toneMappingProgram.whitePoint->setFloat(params);

            setTexture(0, internalParams[Parameter::inputTexture]);
            setTexture(1, averageSceneLuminanceTextures[1]);

            // Switch the two average scene luminance textures so that averaging based on the previous frame's luminance can be
            // done next frame
            auto t = averageSceneLuminanceTextures[0];
            averageSceneLuminanceTextures[0] = averageSceneLuminanceTextures[1];
            averageSceneLuminanceTextures[1] = t;
        }
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(PostProcessToneMappingGLSL, OpenGLBase)

}

}
