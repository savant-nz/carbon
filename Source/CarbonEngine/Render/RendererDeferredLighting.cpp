/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Graphics/States/StateCacher.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Platform/FrameTimers.h"
#include "CarbonEngine/Render/Effect.h"
#include "CarbonEngine/Render/EffectQueue.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"

namespace Carbon
{

bool Renderer::isDeferredLightingSupported() const
{
    // Deferred lighting requires NPOT support, render target support, and successful setup of the deferred lighting
    // effects

    return graphics().isNonPowerOfTwoTextureSupported(GraphicsInterface::Texture2D) &&
        graphics().isRenderTargetSupported() && deferredLightingRenderTarget_ && shadowMapRenderTarget_ &&
        deferredLightingSetupEffect_ && deferredLightingSetupEffect_->isActiveShaderReady() &&
        deferredLightingDirectionalLightEffect_ && deferredLightingDirectionalLightEffect_->isActiveShaderReady() &&
        deferredLightingPointLightEffect_ && deferredLightingPointLightEffect_->isActiveShaderReady() &&
        deferredLightingSurfaceEffect_ && deferredLightingSurfaceEffect_->isActiveShaderReady();
}

bool Renderer::renderDeferredLightingTexture(Scene* scene, const ConvexHull& frustum,
                                             const Vector<EffectQueue*>& normalGeometry,
                                             Vector<const Texture*>& allocatedTemporaryTextures)
{
    if (!scene->isDeferredLightingEnabled() || !isDeferredLightingSupported())
        return false;

    // The algorithm currently requires 3 offscreen render targets, two color and one depth. One color texture is used
    // for world space normals, and the other is used to accumulate lighting information. The first pass lays down the
    // world space normals and a depth buffer. The second pass accumulates lighting information for each light into a
    // lighting texture which is then used by the final surface shaders as an input to the final surface color.

    // Request the temporary textures that are required for deferred lighting
    auto normalsTexture = requestTemporaryTexture(getCamera().getViewport(), Image::RGBA8);
    auto depthTexture = requestTemporaryTexture(getCamera().getViewport(), Image::Depth);
    auto lightTexture = requestTemporaryTexture(getCamera().getViewport(), true, false);

    // Put the textures on the list of those to free at the end of this render
    allocatedTemporaryTextures.append(normalsTexture);
    allocatedTemporaryTextures.append(depthTexture);
    allocatedTemporaryTextures.append(lightTexture);

    if (!normalsTexture || !depthTexture || !lightTexture)
    {
        LOG_ERROR << "Failed allocating the temporary textures required for deferred lighting";
        return false;
    }

    // Setup the render target ready for laying down depth and normals
    if (!deferredLightingRenderTarget_.setTextures(normalsTexture, depthTexture, nullptr) ||
        !deferredLightingRenderTarget_.isValid())
    {
        LOG_ERROR << "Failed setting up render target for deferred lighting";
        return false;
    }

    States::StateCacher::push();

    // First pass: render opaque geometry in order to lay down world space normals and a depth buffer
    States::Viewport = normalsTexture->getRect();
    States::RenderTarget = deferredLightingRenderTarget_;
    graphics().clearBuffers(true, true, true);
    drawEffectQueues(normalGeometry, SkipBlendedGeometry, deferredLightingSetupEffect_);

    // Second pass: use the textures created in the first pass to accumulate lighting information into the lighting
    // texture At the moment each light is drawn via a fullscreen quad, drawing point lights and spot lights as geometry
    // would be more fill-rate efficient

    // Setup for rendering into the light texture
    deferredLightingRenderTarget_.setColorTexture(lightTexture);

    States::DepthTestEnabled = false;
    States::DepthWriteEnabled = false;

    // Clear the light texture to the ambient color
    States::ClearColor = ambientLightColor_;
    graphics().clearBuffers(true, false, false);

    // Additively blend the lights into the light texture
    States::BlendEnabled = true;
    States::BlendFunction = States::BlendFunctionSetup(States::One, States::One);

    // Prepare the parameters that will be passed to the shaders used to render the lighting texture
    auto params = ParameterArray();
    params[Parameter::depthTexture].setPointer(depthTexture);
    params[Parameter::normalMap].setPointer(normalsTexture);

    // Used with shadow maps
    auto lightViewProjectionMatrix = Matrix4();
    params[Parameter::lightViewProjectionMatrix].setPointer(&lightViewProjectionMatrix);

    // Loop over all the lights and accumulate their contributions in the lighting texture using the appropriate shader
    auto currentShader = pointer_to<Shader>::type();
    for (const auto light : currentLights_)
    {
        params[Parameter::lightColor] = light->getColor();
        params[Parameter::lightDirection] = light->getLightTransform().getDirection();

        params[Parameter::isSpecularEnabled].setBoolean(light->isSpecularEnabled());

        // The specular intensity used for rendering is the light's specular intensity multiplied by the square of the
        // luminance of the light color
        if (light->isSpecularEnabled())
        {
            params[Parameter::specularIntensity].setFloat(light->getSpecularIntensity() *
                                                          powf(light->getColor().getRGBLuminance(), 2.0f));
        }

        auto nextShader = pointer_to<Shader>::type();

        if (light->isPointLight() || light->isSpotLight())
        {
            // Set a light scissor rectangle around the light sphere to reduce fill rate consumption by the full-screen
            // quad
            auto rect = getCamera().getProjectionMatrix().getProjectedSphereBounds(
                getCamera().getViewMatrix() * light->getLightTransform().getPosition(), light->getRadius(),
                getCamera().getNearPlaneDistance());
            if (rect.getTop() <= rect.getBottom() || rect.getRight() <= rect.getLeft())
                continue;

            States::ScissorEnabled = true;
            States::ScissorRectangle = (rect + Vec2::One) * Vec2::Half * States::Viewport.get().getSize();

            if (light->isSpotLight())
            {
                params[Parameter::minimumConeAngle].setFloat(light->getMinimumConeAngle());

                // Calculate the spot light's view-projection matrix
                lightViewProjectionMatrix =
                    light->getProjectionMatrix() * light->getLightTransform().getInverse().getMatrix();
            }
            else
                params.remove(Parameter::minimumConeAngle);

            if (light->isSpotLight() || light->getProjectionTexture())
                params[Parameter::maximumConeAngle].setFloat(light->getMaximumConeAngle());
            else
                params.remove(Parameter::maximumConeAngle);

            params[Parameter::lightPosition].setVec3(light->getLightTransform().getPosition());
            params[Parameter::lightOrientation].setQuaternion(light->getLightTransform().getOrientation());
            params[Parameter::lightRadius].setFloat(light->getRadius());

            if (light->getProjectionTexture())
                params[Parameter::projectionTexture].setPointer(light->getProjectionTexture());
            else
                params.remove(Parameter::projectionTexture);

            if (light->getProjectionCubemapTexture())
                params[Parameter::projectionCubemap].setPointer(light->getProjectionCubemapTexture());
            else
                params.remove(Parameter::projectionCubemap);

            nextShader = deferredLightingPointLightEffect_->getActiveShader();
        }
        else if (light->isDirectionalLight())
        {
            States::ScissorEnabled = false;

            nextShader = deferredLightingDirectionalLightEffect_->getActiveShader();
        }

        // Shadow maps are supported on directional and spot lights, render one if it is needed
        auto shadowMap = pointer_to<const Texture>::type();
        if (light->isShadowingEnabled() && areShadowMapsSupported())
        {
            if (light->isDirectionalLight())
                shadowMap = renderDirectionalShadowMap(scene, light, lightViewProjectionMatrix);
            else if (light->isSpotLight())
                shadowMap = renderSpotShadowMap(scene, light, lightViewProjectionMatrix);
        }
        if (shadowMap)
        {
            allocatedTemporaryTextures.append(shadowMap);
            params[Parameter::shadowMap].setPointer(shadowMap);
        }
        else
            params.remove(Parameter::shadowMap);

        // Add conversion to 0-1 range to the light's view projection matrix so that shaders don't have to do it
        if (shadowMap || light->isSpotLight())
            lightViewProjectionMatrix = Matrix4::Half * lightViewProjectionMatrix;

        // Update the active lighting shader if required
        if (currentShader != nextShader)
        {
            if (currentShader)
                currentShader->exitShader();

            currentShader = nextShader;
            currentShader->enterShader();
        }

        // Render fullscreen quad with the shader for this light type to accumulate its light in the lighting texture
        currentShader->setShaderParams(unitRectangleGeometry_, params, ParameterArray::Empty, 0,
                                       currentShader->getSortKey(params, ParameterArray::Empty));
        drawUnitRectangle();
    }

    if (currentShader)
        currentShader->exitShader();

    // Clear the render target
    deferredLightingRenderTarget_.removeTextures();

    // Set the deferred lighting texture member so that it is accessible to shaders
    deferredLightingTexture_ = lightTexture;

    States::StateCacher::pop();

    return true;
}

bool Renderer::areShadowMapsSupported() const
{
    // Shadow maps require support for depth textures and successful setup of the shadow mapping effect

    return graphics().isPixelFormatSupported(Image::Depth, GraphicsInterface::Texture2D) && baseShadowMappingEffect_ &&
        baseShadowMappingEffect_->isActiveShaderReady();
}

const Texture* Renderer::renderDirectionalShadowMap(Scene* scene, const Light* light,
                                                    Matrix4& lightViewProjectionMatrix)
{
    // TODO: compute a smarter culling convex hull when gathering geometry for directional lights, it should be formed
    // from the camera frustum extruded along the light direction
    auto convexHull = AABB::Max.getConvexHull();

    // Query the scene for all geometry within the light's area
    auto queues = EffectQueueArray();
    auto extraShadowCasterExtents = AABB();
    scene->gatherShadowGeometry(light->getLightTransform().getDirection(), convexHull, queues,
                                &extraShadowCasterExtents);

    // Sort the gathered geometry
    auto normalGeometry = Vector<EffectQueue*>();
    auto refractiveGeometry = Vector<EffectQueue*>();
    auto reflectionPlanes = Vector<Plane>();
    sortEffectQueues(queues, Camera(), normalGeometry, refractiveGeometry, reflectionPlanes);
    if (normalGeometry.empty())
        return nullptr;

    // Get light space AABB around the shadow casting geometry
    auto lightSpaceShadowCasterAABB = AABB();
    if (extraShadowCasterExtents != AABB())
        lightSpaceShadowCasterAABB = AABB(extraShadowCasterExtents, light->getLightTransform().getInverse());
    for (auto queue : normalGeometry)
    {
        auto transform = pointer_to<const ChangeTransformRenderQueueItem>::type();

        for (auto item : queue->getItems())
        {
            if (auto transformItem = item->as<ChangeTransformRenderQueueItem>())
                transform = transformItem;
            else if (auto drawChunkItem = item->as<DrawGeometryChunkRenderQueueItem>())
            {
                auto corners = std::array<Vec3, 8>();
                drawChunkItem->getGeometryChunk().getAABB().getCorners(corners, transform->getTransform(),
                                                                       transform->getScale());

                for (const auto& corner : corners)
                    lightSpaceShadowCasterAABB.addPoint(light->getLightTransform().getInverse() * corner);
            }
        }
    }

    // Request a temporary shadow map texture
    auto shadowMap = requestTemporaryTexture(shadowMapSize_, shadowMapSize_, Image::Depth);
    if (!shadowMap)
        return nullptr;

    // Work out sizes, padding by a few depth texels to avoid texture clamping issues
    auto borderPadding = float(shadowMapSize_ + 3) / float(shadowMapSize_);
    auto size = lightSpaceShadowCasterAABB.getDimensions() * 0.5f * borderPadding;

    // Create an orthographic camera for rendering the shadow map
    auto shadowMapCamera = Camera(
        {light->getLightTransform() * lightSpaceShadowCasterAABB.getCenter(),
         light->getLightTransform().getOrientation()},
        shadowMap->getRect(), Matrix4::getOrthographicProjection({-size.x, -size.y, size.x, size.y}, -size.z, size.z),
        -size.z, size.z);

    States::StateCacher::push();
    pushCamera(shadowMapCamera);
    {
        shadowMapRenderTarget_.setDepthTexture(shadowMap);

        States::RenderTarget = shadowMapRenderTarget_;
        States::DepthTestEnabled = true;
        States::DepthWriteEnabled = true;
        States::ScissorEnabled = false;

        // Clear the shadow map texture
        graphics().clearBuffers(false, true, true);

        // Render the shadow casting geometry using the shadow mapping effect
        drawEffectQueues(normalGeometry, SkipBlendedGeometry, baseShadowMappingEffect_);

        shadowMapRenderTarget_.setDepthTexture(nullptr);
    }
    popCamera();
    States::StateCacher::pop();

    // Calculate light view projection matrix
    lightViewProjectionMatrix = shadowMapCamera.getProjectionMatrix() * shadowMapCamera.getViewMatrix();

    return shadowMap;
}

const Texture* Renderer::renderSpotShadowMap(Scene* scene, const Light* light, const Matrix4& lightViewProjectionMatrix)
{
    // Query the scene for all geometry within the light's area
    auto queues = EffectQueueArray();
    scene->gatherShadowGeometry(light->getLightTransform().getPosition(), ConvexHull(lightViewProjectionMatrix),
                                queues);

    // Sort the gathered geometry
    auto normalGeometry = Vector<EffectQueue*>();
    auto refractiveGeometry = Vector<EffectQueue*>();
    auto reflectionPlanes = Vector<Plane>();
    sortEffectQueues(queues, Camera(), normalGeometry, refractiveGeometry, reflectionPlanes);
    if (normalGeometry.empty())
        return nullptr;

    // Request a temporary shadow map texture
    auto shadowMap = requestTemporaryTexture(shadowMapSize_, shadowMapSize_, Image::Depth);
    if (!shadowMap)
        return nullptr;

    // Create a perspective camera for rendering the shadow map
    auto shadowMapCamera = Camera(light->getLightTransform(), shadowMap->getRect(), light->getProjectionMatrix(), 0.25f,
                                  light->getRadius());

    States::StateCacher::push();
    pushCamera(shadowMapCamera);
    {
        shadowMapRenderTarget_.setDepthTexture(shadowMap);

        States::RenderTarget = shadowMapRenderTarget_;
        States::Viewport = shadowMapCamera.getViewport();
        States::DepthTestEnabled = true;
        States::DepthWriteEnabled = true;
        States::ScissorEnabled = false;

        // Clear the shadow map texture
        graphics().clearBuffers(false, true, true);

        // Render the shadow casting geometry using the shadow mapping effect
        drawEffectQueues(normalGeometry, SkipBlendedGeometry, baseShadowMappingEffect_);

        shadowMapRenderTarget_.setDepthTexture(nullptr);
    }
    popCamera();
    States::StateCacher::pop();

    return shadowMap;
}

}
