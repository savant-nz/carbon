/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Graphics/States/StateCacher.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Platform/Console.h"
#include "CarbonEngine/Platform/FrameTimers.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/ThemeManager.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Render/EffectQueue.h"
#include "CarbonEngine/Render/Font.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

namespace Carbon
{

CARBON_DEFINE_FRAME_TIMER(RendererDrawGeometryTimer, Color(1.0f, 1.0f, 0.0f))
CARBON_DEFINE_FRAME_TIMER(RendererDrawTextTimer, Color(0.5f, 1.0f, 0.0f))
CARBON_DEFINE_FRAME_TIMER(ShaderTimer, Color(0.6f, 0.0f, 0.0f))

void Renderer::drawEffectQueues(const Vector<EffectQueue*>& queues, BlendedGeometrySetting blendedGeometrySetting,
                                Effect* overrideEffect)
{
    auto currentShader = pointer_to<Shader>::type();

    auto isModelViewMatrixCurrent = false;

    for (auto q : queues)
    {
        auto needsNewSortKey = true;

        auto effect = pointer_to<Effect>::type();

        // When doing the final pass of deferred lighting users of BaseSurface are switched to DeferredLightingSurface
        if (q->getEffect() && deferredLightingTexture_ && q->getEffect() == baseSurfaceEffect_)
            effect = deferredLightingSurfaceEffect_;
        else
        {
            effect = q->getEffect();
            needsNewSortKey = false;
        }

        if (overrideEffect)
        {
            if (overrideEffect == deferredLightingSetupEffect_ && effect != baseSurfaceEffect_)
                continue;

            effect = overrideEffect;
            needsNewSortKey = true;
        }

        {
            auto timer = ScopedFrameTimer(ShaderTimer);

            // Get the shader to use for rendering this effect
            auto nextShader = effect->getActiveShader();
            if (!nextShader)
                continue;

            // Switch shader if needed
            if (nextShader != currentShader)
            {
                // Exit previous shader
                if (currentShader)
                    currentShader->exitShader();

                // Set the new shader as current
                currentShader = nextShader;

                // Setup the shader if needed
                if (!currentShader->setup())
                {
                    currentShader = nullptr;
                    continue;
                }

                currentShader->enterShader();
            }

            // Update texture animation frames for this queue
            q->applyTextureAnimations();
        }

        // Get sort key
        auto sortKey = q->getSortKey();
        if (needsNewSortKey && currentShader)
            sortKey = currentShader->getSortKey(q->getParams(), q->getInternalParams());

        // Loop through queue items actioning each one
        for (auto item : q->getItems())
        {
            if (auto transformItem = item->as<ChangeTransformRenderQueueItem>())
            {
                executeRenderQueueItem(*transformItem);

                isModelViewMatrixCurrent = false;
            }
            else
            {
                // Ensure the model-view matrix is up to date before rendering anything
                if (!isModelViewMatrixCurrent)
                {
                    auto modelMatrix = Matrix4();

                    // Identity quaternions are fairly common so detect this case to avoid a quaternion->matrix
                    // conversion
                    if (currentTransform_.getOrientation() == Quaternion::Identity)
                    {
                        modelMatrix[0] = currentScale_.x;
                        modelMatrix[1] = 0.0f;
                        modelMatrix[2] = 0.0f;
                        modelMatrix[3] = 0.0f;
                        modelMatrix[4] = 0.0f;
                        modelMatrix[5] = currentScale_.y;
                        modelMatrix[6] = 0.0f;
                        modelMatrix[7] = 0.0f;
                        modelMatrix[8] = 0.0f;
                        modelMatrix[9] = 0.0f;
                        modelMatrix[10] = currentScale_.z;
                        modelMatrix[11] = 0.0f;
                        modelMatrix[12] = currentTransform_.getPosition().x;
                        modelMatrix[13] = currentTransform_.getPosition().y;
                        modelMatrix[14] = currentTransform_.getPosition().z;
                        modelMatrix[15] = 1.0f;
                    }
                    else
                    {
                        modelMatrix = currentTransform_.getMatrix();
                        modelMatrix.scale(currentScale_);
                    }

                    // Update model-view matrix
                    modelViewMatrix_ = getCamera().getViewMatrix() * modelMatrix;
                    isModelViewMatrixCurrent = true;
                }

                // Apply the blended geometry setting
                if (blendedGeometrySetting != DrawBlendedGeometry)
                {
                    auto isBlended =
                        (currentShader->getShaderType(q->getParams(), q->getInternalParams()) == Shader::Blended);

                    if ((blendedGeometrySetting == SkipBlendedGeometry && isBlended) ||
                        (blendedGeometrySetting == OnlyDrawBlendedGeometry && !isBlended))
                        continue;
                }

                auto timer = ScopedFrameTimer(RendererDrawGeometryTimer);

                // Call the appropriate method for executing this queue item
                if (auto chunkItem = item->as<DrawGeometryChunkRenderQueueItem>())
                    executeRenderQueueItem(*chunkItem, effect, currentShader, q->getParams(), q->getInternalParams(),
                                           sortKey);
                else if (auto rectItem = item->as<DrawRectangleRenderQueueItem>())
                    executeRenderQueueItem(*rectItem, currentShader, q->getParams(), q->getInternalParams(), sortKey);
                else if (auto textItem = item->as<DrawTextRenderQueueItem>())
                    executeRenderQueueItem(*textItem, currentShader);
            }
        }
    }

    // Exit current shader
    if (currentShader)
        currentShader->exitShader();
}

void Renderer::executeRenderQueueItem(const ChangeTransformRenderQueueItem& item)
{
    currentTransform_ = item.getTransform();
    currentScale_ = item.getScale();

    clearCachedTransforms();
}

void Renderer::clearCachedTransforms()
{
    isCurrentOrientationInverseMatrixCached_ = false;
    isCurrentTransformInverseMatrixCached_ = false;
    isLocalSpaceCameraPositionCached_ = false;
    isModelViewProjectionMatrixCached_ = false;
    isScreenProjectionMatrixCached_ = false;
}

void Renderer::executeRenderQueueItem(const DrawTextRenderQueueItem& item, Shader* shader)
{
    if (!item.getFont() || !item.getFont()->isReadyForUse())
        return;

    auto timer = ScopedFrameTimer(RendererDrawTextTimer);

    // Calculate pixel dimensions, this assumes an orthographic projection matrix
    auto xPixelSize = (2.0f / getCamera().getProjectionMatrix()[0]) / States::Viewport.get().getWidth();
    auto yPixelSize = (2.0f / getCamera().getProjectionMatrix()[5]) / States::Viewport.get().getHeight();

    // Prepare a parameter array with diffuse texture and color to pass to setShaderParams() for the internal font
    // shader
    auto params = ParameterArray();
    params[Parameter::diffuseMap].setPointer<Texture>(item.getFont()->getTexture());
    params[Parameter::diffuseColor].setColor(item.getColor());

    auto& chunk = item.getFont()->getGeometryChunk();

    // Start rendering with the font shader
    shader->setShaderParams(chunk, params, ParameterArray::Empty, 0, 0);

    auto indexBufferOffset = uintptr_t();
    auto indexDataBuffer = dataBuffers().getAllocationBufferObject(chunk.getIndexAllocation(), indexBufferOffset);
    auto indexDataTypeSize = getDataTypeSize(chunk.getIndexDataType());

    // Disable FSAA when rendering text
    States::MultisampleEnabled.push();
    States::MultisampleEnabled = false;

    States::StateCacher::flush();

    auto initialModelViewMatrix = modelViewMatrix_;

    // Scale for the font size
    modelViewMatrix_.scale(item.getFontSize(), item.getFontSize());

    // Round to whole pixels if requested
    if (item.getFont()->getAlignCharactersToPixelBoundaries())
        modelViewMatrix_[13] = floorf(modelViewMatrix_[13] / yPixelSize) * yPixelSize;

    // The x translation is incremented appropriately for each character
    auto xTranslation = 0.0f;
    auto xTranslationScaleFactor = 1.0f / item.getFont()->getMaximumCharacterHeightInPixels();
    auto oneOverXPixelSize = 1.0f / xPixelSize;

    // Iterate over every character in the string
    auto codePoint = item.getText().cStr();
    while (*codePoint)
    {
        auto characterIndex = item.getFont()->getCharacterIndex(*codePoint);
        if (characterIndex == -1)
        {
            // This font can't render the requested character, use the fallback
            characterIndex = item.getFont()->getCharacterIndex(Font::FallbackCharacter);
            if (characterIndex == -1)
                continue;
        }

        auto& character = item.getFont()->getCharacters()[characterIndex];

        // Position this character
        modelViewMatrix_.translate((xTranslation + character.getPreMove()) * xTranslationScaleFactor);

        // Clamp the X translation to a pixel boundary, the real accumulated translation is restored after flushing the
        // clamped modelview matrix
        auto modelView12 = 0.0f;
        if (item.getFont()->getAlignCharactersToPixelBoundaries())
        {
            modelView12 = modelViewMatrix_[12];
            modelViewMatrix_[12] = floorf(modelViewMatrix_[12] * oneOverXPixelSize) * xPixelSize;
        }

        clearCachedTransforms();

        // Setting pass to 1 indicates to the font shader that only the model-view state has changed
        shader->setShaderParams(chunk, params, ParameterArray::Empty, 1, 0);

        if (item.getFont()->getAlignCharactersToPixelBoundaries())
            modelViewMatrix_[12] = modelView12;

        // Create index data for drawing this character
        auto offset = characterIndex * 6;

        // Draw the character
        graphics().drawIndexedPrimitives(GraphicsInterface::TriangleList, offset, offset + 3, 6, TypeUInt16,
                                         indexDataBuffer, indexBufferOffset + offset * indexDataTypeSize);

        // Move to the next character in the string
        codePoint++;
        xTranslation = character.getWidth() + character.getPostMove();
    }

    // Restore states
    States::MultisampleEnabled.pop();
    modelViewMatrix_ = initialModelViewMatrix;
    clearCachedTransforms();
}

void Renderer::executeRenderQueueItem(const DrawRectangleRenderQueueItem& item, Shader* shader,
                                      const ParameterArray& params, const ParameterArray& internalParams,
                                      unsigned int sortKey)
{
    auto initialModelViewMatrix = modelViewMatrix_;
    modelViewMatrix_.scale(item.getWidth(), item.getHeight());

    auto initialScale = currentScale_;
    currentScale_ *= Vec3(item.getWidth(), item.getHeight(), 1.0f);

    auto passCount = shader->getPassCount(params, internalParams);
    for (auto i = 0U; i < passCount; i++)
    {
        {
            auto timer = ScopedFrameTimer(ShaderTimer);
            shader->setShaderParams(unitRectangleGeometry_, params, internalParams, i, sortKey);
        }

        drawUnitRectangle();
    }

    modelViewMatrix_ = initialModelViewMatrix;
    currentScale_ = initialScale;
    clearCachedTransforms();
}

void Renderer::executeRenderQueueItem(const DrawGeometryChunkRenderQueueItem& item, Effect* effect, Shader* shader,
                                      const ParameterArray& params, const ParameterArray& internalParams,
                                      unsigned int sortKey)
{
    auto& chunk = item.getGeometryChunk();

    if (!chunk.isRegisteredWithRenderer())
    {
        LOG_WARNING << "Skipping unregistered geometry chunk";
        return;
    }

    if (!chunk.setupForEffect(effect))
        return;

    auto indexBufferOffset = uintptr_t(0);
    auto indexDataBuffer = dataBuffers().getAllocationBufferObject(chunk.getIndexAllocation(), indexBufferOffset);
    auto indexDataTypeSize = getDataTypeSize(chunk.getIndexDataType());

    auto passCount = shader->getPassCount(params, internalParams);
    for (auto i = 0U; i < passCount; i++)
    {
        {
            auto timer = ScopedFrameTimer(ShaderTimer);
            shader->setShaderParams(chunk, params, internalParams, i, sortKey);

            States::StateCacher::flush();
        }

        if (item.getDrawItemIndex() < 0)
        {
            for (auto& drawItem : chunk.getDrawItems())
            {
                graphics().drawIndexedPrimitives(drawItem.getPrimitiveType(), drawItem.getLowestIndex(),
                                                 drawItem.getHighestIndex(), drawItem.getIndexCount(),
                                                 chunk.getIndexDataType(), indexDataBuffer,
                                                 indexBufferOffset + drawItem.getIndexOffset() * indexDataTypeSize);
            }
        }
        else
        {
            auto& drawItem = chunk.getDrawItems()[item.getDrawItemIndex()];

            graphics().drawIndexedPrimitives(drawItem.getPrimitiveType(), drawItem.getLowestIndex(),
                                             drawItem.getHighestIndex(), drawItem.getIndexCount(),
                                             chunk.getIndexDataType(), indexDataBuffer,
                                             indexBufferOffset + drawItem.getIndexOffset() * indexDataTypeSize);
        }
    }
}

void Renderer::drawUnitRectangle()
{
    auto indexBufferOffset = uintptr_t(0);
    auto indexAllocation = unitRectangleGeometry_.getIndexAllocation();
    auto indexDataBuffer = dataBuffers().getAllocationBufferObject(indexAllocation, indexBufferOffset);

    auto& drawItem = unitRectangleGeometry_.getDrawItems()[0];
    auto indexDataTypeSize = getDataTypeSize(unitRectangleGeometry_.getIndexDataType());

    States::StateCacher::flush();

    graphics().drawIndexedPrimitives(drawItem.getPrimitiveType(), drawItem.getLowestIndex(), drawItem.getHighestIndex(),
                                     drawItem.getIndexCount(), unitRectangleGeometry_.getIndexDataType(),
                                     indexDataBuffer,
                                     indexBufferOffset + drawItem.getIndexOffset() * indexDataTypeSize);
}

}
