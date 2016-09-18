/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class PostProcessBloomGLSL : public Shader
{
public:

    std::array<const Texture*, 3> bloomTextures = {};
    Shader* activeShader = nullptr;

    PostProcessBloomGLSL() : Shader("PostProcessBloom", 100, ShaderProgram::GLSL110) {}

    Shader::ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return PostProcess;
    }

    void enterShader() override
    {
        States::StateCacher::push();

        activeShader = nullptr;

        if (renderer().isHDREnabled())
        {
            // Intermediate bloom textures are 1/16th the area of the viewport
            auto rect =
                Rect(0.0f, 0.0f, States::Viewport.get().getWidth() / 4.0f, States::Viewport.get().getHeight() / 4.0f);

            bloomTextures[0] = renderer().requestTemporaryTexture(rect, true, false, TextureProperties::BilinearFilter);
            bloomTextures[1] = renderer().requestTemporaryTexture(rect, true, false, TextureProperties::BilinearFilter);
            bloomTextures[2] = renderer().requestTemporaryTexture(rect, true, false, TextureProperties::BilinearFilter);
        }
        else
            bloomTextures = std::array<const Texture*, 3>();
    }

    unsigned int getPassCount(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return renderer().isHDREnabled() ? 4 : 1;
    }

    void exitActiveShader()
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
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                         const ParameterArray& internalParams, unsigned int pass, unsigned int sortKey) override
    {
        exitActiveShader();

        if (!renderer().isHDREnabled())
        {
            // If HDR is disabled then PostProcessBloom does nothing so we just delegate to the pass through effect

            activeShader = effects().getEffectActiveShader("PostProcessPassThrough");
            if (!activeShader || !activeShader->setup())
                return;

            activeShader->enterShader();
            activeShader->setShaderParams(geometryChunk, params, internalParams, 0, 0);
        }
        else
        {
            if (pass == 0)
            {
                // Pass 1: Copy final render into low resolution texture clamping LDR values using the bright pass
                // effect

                activeShader = effects().getEffectActiveShader("PostProcessBrightPass");
                if (!activeShader || !activeShader->setup())
                    return;

                activeShader->enterShader();

                // Target the first bloom intermediate texture
                renderer().setPostProcessIntermediateTargetTexture(bloomTextures[0]);

                activeShader->setShaderParams(geometryChunk, params, internalParams, 0, 0);
            }
            else if (pass == 1 || pass == 2)
            {
                // Pass 2 and 3: Horizontal and vertical blur on the texture created by the bright pass effect in the
                // first pass, this uses the blur effect

                activeShader = effects().getEffectActiveShader("PostProcessBlur");
                if (!activeShader || !activeShader->setup())
                    return;

                // Setup parameters for the blur, the input texture is the output from the previous pass
                auto newParams = params;
                newParams[Parameter::blurType].setString(pass == 1 ? "horizontal" : "vertical");

                auto newInternalParams = internalParams;
                newInternalParams[Parameter::inputTexture].setPointer(bloomTextures[pass - 1]);

                // Enter shader
                activeShader->enterShader();

                // Target the next bloom intermediate texture
                renderer().setPostProcessIntermediateTargetTexture(bloomTextures[pass]);

                activeShader->setShaderParams(geometryChunk, newParams, newInternalParams, 0, 0);
            }
            else if (pass == 3)
            {
                // Pass 4: Creates the final bloomed image by overlaying the final blurred bloom texture on the original
                // input texture using the add effect

                activeShader = effects().getEffectActiveShader("PostProcessAdd");
                if (!activeShader || !activeShader->setup())
                    return;

                // Setup parameters for the add
                auto newParams = params;
                newParams[Parameter::addTexture].setPointer(bloomTextures[2]);
                newParams[Parameter::addTextureFactor] = params[Parameter::bloomFactor];
                newParams[Parameter::finalScale] = params[Parameter::exposure];

                // Enter shader
                activeShader->enterShader();
                activeShader->setShaderParams(geometryChunk, newParams, internalParams, 0, 0);
            }
        }
    }

    void exitShader() override
    {
        exitActiveShader();

        renderer().releaseTemporaryTexture(bloomTextures[0]);
        renderer().releaseTemporaryTexture(bloomTextures[1]);
        renderer().releaseTemporaryTexture(bloomTextures[2]);

        States::StateCacher::pop();
    }
};

CARBON_REGISTER_SHADER(PostProcessBloomGLSL, OpenGLBase)

}

}
