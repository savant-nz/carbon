/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class PostProcessDepthOfFieldGLSL : public Shader
{
public:

    class PostProcessDepthOfFieldProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* sSceneTexture = nullptr;
        ShaderConstant* sSceneBlurTexture = nullptr;
        ShaderConstant* sDepthTexture = nullptr;
        ShaderConstant* nearFarPlaneDistanceConstants = nullptr;
        ShaderConstant* focalLength = nullptr;
        ShaderConstant* focalRange = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(sSceneTexture);
            CACHE_SHADER_CONSTANT(sSceneBlurTexture);
            CACHE_SHADER_CONSTANT(sDepthTexture);
            CACHE_SHADER_CONSTANT(focalLength);
            CACHE_SHADER_CONSTANT(focalRange);
            CACHE_SHADER_CONSTANT(nearFarPlaneDistanceConstants);
        }
    };

    PostProcessDepthOfFieldProgram program;

    Shader* activeShader = nullptr;
    const Texture* blurredSceneTexture = nullptr;

    PostProcessDepthOfFieldGLSL() : Shader("PostProcessDepthOfField", 100, ShaderProgram::GLSL110) {}

    Shader::ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return PostProcess;
    }

    bool initialize() override
    {
        return program.setup(ShaderProgram::GLSL110, {"UnitRectangle.glsl.vert", "PostProcessDepthOfField.glsl.frag"});
    }

    void uninitialize() override { program.clear(); }

    void enterShader() override
    {
        States::StateCacher::push();

        activeShader = nullptr;

        blurredSceneTexture =
            renderer().requestTemporaryTexture(States::Viewport, true, false, TextureProperties::BilinearFilter);
    }

    unsigned int getPassCount(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return 2;
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                         const ParameterArray& internalParams, unsigned int pass, unsigned int sortKey) override
    {
        if (activeShader)
        {
            auto viewport = States::Viewport.get();
            auto renderTarget = States::RenderTarget.get();

            activeShader->exitShader();
            activeShader = nullptr;

            States::Viewport = viewport;
            States::RenderTarget = renderTarget;
        }

        if (pass == 0)
        {
            // Pass 1: Blur the scene texture, this uses the blur effect

            activeShader = effects().getEffectActiveShader("PostProcessBlur");
            if (!activeShader || !activeShader->setup())
                return;

            // Setup parameters for the blur, the input texture is the output from the previous pass
            auto newParams = params;
            newParams.set(Parameter::blurType, "2D");

            activeShader->enterShader();

            // Target the blurred scene intermediate texture
            renderer().setPostProcessIntermediateTargetTexture(blurredSceneTexture);

            activeShader->setShaderParams(geometryChunk, newParams, internalParams, 0, 0);
        }
        else if (pass == 1)
        {
            // Pass 2: Do depth of field by blurring between the full resolution scene texture and the downsampled and
            // blurred scene texture according to the depth at each pixel

            program.activate();

            program.setVertexAttributeArrayConfiguration(geometryChunk);

            program.sSceneTexture->setInteger(0);
            program.sSceneBlurTexture->setInteger(1);
            program.sDepthTexture->setInteger(2);

            setTexture(0, internalParams[Parameter::inputTexture]);
            setTexture(1, blurredSceneTexture);
            setTexture(2, internalParams[Parameter::depthTexture]);

            auto zNear = renderer().getCamera().getNearPlaneDistance();
            auto zFar = renderer().getCamera().getFarPlaneDistance();
            program.nearFarPlaneDistanceConstants->setFloat3(zNear * zFar, zFar, zFar - zNear);

            program.focalLength->setFloat(params);
            program.focalRange->setFloat(params);
        }
    }

    void exitShader() override
    {
        States::StateCacher::pop();

        renderer().releaseTemporaryTexture(blurredSceneTexture);
    }
};

CARBON_REGISTER_SHADER(PostProcessDepthOfFieldGLSL, OpenGLBase)

}

}
