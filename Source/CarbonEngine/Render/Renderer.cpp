/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Core/SettingsManager.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Graphics/States/StateCacher.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/Console.h"
#include "CarbonEngine/Platform/FrameTimers.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/ThemeManager.h"
#include "CarbonEngine/Render/DataBufferManager.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Render/EffectQueue.h"
#include "CarbonEngine/Render/EffectQueueArray.h"
#include "CarbonEngine/Render/FontManager.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/TextureCubemap.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

namespace Carbon
{

const auto ShadowMapSizeSetting = String("ShadowMapSize");
const auto ShowFPSSetting = String("ShowFPS");
const auto ShowDebugInfoSetting = String("ShowDebugInfo");
const auto HDREnabledSetting = String("HDREnabled");
const auto FrameTimerRenderingEnabledSetting = String("FrameTimerRenderingEnabled");

CARBON_DEFINE_FRAME_TIMER(SwapBuffersTimer, Color(0.0f, 0.6f, 0.3f))
CARBON_DEFINE_FRAME_TIMER(RendererSortTimer, Color(1.0f, 0.0f, 1.0f))
CARBON_DEFINE_FRAME_TIMER(RendererTimer, Color(1.0f, 0.0f, 0.0f))

Renderer::~Renderer()
{
    clearTemporaryRenderTargets();
    clearTemporaryTextures();

    textures().releaseTexture(errorTexture_);
    textures().releaseTexture(errorNormalMap_);
    textures().releaseTexture(whiteTexture_);
    textures().releaseTexture(blackTexture_);
    textures().releaseTexture(whiteCubemapTexture_);
    textures().releaseTexture(blackCubemapTexture_);

    events().removeHandler(this);
    FrameTimers::OnSamplingDataReady.removeHandler(this, &Renderer::onFrameTimersSamplingDataReady);
}

bool Renderer::setup()
{
    // Initialize graphics interface
    if (!graphics().setup())
    {
        LOG_ERROR << "Failed initializing graphics interface";
        return false;
    }

    events().addHandler<ResizeEvent>(this);
    events().addHandler<RecreateWindowEvent>(this);

    // Setup texture layer
    textures().setup();

    // Setup effects
    effects().loadEffects();
    baseColoredEffect_ = effects().getEffect("BaseColored");
    baseShadowMappingEffect_ = effects().getEffect("InternalShadowMapping");
    baseSurfaceEffect_ = effects().getEffect("BaseSurface");
    deferredLightingDirectionalLightEffect_ = effects().getEffect("InternalDeferredLightingDirectionalLight");
    deferredLightingPointLightEffect_ = effects().getEffect("InternalDeferredLightingPointLight");
    deferredLightingSetupEffect_ = effects().getEffect("InternalDeferredLightingSetup");
    deferredLightingSurfaceEffect_ = effects().getEffect("InternalDeferredLightingSurface");
    fontEffect_ = effects().getEffect("InternalFont");

    // Create internal render targets
    deferredLightingRenderTarget_.create();
    globalPostProcessRenderTarget_.create();
    renderToTextureRenderTarget_.create();
    scenePostProcessRenderTarget_.create();
    shadowMapRenderTarget_.create();

    // Setup the unit rectangle geometry
    createUnitRectangleGeometry();

    // Initialize statistics
    lastFPSTime_ = platform().getTime();
    frameDrawCallCount_ = 0;
    lastDrawCallCount_ = 0;
    frameTriangleCount_ = 0;
    lastTriangleCount_ = 0;
    frameAPICallCount_ = 0;
    lastAPICallCount_ = 0;

    FrameTimers::OnSamplingDataReady.addHandler(this, &Renderer::onFrameTimersSamplingDataReady);

    // Load settings
    shadowMapSize_ = settings().getInteger(ShadowMapSizeSetting);
    showFPS_ = settings().getBoolean(ShowFPSSetting);
    showDebugInfo_ = settings().getBoolean(ShowDebugInfoSetting);
    isHDREnabled_ = settings().getBoolean(HDREnabledSetting);
    frameTimerRenderingEnabled_ = settings().getBoolean(FrameTimerRenderingEnabledSetting);

    // Default shadow map size depends on the device type
    if (!shadowMapSize_)
        shadowMapSize_ = platform().isPhone() ? 512 : 1024;

    return true;
}

bool Renderer::processEvent(const Event& e)
{
    if (e.as<ResizeEvent>())
    {
        // On a resize all render targets and temporary textures are destroyed, they will be recreated on demand as required.
        // This is because these resources often have dimensions that are based on the dimensions of the rendering window, and
        // letting these resources accumulate in the background across resizes would lead to slowly increasing resource usage
        // when lots of resolution switching occurs

        clearTemporaryRenderTargets();
        clearTemporaryTextures();
    }
    else if (auto rwe = e.as<const RecreateWindowEvent>())
    {
        if (rwe->getWindowEventType() == RecreateWindowEvent::CloseWindow)
        {
            deferredLightingRenderTarget_.clear();
            globalPostProcessRenderTarget_.clear();
            renderToTextureRenderTarget_.clear();
            scenePostProcessRenderTarget_.clear();
            shadowMapRenderTarget_.clear();

            clearTemporaryRenderTargets();
            clearTemporaryTextures();

            effects().onRecreateWindowEvent(*rwe);
            dataBuffers().onRecreateWindowEvent(*rwe);
            textures().onRecreateWindowEvent(*rwe);

            graphics().shutdown();

            // Instantiate the new graphics interface
            Globals::recreateGraphicsInterface();
        }
        else if (rwe->getWindowEventType() == RecreateWindowEvent::NewWindow)
        {
            // Setup the graphics interface on the new window
            if (!graphics().setup())
            {
                LOG_ERROR << "Graphics interface doesn't support rendering to the new window";
                return false;
            }

            effects().onRecreateWindowEvent(*rwe);
            dataBuffers().onRecreateWindowEvent(*rwe);
            textures().onRecreateWindowEvent(*rwe);

            deferredLightingRenderTarget_.create();
            globalPostProcessRenderTarget_.create();
            renderToTextureRenderTarget_.create();
            scenePostProcessRenderTarget_.create();
            shadowMapRenderTarget_.create();
        }
    }

    return true;
}

void Renderer::setShowFPS(bool showFPS)
{
    showFPS_ = showFPS;
    settings().set(ShowFPSSetting, showFPS_);
}

void Renderer::setShowDebugInfo(bool showDebugInfo)
{
    showDebugInfo_ = showDebugInfo;
    settings().set(ShowDebugInfoSetting, showDebugInfo_);
}

bool Renderer::setShadowMapSize(unsigned int size)
{
    if (!Math::isPowerOfTwo(size))
    {
        LOG_ERROR << "Shadow map size can't be " << size << ", it must be a power of two";
        return false;
    }

    shadowMapSize_ = size;
    settings().set(ShadowMapSizeSetting, shadowMapSize_);

    // Clear temporary depth textures so they don't accumulate
    clearTemporaryTextures(true);

    return true;
}

void Renderer::createUnitRectangleGeometry()
{
    // Layout: x, y, z, s, t, nx, ny, nz
    auto vertexData = std::array<float, 8 * 8>{{0.0f,  0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 1.0f, 1.0f,  0.0f, 0.0f, 1.0f, 0.0f,
                                                0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f, 1.0f,
                                                0.0f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,
                                                -1.0f, 0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
                                                1.0f,  0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  0.0f, 0.0f, -1.0f}};

    auto indices = Vector<unsigned int>{0, 1, 2, 1, 3, 2, 4, 5, 6, 5, 7, 6};

    unitRectangleGeometry_.clear();

    unitRectangleGeometry_.addVertexStream({VertexStream::Position, 3});
    unitRectangleGeometry_.addVertexStream({VertexStream::DiffuseTextureCoordinate, 2});
    unitRectangleGeometry_.addVertexStream({VertexStream::Normal, 3});
    unitRectangleGeometry_.setVertexCount(8);

    memcpy(unitRectangleGeometry_.lockVertexData(), vertexData.data(), vertexData.size() * sizeof(float));
    unitRectangleGeometry_.unlockVertexData();

    unitRectangleGeometry_.setupIndexData({{GraphicsInterface::TriangleList, indices.size(), 0}}, indices);
    unitRectangleGeometry_.calculateTangentBases();
    unitRectangleGeometry_.registerWithRenderer();
}

const Texture* Renderer::getErrorTexture() const
{
    if (!errorTexture_)
        errorTexture_ = textures().setupTexture(GraphicsInterface::Texture2D, "TextureError.png", "WorldDiffuse");

    return errorTexture_;
}

const Texture* Renderer::getFlatNormalMap() const
{
    if (!errorNormalMap_)
        errorNormalMap_ = textures().setupTexture(GraphicsInterface::Texture2D, "FlatNormalMap.png", "WorldNormal");

    return errorNormalMap_;
}

const Texture* Renderer::getWhiteTexture() const
{
    if (!whiteTexture_)
        whiteTexture_ = textures().setupTexture(GraphicsInterface::Texture2D, "White.png", "WorldDiffuse");

    return whiteTexture_;
}

const Texture* Renderer::getBlackTexture() const
{
    if (!blackTexture_)
        blackTexture_ = textures().setupTexture(GraphicsInterface::Texture2D, "Black.png", "WorldDiffuse");

    return blackTexture_;
}

const Texture* Renderer::getWhiteCubemapTexture() const
{
    if (!whiteCubemapTexture_)
        whiteCubemapTexture_ = textures().create1x1CubemapTexture("WhiteCube", Color::White);

    return whiteCubemapTexture_;
}

const Texture* Renderer::getBlackCubemapTexture() const
{
    if (!blackCubemapTexture_)
        blackCubemapTexture_ = textures().create1x1CubemapTexture("BlackCube", Color::Black);

    return blackCubemapTexture_;
}

const Matrix3& Renderer::getCurrentOrientationInverseMatrix() const
{
    if (!isCurrentOrientationInverseMatrixCached_)
    {
        currentOrientationInverseMatrix_ = getCurrentTransform().getOrientation().getInverse().getMatrix3();
        isCurrentOrientationInverseMatrixCached_ = true;
    }

    return currentOrientationInverseMatrix_;
}

const Matrix4& Renderer::getCurrentTransformInverseMatrix() const
{
    if (!isCurrentTransformInverseMatrixCached_)
    {
        auto matrix = currentTransform_.getMatrix();
        matrix.scale(currentScale_);
        matrix.getInverse(currentTransformInverseMatrix_);

        isCurrentTransformInverseMatrixCached_ = true;
    }

    return currentTransformInverseMatrix_;
}

const Vec3& Renderer::getLocalSpaceCameraPosition() const
{
    if (!isLocalSpaceCameraPositionCached_)
    {
        localSpaceCameraPosition_ = getCurrentTransformInverseMatrix() * getCamera().getPosition();
        isLocalSpaceCameraPositionCached_ = true;
    }

    return localSpaceCameraPosition_;
}

const Matrix4& Renderer::getModelViewProjectionMatrix() const
{
    if (!isModelViewProjectionMatrixCached_)
    {
        modelViewProjectionMatrix_ = getCamera().getProjectionMatrix() * getModelViewMatrix();
        isModelViewProjectionMatrixCached_ = true;
    }

    return modelViewProjectionMatrix_;
}

const Matrix4& Renderer::getScreenProjectionMatrix() const
{
    if (!isScreenProjectionMatrixCached_)
    {
        screenProjectionMatrix_ = Matrix4::Half * getCamera().getProjectionMatrix() * modelViewMatrix_;
        isScreenProjectionMatrixCached_ = true;
    }

    return screenProjectionMatrix_;
}

void Renderer::pushCamera(const Camera& camera)
{
    cameras_.append(&camera);

    States::Viewport = camera.getViewport();
}

bool Renderer::gatherLights(const AABB& localAABB, Vector<const Light*>& lights)
{
    auto worldAABB = AABB();
    worldAABB.merge(localAABB, currentTransform_, currentScale_);

    lights.clear();

    for (auto light : currentLights_)
    {
        if (worldAABB.intersect(light->getLightAABB()))
            lights.append(light);
    }

    return true;
}

void Renderer::queueForRendering(Scene* scene, const Camera& camera, int priority,
                                 GraphicsInterface::OutputDestination outputDestination)
{
    auto i = 0U;

    for (; i < queuedScenes_.size(); i++)
    {
        if (queuedScenes_[i].priority > priority)
            break;
    }

    queuedScenes_.insert(i, QueuedScene(scene, camera, priority, outputDestination));
}

void Renderer::render()
{
    auto timer = ScopedFrameTimer(RendererTimer);

    States::StateCacher::resetGraphicsInterfaceStateUpdateCount();

    // Calculate the FPS
    if (lastFPSTime_.getSecondsSince() >= 1.0f)
    {
        lastFPS_ = frameCount_;
        frameCount_ = 0;
        lastFPSTime_ = platform().getTime();
    }
    else
        frameCount_++;

    // Update FSAA state
    States::MultisampleEnabled = platform().getFSAAMode() != PlatformInterface::FSAANone;

    // Group queued sceens by their output destination
    auto queuedScenesByOutputDestination = std::map<GraphicsInterface::OutputDestination, Vector<QueuedScene*>>();
    for (auto& queuedScene : queuedScenes_)
        queuedScenesByOutputDestination[queuedScene.outputDestination].append(&queuedScene);

    for (auto& outputDestinationWithScenes : queuedScenesByOutputDestination)
    {
        auto outputDestination = outputDestinationWithScenes.first;
        auto& queuedScenes = outputDestinationWithScenes.second;

        auto outputRenderTarget = graphics().getOutputDestinationRenderTarget(outputDestination);
        auto outputViewport = graphics().getOutputDestinationViewport(outputDestination);

        // Set up for global post-processing
        auto isGlobalPostProcessingOn = setupForPostProcessing(globalPostProcessRenderTarget_, globalPostProcessEffects_,
                                                               outputViewport * fullscreenRenderTargetDimensionsScaleFactor_);

        if (!isGlobalPostProcessingOn)
        {
            States::RenderTarget = outputRenderTarget;
            States::Viewport = outputViewport;
        }

        // Render each queued scene for this output destination
        auto clearColorBuffer = true;
        for (auto queuedScene : queuedScenes)
        {
            // Adjust the camera's viewport for any render target dimensions scale factor
            auto camera = queuedScene->camera;
            if (isGlobalPostProcessingOn)
                camera.setViewport(camera.getViewport() * fullscreenRenderTargetDimensionsScaleFactor_);

            draw(queuedScene->scene, camera, clearColorBuffer, clearColorBuffer || queuedScene->scene->isDepthClearEnabled(),
                 true);

            clearColorBuffer = false;
        }

        // Check there are no post-process pass throughs dangling
        checkPostProcessPassThroughsCompleted(scenePostProcessRenderTarget_);

        // Apply global post-processing if present
        if (isGlobalPostProcessingOn)
            drawPostProcess(globalPostProcessRenderTarget_, globalPostProcessEffects_, outputRenderTarget, outputViewport);

        // The contents of the depth and stencil buffers can now be discarded
        graphics().discardRenderTargetBuffers(false, true, true);

        graphics().flushOutputDestination(outputDestination);
    }

    queuedScenes_.clear();

    // Set to default output
    States::RenderTarget = graphics().getOutputDestinationRenderTarget(GraphicsInterface::OutputDefault);
    States::Viewport = graphics().getOutputDestinationViewport(GraphicsInterface::OutputDefault);

    // Clear the default output if no scenes were rendered to it
    if (queuedScenesByOutputDestination.find(GraphicsInterface::OutputDefault) == queuedScenesByOutputDestination.end())
        graphics().clearBuffers(true, true, true);

    drawDebugOverlays();

    {
        ScopedFrameTimer swapTimer(SwapBuffersTimer);
        platform().swap();
    }

    printRenderQueues_ = false;

    // Calculate frame statistics based on the graphics interface counters
    frameDrawCallCount_ = graphics().getDrawCallCount() - lastDrawCallCount_;
    lastDrawCallCount_ = graphics().getDrawCallCount();
    frameTriangleCount_ = graphics().getTriangleCount() - lastTriangleCount_;
    lastTriangleCount_ = graphics().getTriangleCount();
    frameAPICallCount_ = graphics().getAPICallCount() - lastAPICallCount_;
    lastAPICallCount_ = graphics().getAPICallCount();
}

bool Renderer::renderIntoTexture(Scene* scene, const Vector<Camera>& cameras, Texture2D* texture)
{
    auto timer = ScopedFrameTimer(RendererTimer);

    if (!scene || !texture || !renderToTextureRenderTarget_)
        return false;

    // Get a depth/stencil texture to use if this is a 3D scene
    auto depthStencilTexture = pointer_to<const Texture>::type();
    if (scene->isDepthTestEnabled())
        depthStencilTexture = requestTemporaryTexture(texture->getRect(), Image::Depth24Stencil8);

    // Prepare the render target
    if (!renderToTextureRenderTarget_.setTextures(texture, depthStencilTexture, depthStencilTexture) ||
        !renderToTextureRenderTarget_.isValid())
    {
        releaseTemporaryTexture(depthStencilTexture);
        return false;
    }

    States::StateCacher::push();
    {
        States::RenderTarget = renderToTextureRenderTarget_;

        for (auto i = 0U; i < cameras.size(); i++)
        {
            // Only clear buffers when rendering for the first camera
            auto clearBuffers = (i == 0);

            draw(scene, cameras[i], clearBuffers, clearBuffers);
        }

        // The contents of the depth and stencil buffers can now be discarded
        graphics().discardRenderTargetBuffers(false, true, true);
    }
    States::StateCacher::pop();

    // Release the depth/stencil texture that was used
    releaseTemporaryTexture(depthStencilTexture);

    return true;
}

bool Renderer::renderIntoTexture(Scene* scene, const Vector<Camera>& cameras, TextureCubemap* texture)
{
    auto timer = ScopedFrameTimer(RendererTimer);

    if (!scene || !texture || !cameras.size() || !renderToTextureRenderTarget_)
        return false;

    // These are the camera orientations needed to render each of the six cubemap faces, the order is +X, -X, +Y, -Y, +Z, -Z
    static const auto cubemapFaceOrientations = std::array<Matrix3, 6>{
        {Matrix3::getRotationZ(Math::Pi) * Matrix3::getRotationY(Math::HalfPi),
         Matrix3::getRotationZ(Math::Pi) * Matrix3::getRotationY(-Math::HalfPi), Matrix3::getRotationX(Math::HalfPi),
         Matrix3::getRotationX(-Math::HalfPi), Matrix3::getRotationX(Math::Pi), Matrix3::getRotationZ(Math::Pi)}};

    // Get a depth/stencil texture to use
    auto depthStencilTexture = requestTemporaryTexture(texture->getRect(), Image::Depth24Stencil8);

    // Loop over the six cubemap faces
    for (auto i = 0U; i < cubemapFaceOrientations.size(); i++)
    {
        // Prepare the render target to render into this cubemap face
        if (!renderToTextureRenderTarget_.setTextures(texture, i, depthStencilTexture, depthStencilTexture) ||
            !renderToTextureRenderTarget_.isValid())
        {
            releaseTemporaryTexture(depthStencilTexture);
            return false;
        }

        // Draw this face of the cubemap
        States::StateCacher::push();
        {
            States::RenderTarget = renderToTextureRenderTarget_;

            for (auto j = 0U; j < cameras.size(); j++)
            {
                // Override camera orientation as needed for this cubemap face
                auto camera = cameras[j];
                camera.setOrientation(camera.getOrientation() * cubemapFaceOrientations[i]);

                // Only clear buffers when rendering for the first camera
                auto clearBuffers = (j == 0);

                draw(scene, camera, clearBuffers, clearBuffers);
            }

            // The contents of the depth and stencil buffers can now be discarded
            graphics().discardRenderTargetBuffers(false, true, true);
        }
        States::StateCacher::pop();
    }

    // Release the depth/stencil texture that was used
    releaseTemporaryTexture(depthStencilTexture);

    return true;
}

void Renderer::draw(Scene* scene, const Camera& camera, bool clearColorBuffer, bool clearDepthStencilBuffer,
                    bool allowPostProcessPassThrough, unsigned int recursionDepth)
{
    const auto maximumRecursionDepth = 1U;

    // Build a culling frustum for this camera
    auto frustum = ConvexHull(camera.getProjectionMatrix(), camera.getViewMatrix());

    // Gather the geometry to render from the scene
    auto queues = EffectQueueArray();
    scene->gatherGeometry(camera.getPosition(), frustum, queues);

    if (printRenderQueues_)
    {
        LOG_DEBUG << "--------------------------------------";
        LOG_DEBUG << "Gathered render queues for scene '" << scene->getName() << "':";
        LOG_DEBUG << "    Camera:";
        LOG_DEBUG << "        Position:    " << camera.getPosition();
        LOG_DEBUG << "        Orientation: " << camera.getOrientation();
        LOG_DEBUG << "        View matrix: " << camera.getViewMatrix();
        LOG_DEBUG << "        Viewport:    " << camera.getViewport();
        LOG_DEBUG << "        Projection:  " << camera.getProjectionMatrix();
        queues.debugTrace();
    }

    // Sort the gathered geometry and split it into different groups, the required reflection planes are put into a vector
    auto normalGeometry = Vector<EffectQueue*>();
    auto refractiveGeometry = Vector<EffectQueue*>();
    auto reflectionPlanes = Vector<Plane>();
    sortEffectQueues(queues, camera, normalGeometry, refractiveGeometry, reflectionPlanes);

    // Resources used in this method are put into these vectors so that they can be released at the end of this method
    auto allocatedRenderTargets = Vector<const RenderTarget*>();
    auto allocatedTemporaryTextures = Vector<const Texture*>();

    // Limit recursion of reflections
    if (recursionDepth >= maximumRecursionDepth)
        reflectionPlanes.clear();

    States::DepthTestEnabled = scene->isDepthTestEnabled();

    // Iterate over each reflection plane and render the corresponding reflection texture
    for (auto& plane : reflectionPlanes)
    {
        // Try and allocate a render target for the reflection
        auto target = requestTemporaryRenderTarget(camera.getViewport());
        if (!target)
            continue;

        allocatedRenderTargets.append(target);

        // Prepare a reflected camera
        auto reflectedCamera = camera;
        reflectedCamera.reflectInPlane(plane);
        reflectedCamera.setViewport(target->getColorTexture()->getRect());

        // Render the reflection into the render target by recursively calling this method
        States::StateCacher::push();
        {
            States::RenderTarget = *target;

            // Flip cull mode
            if (States::CullMode == States::CullFrontFaces)
                States::CullMode = States::CullBackFaces;
            else if (States::CullMode == States::CullBackFaces)
                States::CullMode = States::CullFrontFaces;

            auto clearBuffers = true;

            draw(scene, reflectedCamera, clearBuffers, clearBuffers, false, recursionDepth + 1);
        }
        States::StateCacher::pop();

        reflectionTexture_ = target->getColorTexture();
    }

    // Get all the lights that lie in the current culling frustum
    scene->gatherLights(frustum, currentLights_, ambientLightColor_);
    ambientLightColor_.a = 0.0f;

    pushCamera(camera);

    // The first directional light encountered is taken as the primary directional light for this scene
    directionalLightDirection_ = -Vec3::UnitY;
    directionalLightColor_ = Color::Black;
    for (auto light : currentLights_)
    {
        if (light->isDirectionalLight())
        {
            directionalLightDirection_ = light->getLightTransform().getDirection();
            directionalLightColor_ = light->getColor();
            break;
        }
    }

    // Render deferred lighting texture if required
    auto isDeferredLightingOn = renderDeferredLightingTexture(scene, frustum, normalGeometry, allocatedTemporaryTextures);

    // Setup for post-processing if required, post-processing is disabled when rendering reflections
    auto postProcessFinalRenderTargetObject = States::RenderTarget.get();
    auto postProcessFinalViewport = States::Viewport.get();

    auto isPostProcessingOn =
        recursionDepth == 0 && setupForPostProcessing(scenePostProcessRenderTarget_, scene->getPostProcessEffects(),
                                                      getCamera().getViewport(), &clearColorBuffer);

    // Clear buffers
    graphics().clearBuffers(clearColorBuffer, clearDepthStencilBuffer, clearDepthStencilBuffer);

    // Render the 'normal' (i.e. non-refractive) geometry
    if (!isDeferredLightingOn)
        drawEffectQueues(normalGeometry);
    else
    {
        // When doing deferred lighting blended geometry is drawn in a separate forward rendering pass with deferred lighting
        // turned off

        drawEffectQueues(normalGeometry, SkipBlendedGeometry);

        auto oldDeferredLightingTexture = deferredLightingTexture_;
        deferredLightingTexture_ = nullptr;
        drawEffectQueues(normalGeometry, OnlyDrawBlendedGeometry);
        deferredLightingTexture_ = oldDeferredLightingTexture;
    }

    // If there is any geometry that requires a refraction texture then copy the backbuffer into a temporary texture and use
    // that as the refraction texture
    if (refractiveGeometry.size())
    {
        const auto& textureRect = States::Viewport.get();
        refractionTexture_ = requestTemporaryTexture(textureRect, Image::RGB8, TextureProperties::BilinearFilter);

        // If a refraction texture is available then copy into it
        if (refractionTexture_)
        {
            graphics().copyBackbufferTo2DTexture(refractionTexture_->getActiveTextureObject(), 0,
                                                 refractionTexture_->getRect() +
                                                     Vec2(textureRect.getLeft(), textureRect.getBottom()));
        }

        // Draw the geometry that requires the refraction texture
        drawEffectQueues(refractiveGeometry);

        // Free the refraction texture
        releaseTemporaryTexture(refractionTexture_);
        refractionTexture_ = nullptr;
    }

    // If this scene is using post-processing then pass off to drawPostProcess()
    if (isPostProcessingOn)
    {
        drawPostProcess(scenePostProcessRenderTarget_, scene->getPostProcessEffects(), postProcessFinalRenderTargetObject,
                        postProcessFinalViewport, allowPostProcessPassThrough && scene->isPostProcessPassThroughEnabled());
    }

    if (isDeferredLightingOn)
        deferredLightingTexture_ = nullptr;

    // Release any render targets and temporary textures used during this render
    for (auto target : allocatedRenderTargets)
        releaseTemporaryRenderTarget(target);
    for (auto texture : allocatedTemporaryTextures)
        releaseTemporaryTexture(texture);

    reflectionTexture_ = nullptr;
    refractionTexture_ = nullptr;

    popCamera();
}

void Renderer::sortEffectQueues(EffectQueueArray& queues, const Camera& camera, Vector<EffectQueue*>& normalGeometry,
                                Vector<EffectQueue*>& refractiveGeometry, Vector<Plane>& reflectionPlanes)
{
    if (!queues.size())
        return;

    auto timer = ScopedFrameTimer(RendererSortTimer);

    auto currentPriority = queues[0]->getPriority();

    // When the priority changes the unblended and blended queues are put onto the end of the normalGeometry queue, however
    // while the priority isn't changing they need to be gathered in separate lists so that blended geometry can be put after
    // the unblended geometry in the final queue. Priority ordering therefore overrides the unblended/blended sorting that
    // occurs with queues that are of the same priority.

    auto unblendedQueues = Vector<EffectQueue*>();
    auto blendedQueues = Vector<EffectQueue*>();

    for (auto i = 0U; i <= queues.size(); i++)
    {
        // If the priority is changing or this is the end of the queues vector then append the unblended and blended queues to
        // the end of the normalGeometry queue
        if (i == queues.size() || queues[i]->getPriority() != currentPriority)
        {
            normalGeometry.append(unblendedQueues);
            normalGeometry.append(blendedQueues);

            if (i == queues.size())
                break;

            unblendedQueues.resize(0);
            blendedQueues.resize(0);
        }

        auto queue = queues[i];

        // Update the current priority
        currentPriority = queue->getPriority();

        // Get the shader for this queue's effect along with the sorting key to use
        auto shader = pointer_to<const Shader>::type();
        if (queue->getEffect())
        {
            shader = queue->getEffect()->getActiveShader();
            if (!shader)
                continue;

            // Get the sorting key for this set of parameters
            queue->setSortKey(shader->getSortKey(queue->getParams(), queue->getInternalParams()));
        }
        else
            queue->setSortKey(0);

        // This queue needs to be put into a sensible place in one of the queue vectors. The queue vector to insert into is
        // chosen based on the shader type (e.g. blended, refractive, etc...)

        auto queueVector = pointer_to<Vector<EffectQueue*>>::type();

        if (!shader)
            queueVector = &blendedQueues;
        else
        {
            switch (shader->getShaderType(queue->getParams(), queue->getInternalParams()))
            {
                case Shader::Framebuffer:
                    queueVector = &refractiveGeometry;
                    break;

                case Shader::Blended:
                    queueVector = &blendedQueues;
                    break;

                case Shader::Reflection:
                    queueVector = &unblendedQueues;

                case Shader::RefractionReflection:
                    if (!queueVector)
                        queueVector = &refractiveGeometry;

                    // This shader requires a reflection texture, so work out the required reflection plane for it and put it
                    // into the reflectionPlanes vector
                    getReflectionPlaneIndex(queue->getItems(), camera, reflectionPlanes);

                    break;

                default:
                    queueVector = &unblendedQueues;
                    break;
            }
        }

        // Now that the queue vector to put this queue into has been chosen the queue needs to actually be inserted. In order to
        // reduce the number of effect changes when rendering, the queue is inserted next to others that use the same effect.
        // Each queue's sortKey value is used to do more fine grained sorting of the queues, this sorting is based entirely on
        // the return value from Shader::getSortKey().

        for (auto j = 0U; j < queueVector->size(); j++)
        {
            if (queueVector->at(j)->getEffect() == queue->getEffect())
            {
                // This queue's effect is already used by a queue in this queue vector, insert it into the optimal place based
                // on the sortKey values
                for (auto k = j; k < queueVector->size(); k++)
                {
                    if (queueVector->at(k)->getEffect() != queue->getEffect() ||
                        queueVector->at(k)->getSortKey() >= queue->getSortKey())
                    {
                        // Insert the queue and break out
                        queueVector->insert(k, queue);
                        queueVector = nullptr;
                        break;
                    }
                }

                if (!queueVector)
                    break;
            }
        }

        // If queueVector is still set then the queue wasn't inserted by the above loop, this will only happen if its effect
        // isn't currently used by an existing queue. In this case just append the queue to the queue vector.
        if (queueVector)
            queueVector->append(queue);
    }
}

int Renderer::getReflectionPlaneIndex(const RenderQueueItemArray& items, const Camera& camera,
                                      Vector<Plane>& reflectionPlanes) const
{
    // The reflection plane is found by looking at the first chunk in the given queue and calling GeometryChunk::getPlane() on
    // it. This means that only one reflection plane can be used per geometry chunk, but this limitation isn't currently an
    // issue.

    // The world space plane is found by transforming the geometry chunk's plane by the most recent ChangeTransform queue item.

    auto lastTransform = pointer_to<const ChangeTransformRenderQueueItem>::type();

    for (auto j = 0U; j < items.size(); j++)
    {
        if (auto transformItem = items[j].as<ChangeTransformRenderQueueItem>())
            lastTransform = transformItem;
        else if (auto drawChunkItem = items[j].as<DrawGeometryChunkRenderQueueItem>())
        {
            // Get the transform matrix
            auto matrix = lastTransform->getTransform().getMatrix();

            // Compute this chunk's world space plane
            auto plane = matrix * drawChunkItem->getGeometryChunk().getPlane();

            // Check the camera is in front of the plane as there's no point in doing the reflection if it's behind the plane
            if (plane.classify(camera.getPosition()) == Plane::Front)
            {
                // See if this plane or one very similar to it is already in the reflectionPlanes vector and if so then just use
                // that one
                for (auto i = 0U; i < reflectionPlanes.size(); i++)
                {
                    if (reflectionPlanes[i].getNormal().dot(plane.getNormal()) + Math::Epsilon > 1.0f &&
                        fabsf(reflectionPlanes[i].getDistance() - plane.getDistance()) < Math::Epsilon)
                        return i;
                }

                // Add the new reflection plane
                reflectionPlanes.append(plane);

                return reflectionPlanes.size() - 1;
            }
        }
    }

    return -1;
}

void Renderer::Camera::reflectInPlane(const Plane& plane)
{
    // Reflect the camera position
    position_ = plane.reflect(position_);

    // Reflect the camera orientation matrix
    orientation_ = Matrix4::getReflection(plane).getMatrix3() * orientation_;

    updateViewMatrix();

    // Modify the projection matrix so that the near clip plane lies on the reflection plane, this clips off everything that
    // lies behind the reflection plane
    projectionMatrix_.modifyProjectionMatrix(viewMatrix_ * plane);
}

void Renderer::Camera::setOrientation(const Matrix3& matrix)
{
    orientation_ = matrix;

    updateViewMatrix();
}

void Renderer::Camera::updateViewMatrix()
{
    viewMatrix_[0] = orientation_[0];
    viewMatrix_[1] = orientation_[3];
    viewMatrix_[2] = orientation_[6];
    viewMatrix_[3] = 0.0f;
    viewMatrix_[4] = orientation_[1];
    viewMatrix_[5] = orientation_[4];
    viewMatrix_[6] = orientation_[7];
    viewMatrix_[7] = 0.0f;
    viewMatrix_[8] = orientation_[2];
    viewMatrix_[9] = orientation_[5];
    viewMatrix_[10] = orientation_[8];
    viewMatrix_[11] = 0.0f;
    viewMatrix_[12] = orientation_[0] * -position_.x + orientation_[1] * -position_.y + orientation_[2] * -position_.z;
    viewMatrix_[13] = orientation_[3] * -position_.x + orientation_[4] * -position_.y + orientation_[5] * -position_.z;
    viewMatrix_[14] = orientation_[6] * -position_.x + orientation_[7] * -position_.y + orientation_[8] * -position_.z;
    viewMatrix_[15] = 1.0f;
}

bool Renderer::setPostProcessIntermediateTargetTexture(const Texture* texture)
{
    if (!activePostProcessRenderTarget_)
    {
        LOG_ERROR << "There is no active post-process render target, this method should not be called";
        return false;
    }

    States::Viewport = texture->getRect();

    return activePostProcessRenderTarget_->setColorTexture(texture);
}

bool Renderer::isHDRSupported() const
{
    return graphics().isPixelFormatSupported(Image::RGBA16f, GraphicsInterface::Texture2D) &&
        graphics().isNonPowerOfTwoTextureSupported(GraphicsInterface::Texture2D) && graphics().isRenderTargetSupported();
}

bool Renderer::setHDREnabled(bool enabled)
{
    if (enabled == isHDREnabled_)
        return true;

    if (enabled && !isHDRSupported())
    {
        LOG_ERROR << "HDR is not supported on this hardware";
        return false;
    }

    isHDREnabled_ = enabled;
    settings().set(HDREnabledSetting, isHDREnabled_);

    // Clear out render targets and refraction textures as these may have been created based on the HDR enabled setting
    clearTemporaryRenderTargets();
    clearTemporaryTextures();

    return true;
}

void Renderer::addDebugString(const UnicodeString& s)
{
    if (showDebugInfo_)
        debugStrings_.append(s);
}

void Renderer::setDebugTexture(const String& name, unsigned int frame, unsigned int mipmap, bool renderAlpha, float scale)
{
    debugTexture_.name.clear();
    debugTexture_.frame = 0;

    if (!name.length())
        return;

    auto texture = textures().getTexture(name);
    if (!texture)
    {
        LOG_ERROR << "Unknown texture: " << name;
        return;
    }

    texture->ensureImageIsLoaded();

    // Only 2D and cubemap textures are supported
    if ((texture->getTextureType() != GraphicsInterface::Texture2D &&
         texture->getTextureType() != GraphicsInterface::TextureCubemap) ||
        !graphics().isTextureSupported(texture->getTextureType(), texture->getImage()))
    {
        LOG_ERROR << "Can't debug this texture type";
        return;
    }

    debugTexture_.name = name;
    debugTexture_.frame = frame;
    debugTexture_.mipmap = mipmap;
    debugTexture_.scale = scale;
    debugTexture_.renderAlpha = renderAlpha;
}

void Renderer::clearTemporaryTextures(bool depthTexturesOnly)
{
    for (auto i = 0U; i < temporaryTextures_.size(); i++)
    {
        if (depthTexturesOnly && !Image::isPixelFormatDepthAware(temporaryTextures_[i].texture->getPixelFormat()))
            continue;

        if (temporaryTextures_[i].inUse)
            LOG_WARNING << "Temporary texture is currently in use: " << temporaryTextures_[i].texture;

        textures().releaseTexture(temporaryTextures_[i].texture);

        temporaryTextures_.erase(i--);
    }
}

const Texture* Renderer::requestTemporaryTexture(unsigned int width, unsigned int height, Image::PixelFormat pixelFormat,
                                                 TextureProperties::TextureFilter filter)
{
    assert(width && height && "Requested a temporary texture with zero area");

    // NPOT texture support is required for NPOT temporary textures
    if ((!Math::isPowerOfTwo(width) || !Math::isPowerOfTwo(height)) &&
        !graphics().isNonPowerOfTwoTextureSupported(GraphicsInterface::Texture2D))
        return nullptr;

    if (pixelFormat == Image::Depth24Stencil8 && !graphics().isStencilBufferSupported())
        pixelFormat = Image::Depth;

    // Force nearest filtering on HDR and depth images at this stage
    if (pixelFormat == Image::Depth || pixelFormat == Image::Depth24Stencil8 || Image::isPixelFormatFloatingPoint(pixelFormat))
        filter = TextureProperties::NearestFilter;

    // Find a temporary texture to use
    for (auto& temporaryTexture : temporaryTextures_)
    {
        if (temporaryTexture.texture->getImage().getWidth() == width &&
            temporaryTexture.texture->getImage().getHeight() == height &&
            temporaryTexture.texture->getPixelFormat() == pixelFormat &&
            temporaryTexture.texture->getProperties().getFilter() == filter && !temporaryTexture.inUse)
        {
            temporaryTexture.inUse = true;
            return temporaryTexture.texture;
        }
    }

    // Create a new temporary texture
    auto temporaryTexture = textures().create2DTexture();

    auto image = Image();
    if (!image.initialize(width, height, 1, pixelFormat, false, 1) ||
        !temporaryTexture->load(String() + ".Renderer" + (Image::isPixelFormatDepthAware(pixelFormat) ? "Depth" : "Color") +
                                    temporaryTextures_.size(),
                                std::move(image)) ||
        !temporaryTexture->upload())
    {
        textures().releaseTexture(temporaryTexture);
        LOG_ERROR << "Failed creating temporary texture of size " << width << "x" << height;
        return nullptr;
    }

    temporaryTexture->setProperties(TextureProperties(filter));

    temporaryTextures_.emplace(temporaryTexture, true);

    LOG_INFO << "Created temporary texture '" << temporaryTexture->getName() << "': " << temporaryTexture->getWidth() << "x"
             << temporaryTexture->getHeight() << " " << Image::getPixelFormatString(temporaryTexture->getPixelFormat());

    return temporaryTexture;
}

const Texture* Renderer::requestTemporaryTexture(unsigned int width, unsigned int height, bool includeAlpha, bool forceHDR,
                                                 TextureProperties::TextureFilter filter)
{
    // Choose the pixel format
    auto pixelFormat = Image::PixelFormat();
    if (isHDREnabled_ || forceHDR)
        pixelFormat = includeAlpha ? Image::RGBA16f : Image::RGB16f;
    else
        pixelFormat = includeAlpha ? Image::RGBA8 : Image::RGB8;

    return requestTemporaryTexture(width, height, pixelFormat, filter);
}

void Renderer::releaseTemporaryTexture(const Texture* texture)
{
    if (!texture)
        return;

    for (auto& temporaryTexture : temporaryTextures_)
    {
        if (temporaryTexture.texture == texture)
        {
            if (!temporaryTexture.inUse)
                LOG_WARNING << "Temporary texture is not currently in use: " << texture->getName();

            temporaryTexture.inUse = false;
            return;
        }
    }

    LOG_ERROR << "Unknown temporary texture: " << texture->getName();
}

const RenderTarget* Renderer::requestTemporaryRenderTarget(unsigned int width, unsigned int height)
{
    if (!graphics().isRenderTargetSupported())
        return nullptr;

    // If the request is for an NPOT render target then check NPOT is supported
    if ((!Math::isPowerOfTwo(width) || !Math::isPowerOfTwo(height)) &&
        !graphics().isNonPowerOfTwoTextureSupported(GraphicsInterface::Texture2D))
        return nullptr;

    // Determine the pixel format for the color texture
    auto colorPixelFormat = isHDREnabled_ ? Image::RGBA16f : Image::RGBA8;

    // Loop over all existing render targets to see if any of them can be used to service this request
    for (auto target : temporaryRenderTargets_)
    {
        auto& image = target->getColorTexture()->getImage();

        if (!target->inUse_ && image.getWidth() == width && image.getHeight() == height &&
            image.getPixelFormat() == colorPixelFormat)
        {
            target->inUse_ = true;
            return target;
        }
    }

    // Try and create a new render target to fill this request
    auto colorTexture = pointer_to<Texture2D>::type();
    auto depthStencilTexture = pointer_to<Texture2D>::type();
    auto target = pointer_to<RenderTarget>::type();

    try
    {
        // Create a color texture for this render target
        colorTexture = textures().create2DTexture();
        auto colorTextureImage = Image();
        if (!colorTextureImage.initialize(width, height, 1, colorPixelFormat, false, 1) ||
            !colorTexture->load(String() + ".RenderTargetColor" + temporaryRenderTargets_.size(),
                                std::move(colorTextureImage)) ||
            !colorTexture->upload())
            throw Exception("Failed creating depth texture");

        // Nearest filtering on HDR images, otherwise bilinear
        colorTexture->setProperties(isHDREnabled_ ? TextureProperties::NearestFilter : TextureProperties::BilinearFilter);

        // Create a depth/stencil texture for this render target, stencil is only included if the graphics backend supports it
        auto depthStencilTextureImage = Image();
        if (!depthStencilTextureImage.initialize(
                width, height, 1, graphics().isStencilBufferSupported() ? Image::Depth24Stencil8 : Image::Depth, false, 1))
            throw Exception("Failed initializing depth texture image");

        depthStencilTexture = textures().create2DTexture();
        if (!depthStencilTexture->load(String() + ".RenderTargetDepthStencil" + temporaryRenderTargets_.size(),
                                       std::move(depthStencilTextureImage)) ||
            !depthStencilTexture->upload())
            throw Exception("Failed creating depth texture");

        // Create a new render target
        target = new RenderTarget;
        if (!target->create() || !target->setTextures(colorTexture, depthStencilTexture, depthStencilTexture))
            throw Exception("Failed setting render target textures");
        if (!target->isValid())
            throw Exception("The new render target is invalid");

        temporaryRenderTargets_.append(target);

        LOG_INFO << "Created temporary render target, size: " << width << "x" << height;

        return target;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        delete target;
        target = nullptr;

        textures().releaseTexture(depthStencilTexture);
        textures().releaseTexture(colorTexture);

        return nullptr;
    }
}

void Renderer::releaseTemporaryRenderTarget(const RenderTarget* target)
{
    if (target && target->inUse_)
        target->inUse_ = false;
}

void Renderer::clearTemporaryRenderTargets()
{
    for (auto target : temporaryRenderTargets_)
    {
        if (target->inUse_)
            LOG_WARNING << "Deleting a target that is in use";

        textures().releaseTexture(target->getColorTexture());
        textures().releaseTexture(target->getDepthTexture());

        delete target;
        target = nullptr;
    }

    temporaryRenderTargets_.clear();
}

void Renderer::setFrameTimerRenderingEnabled(bool enabled)
{
    frameTimerRenderingEnabled_ = enabled;
    settings().set(FrameTimerRenderingEnabledSetting, frameTimerRenderingEnabled_);
}

}
