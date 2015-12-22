/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Graphics/States/StateCacher.h"
#include "CarbonEngine/Platform/Console.h"
#include "CarbonEngine/Platform/FrameTimers.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/ThemeManager.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Render/EffectQueue.h"
#include "CarbonEngine/Render/Font.h"
#include "CarbonEngine/Render/FontManager.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/RenderQueueItemArray.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/TextureCubemap.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

namespace Carbon
{

void Renderer::drawDebugOverlays()
{
    // Skip this method if no overlays are active
    if (!FrameTimers::Enabled && debugTexture_.name.length() == 0 && !showFPS_ && !showDebugInfo_ && !console().isVisible())
        return;

    // Camera for rendering the debug overlays
    auto debugOverlayCamera = Camera(SimpleTransform::Identity, States::Viewport.get(),
                                     Matrix4::getOrthographicProjection(States::Viewport.get()), 0.0f, 0.0f);
    pushCamera(debugOverlayCamera);

    States::DepthTestEnabled = false;

    // Font to draw with
    auto font = fonts().getSystemMonospaceFont();
    auto fontSize = font->getMaximumCharacterHeightInPixels();

    auto queues = EffectQueueArray();

    // Inset from the edges of the screen
    const auto borderPadding = 5.0f;

    drawDebugTexture(queues);
    drawFrameTimersGraph(font, fontSize);
    drawDebugInfo(font, fontSize, borderPadding, queues);
    drawConsole(font, fontSize, borderPadding, queues);

    drawEffectQueues(queues.getQueues());

    popCamera();
}

void Renderer::drawDebugInfo(const Font* font, float fontSize, float padding, EffectQueueArray& queues)
{
    if (showFPS_ || showDebugInfo_)
    {
        auto& items = queues.create(0, fontEffect_)->getItems();

        if (showFPS_ && !showDebugInfo_)
        {
            items.addChangeTransformItem(Vec3(padding, padding));
            items.addDrawTextItem(font, fontSize, UnicodeString(lastFPS_) + " FPS", Color::White);
        }
        else
        {
            // Add debugging info
            debugStrings_.prepend(UnicodeString() + lastFPS_ + " FPS   " + frameDrawCallCount_ + " Drawcalls   " +
                                  frameTriangleCount_ + " Triangles   " +
                                  (frameAPICallCount_ ? (UnicodeString(frameAPICallCount_) + " API calls") : ""));

            // Debug strings
            auto y = padding;
            for (const auto& debugString : debugStrings_)
            {
                items.addChangeTransformItem(Vec3(padding, y));
                items.addDrawTextItem(font, fontSize, debugString, Color::White * 0.8f);
                y += fontSize + 1.0f;
            }
        }
    }

    debugStrings_.clear();
}

void Renderer::drawConsole(const Font* font, float fontSize, float padding, EffectQueueArray& queues)
{
    if (!console().isVisible())
        return;

    const auto borderSize = 1.0f;

    auto outputLineCount = console().calculateOutputLineCount(fontSize);
    auto width = platform().getWindowWidthf();

    auto height = fontSize * float(outputLineCount + 1);
    auto offset = platform().getWindowHeightf() - height * console().getExpansion();

    // Draw main body of the console
    auto q = queues.create(0, baseColoredEffect_);
    q->setCustomParameter(Parameter::diffuseColor, theme()["ConsoleFillColor"]);
    q->setCustomParameter(Parameter::blend, true);
    q->getItems().addChangeTransformItem(Vec3(0.0f, offset));
    q->getItems().addDrawRectangleItem(width, height);

    // Draw border line at the bottom
    q = queues.create(1, baseColoredEffect_);
    q->setCustomParameter(Parameter::diffuseColor, theme()["ConsoleBorderColor"]);
    q->setCustomParameter(Parameter::blend, true);
    q->getItems().addChangeTransformItem(Vec3(0.0f, offset - borderSize));
    q->getItems().addDrawRectangleItem(width, borderSize);

    auto& consoleTextColor = theme()["ConsoleTextColor"];

    q = queues.create(2, fontEffect_);

    // Draw history items
    for (auto i = 0U; i < outputLineCount; i++)
    {
        auto& historyItem = console().getHistoryItem(console().getHistorySize() - (console().getHistoryOffsetY() + i) - 1);
        if (historyItem.length() > console().getHistoryOffsetX())
        {
            q->getItems().addChangeTransformItem(Vec3(padding, offset + ((i + 1) * fontSize)));
            q->getItems().addDrawTextItem(font, fontSize, historyItem.substr(console().getHistoryOffsetX()), consoleTextColor);
        }
    }

    // Draw current text
    q->getItems().addChangeTransformItem(Vec3(padding, offset));
    q->getItems().addDrawTextItem(font, fontSize, console().getPrompt() + console().getCurrentText(), consoleTextColor);

    // Draw cursor
    if (console().getTextInput().isCursorOn(false))
    {
        auto s = console().getPrompt() + console().getCurrentText().substr(0, console().getTextInput().getCursorPosition());

        auto xOffset = font->getWidth(s, fontSize) - font->getCharacterPreMove('|', fontSize);

        q->getItems().addChangeTransformItem(Vec3(padding + xOffset, offset));
        q->getItems().addDrawTextItem(font, fontSize, "|", consoleTextColor);
    }
}

void Renderer::onFrameTimersSamplingDataReady(FrameTimers& sender, TimeValue time)
{
    // The frame timers graph geometry will be updated when rendering the next frame
    updateFrameTimersGraph_ = true;
    lastFrameTimersGraphUpdateTime_ = time;
}

void Renderer::setupFrameTimersGraphGeometryChunks(unsigned int timerCount)
{
    if (timerResultsGeometryChunk_.getVertexCount() == FrameTimers::HistorySize * timerCount &&
        timerGraphAxesGeometryChunk_.getVertexCount() == 18)
        return;

    // Setup vertex data for the timer result graph
    timerResultsGeometryChunk_.clear();
    timerResultsGeometryChunk_.addVertexStream({VertexStream::Position, 3});
    timerResultsGeometryChunk_.addVertexStream({VertexStream::DiffuseTextureCoordinate, 2});
    timerResultsGeometryChunk_.addVertexStream({VertexStream::Color, 4, TypeUInt8});
    timerResultsGeometryChunk_.setVertexCount(FrameTimers::HistorySize * timerCount);
    timerResultsGeometryChunk_.setDynamic(true);

    // Create draw items and index list
    auto drawItems = Vector<DrawItem>();
    auto indices = Vector<unsigned int>();
    for (auto i = 0U; i < timerCount; i++)
    {
        drawItems.emplace(GraphicsInterface::LineStrip, FrameTimers::HistorySize, i * FrameTimers::HistorySize);

        for (auto j = 0U; j < FrameTimers::HistorySize; j++)
            indices.append(i * FrameTimers::HistorySize + j);
    }

    timerResultsGeometryChunk_.setupIndexData(drawItems, indices);
    timerResultsGeometryChunk_.registerWithRenderer();

    // Setup vertex data for the frame timer graph axes
    timerGraphAxesGeometryChunk_.clear();
    timerGraphAxesGeometryChunk_.addVertexStream({VertexStream::Position, 3});
    timerGraphAxesGeometryChunk_.addVertexStream({VertexStream::DiffuseTextureCoordinate, 2});
    timerGraphAxesGeometryChunk_.addVertexStream({VertexStream::Color, 4, TypeUInt8});
    timerGraphAxesGeometryChunk_.setVertexCount(18);
    timerGraphAxesGeometryChunk_.setDynamic(true);

    // The graph axes are drawn with 10 triangles using 18 vertices
    drawItems.clear();
    drawItems.emplace(GraphicsInterface::TriangleList, 30, 0);

    // Indices for the 25% line, 50% line, 75% line, bottom edge, and left edge
    indices = Vector<unsigned int>{4,  5,  6,  5, 7, 6, 8, 9, 10, 9, 11, 10, 12, 13, 14,
                                   13, 15, 14, 0, 1, 2, 1, 3, 2,  0, 2,  16, 2,  17, 16};

    timerGraphAxesGeometryChunk_.setupIndexData(drawItems, indices);
    timerGraphAxesGeometryChunk_.registerWithRenderer();
}

void Renderer::drawFrameTimersGraph(const Font* font, float fontSize)
{
    if (!FrameTimers::Enabled || !frameTimerRenderingEnabled_)
        return;

    // These constants are used when rendering the frame timers graph, all units are in pixels
    const auto borderSize = floorf(0.1f * platform().getWindowWidthf());
    const auto textPadding = 150.0f;
    const auto graphWidth = (platform().getWindowWidthf() - borderSize * 2.0f - textPadding);
    const auto graphHeight = platform().getWindowHeightf() - borderSize * 2.0f;
    const auto sampleWidth = graphWidth / float(FrameTimers::HistorySize - 1);
    const auto primaryAxisSize = 2.0f;
    const auto secondaryAxisSize = 1.0f;
    const auto primaryAxisColor = Color::White.toRGBA8();
    const auto secondaryAxisColor = Color(0.85f, 0.85f, 0.85f, 1.0f).toRGBA8();

    // Get the list of frame timers
    auto& timers = FrameTimers::getRegisteredTimers();

    // Ensure that the geometry chunks are setup
    setupFrameTimersGraphGeometryChunks(timers.size());

    // Update the display if needed
    if (updateFrameTimersGraph_)
    {
        updateFrameTimersGraph_ = false;

        // Update vertices for each timer's results line
        {
            timerResultsGeometryChunk_.lockVertexData();

            auto itPosition = timerResultsGeometryChunk_.getVertexStreamIterator<Vec3>(VertexStream::Position);
            auto itColor = timerResultsGeometryChunk_.getVertexStreamIterator<unsigned int>(VertexStream::Color);
            for (auto timer : timers)
            {
                for (auto j = 0U; j < FrameTimers::HistorySize; j++)
                {
                    *itPosition++ =
                        Vec3(float(FrameTimers::HistorySize - j - 1) * sampleWidth, timer->getHistoryEntry(j) * graphHeight);
                    *itColor++ = timer->getColor().toRGBA8();
                }
            }
            timerResultsGeometryChunk_.unlockVertexData();
        }

        // Update vertices for the graph axes
        {
            timerGraphAxesGeometryChunk_.lockVertexData();

            auto itPosition = timerGraphAxesGeometryChunk_.getVertexStreamIterator<Vec3>(VertexStream::Position);
            auto itColor = timerGraphAxesGeometryChunk_.getVertexStreamIterator<unsigned int>(VertexStream::Color);

            // Vertices for the bottom primary axis
            *itPosition++ = Vec3(-primaryAxisSize, -primaryAxisSize);
            *itColor++ = primaryAxisColor;
            *itPosition++ = Vec3(graphWidth - sampleWidth, -primaryAxisSize);
            *itColor++ = primaryAxisColor;
            *itPosition++ = Vec3::Zero;
            *itColor++ = primaryAxisColor;
            *itPosition++ = Vec3(graphWidth - sampleWidth, 0.0f);
            *itColor++ = primaryAxisColor;

            // Vertices for the three secondary axes
            for (auto i = 0U; i < 3; i++)
            {
                auto y = floorf(graphHeight * float(i + 1) * 0.25f);

                *itPosition++ = Vec3(0.0f, y);
                *itColor++ = secondaryAxisColor;
                *itPosition++ = Vec3(graphWidth - sampleWidth, y);
                *itColor++ = secondaryAxisColor;
                *itPosition++ = Vec3(0.0f, y + secondaryAxisSize);
                *itColor++ = secondaryAxisColor;
                *itPosition++ = Vec3(graphWidth - sampleWidth, y + secondaryAxisSize);
                *itColor++ = secondaryAxisColor;
            }

            // Top left corner vertices
            *itPosition++ = Vec3(-primaryAxisSize, graphHeight);
            *itColor++ = primaryAxisColor;
            *itPosition++ = Vec3(0.0f, graphHeight);
            *itColor++ = primaryAxisColor;

            timerGraphAxesGeometryChunk_.unlockVertexData();
        }
    }

    auto baseDiffuseVertexColorParams = ParameterArray();
    baseDiffuseVertexColorParams[Parameter::diffuseColor].setColor(Color::White);
    baseDiffuseVertexColorParams[Parameter::diffuseMap].setPointer<Texture>(getWhiteTexture());
    baseDiffuseVertexColorParams[Parameter::useVertexColor].setBoolean(true);

    // Render grid lines
    {
        auto q = EffectQueue(0, baseSurfaceEffect_);

        q.useParams(baseDiffuseVertexColorParams);
        q.setSortKey(baseSurfaceEffect_->getActiveShader()->getSortKey(baseDiffuseVertexColorParams, ParameterArray::Empty));
        q.getItems().addChangeTransformItem(Vec3(borderSize, borderSize));
        q.getItems().addDrawGeometryChunkItem(timerGraphAxesGeometryChunk_);

        drawEffectQueues(Vector<EffectQueue*>(1, &q));
    }

    // Render timer lines
    {
        States::StateCacher::push();

        States::ScissorEnabled = true;
        States::ScissorRectangle =
            Rect(borderSize, borderSize, borderSize + graphWidth - sampleWidth, borderSize + graphHeight);

        auto q = EffectQueue(0, baseSurfaceEffect_);

        auto xOffset = lastFrameTimersGraphUpdateTime_.getSecondsSince() * FrameTimers::ReportingFrequency;
        xOffset = Math::clamp01(xOffset) * sampleWidth;

        q.useParams(baseDiffuseVertexColorParams);
        q.setSortKey(baseSurfaceEffect_->getActiveShader()->getSortKey(baseDiffuseVertexColorParams, ParameterArray::Empty));
        q.getItems().addChangeTransformItem(Vec3(borderSize - xOffset, borderSize));
        q.getItems().addDrawGeometryChunkItem(timerResultsGeometryChunk_);

        drawEffectQueues(Vector<EffectQueue*>(1, &q));

        States::StateCacher::pop();
    }

    // Render labels
    {
        auto q = EffectQueue(0, fontEffect_);

        auto textTop = borderSize + graphHeight - (graphHeight - timers.size() * fontSize) * 0.5f;

        for (auto i = 0U; i < timers.size(); i++)
        {
            q.getItems().addChangeTransformItem(Vec3(borderSize + graphWidth + 10.0f, textTop - i * fontSize));
            q.getItems().addDrawTextItem(font, fontSize, timers[i]->getName(), timers[i]->getColor());
        }

        static const auto axisLabels = std::array<UnicodeString, 3>{{"25%", "50%", "75%"}};
        for (auto i = 0U; i < 3; i++)
        {
            q.getItems().addChangeTransformItem(Vec3(borderSize - font->getWidth(axisLabels[i], fontSize) - 2.0f,
                                                     ceilf(borderSize + float(i + 1) * graphHeight * 0.25f - fontSize * 0.5f)));
            q.getItems().addDrawTextItem(font, fontSize, axisLabels[i], Color::White);
        }

        drawEffectQueues(Vector<EffectQueue*>(1, &q));
    }
}

void Renderer::drawDebugTexture(EffectQueueArray& queues)
{
    if (!debugTexture_.name.length())
        return;

    auto t = textures().getTexture(debugTexture_.name);
    if (!t)
        return;

    t->ensureImageIsLoaded();

    // Get dimensions
    auto totalWidth = 0U;
    auto totalHeight = 0U;
    auto mipmapCount = 0U;
    auto dimensionsInfo = UnicodeString();
    if (t->getTextureType() == GraphicsInterface::Texture2D)
    {
        auto tex = static_cast<Texture2D*>(t);

        totalWidth = tex->getWidth();
        totalHeight = tex->getHeight();
        dimensionsInfo = UnicodeString(totalWidth) + "x" + totalHeight;
        mipmapCount = Image::getImageMipmapCount(totalWidth, totalHeight, 1);

        // Handle texture quality setting
        auto firstLevel = t->calculateFirstMipmapLevel();
        if (firstLevel)
        {
            mipmapCount -= firstLevel;

            auto hardwareWidth = tex->getWidth();
            auto hardwareHeight = tex->getHeight();
            for (auto i = 0U; i < firstLevel; i++)
            {
                if (hardwareWidth > 1)
                    hardwareWidth /= 2;
                if (hardwareHeight > 1)
                    hardwareHeight /= 2;
            }

            dimensionsInfo << " (uploaded as " << hardwareWidth << "x" << hardwareHeight << ")";
        }
    }
    else
    {
        auto size = static_cast<TextureCubemap*>(t)->getSize();

        totalWidth = size * 4;
        totalHeight = size * 3;
        dimensionsInfo = UnicodeString(size) + "x" + size + "x6";
        mipmapCount = Image::getImageMipmapCount(size, size, 1) - t->calculateFirstMipmapLevel();

        // Handle texture quality setting
        auto firstLevel = t->calculateFirstMipmapLevel();
        if (firstLevel)
        {
            mipmapCount -= firstLevel;
            auto hardwareSize = size >> firstLevel;

            dimensionsInfo << " (uploaded as " << hardwareSize << "x" << hardwareSize << "x6)";
        }
    }

    auto font = fonts().getSystemMonospaceFont();
    auto fontSize = font->getMaximumCharacterHeightInPixels();

    const auto padding = 5.0f;

    // Scale factor, texture must fit in the window, leaving a bit of padding and room for the two lines of info text
    auto maxHorizontalSize = platform().getWindowWidthf() - padding * 2.0f;
    auto maxVerticalSize = platform().getWindowHeightf() - padding * 2.0f - fontSize * 2.0f;

    auto scaleFactor = debugTexture_.scale;
    if (float(totalWidth) * scaleFactor > maxHorizontalSize)
        scaleFactor = maxHorizontalSize / float(totalWidth);
    if (float(totalHeight) * scaleFactor > maxVerticalSize)
        scaleFactor = maxVerticalSize / float(totalHeight);

    auto width = floorf(totalWidth * scaleFactor);
    auto height = floorf(totalHeight * scaleFactor);

    // Get the old current frame and properties to restore afterwards
    auto originalFrame = t->getCurrentFrame();
    auto originalTextureProperties = t->getProperties();

    // Display with the default properties, apart from the quality level
    auto newTextureProperties = TextureProperties();
    newTextureProperties.setQuality(originalTextureProperties.getQuality());
    t->setProperties(newTextureProperties);

    // Set the texture frame to view and and clamp to the chosen mipmap level
    t->setCurrentFrame(debugTexture_.frame);
    graphics().setTextureBaseAndMaximumMipmapLevels(t->getActiveTextureObject(), t->getTextureType(), debugTexture_.mipmap,
                                                    debugTexture_.mipmap);

    States::StateCacher::push();

    // Position for drawing the texture surface
    modelViewMatrix_ = Matrix4::getTranslation({platform().getWindowWidthf() - width - padding, padding, 0.0f});

    // Draw the surface
    drawDebugTextureSurface(t, scaleFactor);

    // Put everything back how it was
    graphics().setTextureBaseAndMaximumMipmapLevels(t->getActiveTextureObject(), t->getTextureType(), 0, mipmapCount);
    States::StateCacher::pop();

    t->setProperties(originalTextureProperties);
    t->setCurrentFrame(originalFrame);

    // Print info on the texture
    auto lines = std::array<UnicodeString, 2>();
    lines[0] = Texture::convertTextureTypeToString(t->getTextureType()) + " texture '" + t->getName() + "', ";
    if (t->getState() == Texture::Error)
        lines[0] << "failed to load";
    else
    {
        auto& image = t->getImage();

        lines[0] << dimensionsInfo + ", " + Image::getPixelFormatString(image.getPixelFormat());

        if (t->getFrameCount() > 1)
            lines[0] << ", " << t->getFrameCount() << " frames";

        if (image.getPixelFormat() == Image::UnknownPixelFormat)
            lines[1] = "Error: unknown pixel format";
        else if ((!image.hasMipmaps() && debugTexture_.mipmap) || (image.hasMipmaps() && debugTexture_.mipmap >= mipmapCount))
            lines[1] = "Error: nonexistent mipmap selected";
        else
        {
            lines[1] = UnicodeString() + "Showing " + (debugTexture_.renderAlpha ? "alpha" : "rgb") + " of frame " +
                debugTexture_.frame + ", mipmap level " + debugTexture_.mipmap + ", scale factor " +
                (scaleFactor * powf(2.0f, float(debugTexture_.mipmap)));
        }
    }

    auto& items = queues.create(0, fontEffect_)->getItems();

    items.addChangeTransformItem(
        Vec3(platform().getWindowWidthf() - padding - font->getWidth(lines[0], fontSize), height + padding + fontSize));
    items.addDrawTextItem(font, fontSize, lines[0]);

    items.addChangeTransformItem(
        Vec3(platform().getWindowWidthf() - padding - font->getWidth(lines[1], fontSize), height + padding));
    items.addDrawTextItem(font, fontSize, lines[1]);
}

void Renderer::drawDebugTextureSurface(const Texture* texture, float scale)
{
    auto params = ParameterArray();

    params[Parameter::diffuseColor].setColor(Color::White);
    params[Parameter::diffuseMap].setPointer(static_cast<const Texture*>(getWhiteTexture()));

    // To view the alpha channel of a texture first a solid white background is drawn and then the texture itself is rendered
    // with a blending mode that multiplies the framebuffer by the incoming texture alpha

    auto shader = baseSurfaceEffect_->getActiveShader();
    if (!shader || !shader->setup())
        return;

    // Render a white background when viewing an alpha channel
    if (debugTexture_.renderAlpha)
    {
        shader->enterShader();
        drawDebugTextureSurfaceGeometry(texture, scale, shader, params);
        shader->exitShader();
    }

    shader->enterShader();

    params[Parameter::diffuseMap].setPointer(texture);

    if (debugTexture_.renderAlpha)
    {
        params[Parameter::blend].setBoolean(true);
        params[Parameter::blendSourceFactor].setInteger(States::Zero);
        params[Parameter::blendDestinationFactor].setInteger(States::SourceAlpha);
    }

    drawDebugTextureSurfaceGeometry(texture, scale, shader, params);

    shader->exitShader();
}

void Renderer::drawDebugTextureSurfaceGeometry(const Texture* texture, float scale, Shader* shader,
                                               const ParameterArray& params)
{
    if (texture->getTextureType() == GraphicsInterface::Texture2D)
    {
        auto previousModelViewMatrix = modelViewMatrix_;

        modelViewMatrix_.scale(texture->getImage().getWidth() * scale, texture->getImage().getHeight() * scale);

        clearCachedTransforms();

        shader->setShaderParams(unitRectangleGeometry_, params, ParameterArray::Empty, 0,
                                shader->getSortKey(params, ParameterArray::Empty));
        drawUnitRectangle();

        modelViewMatrix_ = previousModelViewMatrix;
        clearCachedTransforms();
    }
    else if (texture->getTextureType() == GraphicsInterface::TextureCubemap)
    {
        // TODO: Add back support for debug rendering of cubemap textures, the original implementation was removed when OpenGL
        // ES 2 support was added in r4684
    }
}

}
