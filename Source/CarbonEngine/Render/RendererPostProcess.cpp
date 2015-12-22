/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Graphics/States/StateCacher.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Render/EffectQueue.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/RenderTarget.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

namespace Carbon
{

bool Renderer::setupForPostProcessing(RenderTarget& renderTarget, const EffectQueueArray& postProcessEffects,
                                      const Rect& viewport, bool* clearColorBuffer)
{
    if (!postProcessEffects.size() || !renderTarget)
        return false;

    // If an already rendered scene had post-process pass through enabled then just pick up where it left off, i.e. render this
    // scene over top into the same texture, this is what allows for post-process pass through
    auto colorTexture = renderTarget.getColorTexture();
    auto depthStencilTexture = renderTarget.getDepthTexture();

    if (!colorTexture)
    {
        colorTexture = requestTemporaryTexture(viewport, true, false, TextureProperties::BilinearFilter);
        depthStencilTexture = requestTemporaryTexture(viewport, Image::Depth24Stencil8);

        // This is a new color texture and so it needs to be cleared
        if (clearColorBuffer)
            *clearColorBuffer = true;
    }
    else
    {
        // We are continuing a post-process pass through started by a previous scene, don't want to clear what's already been
        // rendered
        if (clearColorBuffer)
            *clearColorBuffer = false;
    }

    // Setup the render target to use the allocated temporary textures
    if (!colorTexture || !depthStencilTexture ||
        !renderTarget.setTextures(colorTexture, depthStencilTexture, depthStencilTexture) || !renderTarget.isValid())
    {
        releaseTemporaryTexture(colorTexture);
        releaseTemporaryTexture(depthStencilTexture);
        return false;
    }

    States::StateCacher::push();

    States::RenderTarget = renderTarget;
    States::Viewport = colorTexture->getRect();

    return true;
}

void Renderer::drawPostProcess(RenderTarget& renderTarget, const EffectQueueArray& postProcessEffects,
                               GraphicsInterface::RenderTargetObject finalRenderTargetObject, const Rect& finalViewport,
                               bool isPostProcessPassThroughActive)
{
    // If post-process pass through is active then there is nothing to do except restore state, one of the subseqent scenes will
    // have post-processing effects on it that take the final post-process source texture from the render target and actually
    // process it for display
    if (isPostProcessPassThroughActive)
    {
        States::StateCacher::pop();
        return;
    }

    // Get the color and depth textures that contain the final render of the scene
    auto colorTexture = renderTarget.getColorTexture();
    auto depthTexture = renderTarget.getDepthTexture();

    // If multiple post-process effects are used then an additional intermediate texture will be needed to chain them together
    auto intermediateColorTexture = pointer_to<const Texture>::type();
    auto isIntermediateColorTextureCleared = false;

    // Detach the textures from the render target, the render target is available to be reused by multipass post-process shaders
    // and the render target needs to be clear to allow this to happen, see Renderer::setPostProcessIntermediateTargetTexture()
    renderTarget.removeTextures();
    activePostProcessRenderTarget_ = &renderTarget;

    States::DepthTestEnabled = false;

    // Loop over the post-process effects
    for (auto m = 0U; m < postProcessEffects.size(); m++)
    {
        auto effectQueue = postProcessEffects[m];

        // Get the shader and params to use
        auto shader = effectQueue->getEffect()->getActiveShader();
        auto& params = effectQueue->getParams();

        effectQueue->applyTextureAnimations();

        // Set the input textures to the post-process shader
        auto internalParams = ParameterArray();
        internalParams[Parameter::inputTexture].setPointer((m & 1) ? intermediateColorTexture : colorTexture);
        internalParams[Parameter::depthTexture].setPointer(depthTexture);

        // Check this is a post-process shader
        if (shader && shader->getShaderType(params, internalParams) != Shader::PostProcess)
            shader = nullptr;

        // Prepare shader
        if (!shader || !shader->setup())
        {
            // If there's no suitable shader for the post-process effect fall back to a passthrough
            shader = effects().getEffectActiveShader("PostProcessPassThrough");
            if (!shader || !shader->setup())
                continue;
        }

        shader->enterShader();

        auto isOutputSetup = false;

        // Render this post-process effect, one pass at a time
        auto passCount = shader->getPassCount(params, internalParams);
        for (auto pass = 0U; pass < passCount; pass++)
        {
            // If this is the last pass of the shader then set the output target appropriately
            auto isOutputting = (pass == passCount - 1) || shader->isPostProcessShaderReadyToOutput(pass);
            if (isOutputting && !isOutputSetup)
            {
                if (m == postProcessEffects.size() - 1)
                {
                    // If this is the last post-process effect then output to the final target
                    States::RenderTarget = finalRenderTargetObject;
                    States::Viewport = finalViewport;
                }
                else
                {
                    // Output to the other texture
                    if (renderTarget.getColorTexture() == colorTexture)
                    {
                        // Get intermediate color texture if it's not yet been used
                        if (!intermediateColorTexture)
                        {
                            intermediateColorTexture = requestTemporaryTexture(colorTexture->getRect(), true, false,
                                                                               TextureProperties::BilinearFilter);
                        }

                        renderTarget.setColorTexture(intermediateColorTexture);
                    }
                    else
                        renderTarget.setColorTexture(colorTexture);

                    // If this is the first time using the intermediate color texture then it needs to be cleared
                    if (intermediateColorTexture && !isIntermediateColorTextureCleared)
                    {
                        graphics().clearBuffers(true, false, false);
                        isIntermediateColorTextureCleared = true;
                    }
                }

                isOutputSetup = true;
            }

            // Setup for this pass
            shader->setShaderParams(unitRectangleGeometry_, params, internalParams, pass, 0);

            // Allow post-processed scenes to be blended over ones that have already been rendered
            if (isOutputting)
                Shaders::Blending::setShaderParams(params);

            States::DepthWriteEnabled = false;
            drawUnitRectangle();
        }

        shader->exitShader();
    }

    // Now that post-processing has been done the offscreen textures can be released
    renderTarget.removeTextures();
    releaseTemporaryTexture(intermediateColorTexture);
    releaseTemporaryTexture(colorTexture);
    releaseTemporaryTexture(depthTexture);

    States::StateCacher::pop();
    States::RenderTarget.flush();

    activePostProcessRenderTarget_ = nullptr;
}

void Renderer::checkPostProcessPassThroughsCompleted(RenderTarget& renderTarget)
{
    auto colorTexture = renderTarget.getColorTexture();
    auto depthTexture = renderTarget.getDepthTexture();

    // Check any post-process pass throughs were completed
    if (colorTexture)
    {
        LOG_WARNING << "A post-process pass through was left incomplete";

        releaseTemporaryTexture(colorTexture);
        releaseTemporaryTexture(depthTexture);
        renderTarget.removeTextures();
    }
}

}
