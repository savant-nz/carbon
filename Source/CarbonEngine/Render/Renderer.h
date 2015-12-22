/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Math/Matrix4.h"
#include "CarbonEngine/Math/SimpleTransform.h"
#include "CarbonEngine/Platform/FrameTimers.h"
#include "CarbonEngine/Render/EffectQueueArray.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Render/RenderTarget.h"
#include "CarbonEngine/Render/Texture/TextureProperties.h"

namespace Carbon
{

/**
 * Renderer.
 */
class CARBON_API Renderer : public EventHandler, private Noncopyable
{
public:

    /**
     * This class describes a camera to the renderer and is used when queuing a scene for rendering and when rendering a scene
     * into a texture. It includes information about the camera position, orientation, projection matrix, viewport and near and
     * far plane distances.
     */
    class CARBON_API Camera
    {
    public:

        Camera() {}

        /**
         * Sets up this camera description with the given values, the view matrix is calculated from \a transform.
         */
        Camera(const SimpleTransform& transform, const Rect& viewport, const Matrix4& projectionMatrix, float nearPlaneDistance,
               float farPlaneDistance)
            : position_(transform.getPosition()),
              orientation_(transform.getOrientation().getMatrix3()),
              projectionMatrix_(projectionMatrix),
              viewport_(viewport),
              nearPlaneDistance_(nearPlaneDistance),
              farPlaneDistance_(farPlaneDistance)
        {
            updateViewMatrix();
        }

        /**
         * Returns the world space position of this camera.
         */
        const Vec3& getPosition() const { return position_; }

        /**
         * Returns the world space orientation of this camera.
         */
        const Matrix3& getOrientation() const { return orientation_; }

        /**
         * Sets the orientation for this camera.
         */
        void setOrientation(const Matrix3& matrix);

        /**
         * Returns the view matrix for this camera.
         */
        const Matrix4& getViewMatrix() const { return viewMatrix_; }

        /**
         * Returns the screen-space viewport coordinates for this camera in pixels.
         */
        const Rect& getViewport() const { return viewport_; }

        /**
         * Sets the screen-space viewport coordinates for this camera in pixels.
         */
        void setViewport(const Rect& viewport) { viewport_ = viewport; }

        /**
         * Returns the projection matrix for this camera.
         */
        const Matrix4& getProjectionMatrix() const { return projectionMatrix_; }

        /**
         * Returns the near clipping plane distance for this camera.
         */
        float getNearPlaneDistance() const { return nearPlaneDistance_; }

        /**
         * Returns the far clipping plane distance for this camera.
         */
        float getFarPlaneDistance() const { return farPlaneDistance_; }

        /**
         * Reflects this camera in the specified plane, this is used to build cameras for render reflections.
         */
        void reflectInPlane(const Plane& plane);

    private:

        Vec3 position_;
        Matrix3 orientation_;
        Matrix4 viewMatrix_;
        Matrix4 projectionMatrix_;
        Rect viewport_;
        float nearPlaneDistance_ = 0.0f;
        float farPlaneDistance_ = 0.0f;

        void updateViewMatrix();
    };

    /**
     * Pure virtual interface that defines a light that can be used by the renderer.
     */
    class CARBON_API Light
    {
    public:

        virtual ~Light() {}

        /**
         * Returns whether this is a directional light.
         */
        virtual bool isDirectionalLight() const = 0;

        /**
         * Returns whether this is a point light.
         */
        virtual bool isPointLight() const = 0;

        /**
         * Returns whether this is a spot light.
         */
        virtual bool isSpotLight() const = 0;

        /**
         * Returns the color of this light
         */
        virtual const Color& getColor() const = 0;

        /**
         * Returns the world space transform of this light.
         */
        virtual const SimpleTransform& getLightTransform() const = 0;

        /**
         * Returns the projection matrix for this light, this is only used on spotlights.
         */
        virtual const Matrix4& getProjectionMatrix() const = 0;

        /**
         * For point and spot lights, returns the radius of this light.
         */
        virtual float getRadius() const = 0;

        /**
         * Returns whether specular highlights should be computed when rendering this light source.
         */
        virtual bool isSpecularEnabled() const = 0;

        /**
         * Returns the specular intensity that will be used when rendering specular highlights for this light.
         */
        virtual float getSpecularIntensity() const = 0;

        /**
         * For spot lights, this returns the angle away from the Z axis at which cone attenuation will finish.
         */
        virtual float getMaximumConeAngle() const = 0;

        /**
         * For spot lights, this returns the angle away from the Z axis at which cone attenuation will start occurring.
         */
        virtual float getMinimumConeAngle() const = 0;

        /**
         * Returns the texture object for this light's 2D projection texture if it has one, or null if it does not.
         */
        virtual const Texture* getProjectionTexture() const = 0;

        /**
         * Returns the texture object for this light's cubemap projection texture if it has one, or null if it does not.
         */
        virtual const Texture* getProjectionCubemapTexture() const = 0;

        /**
         * Returns whether this light should cast shadows.
         */
        virtual bool isShadowingEnabled() const = 0;

        /**
         * Returns a world space AABB around this light's extents, only implemented for point and spot lights.
         */
        virtual AABB getLightAABB() const = 0;
    };

    /**
     * Pure virtual interface that defines a scene that can be rendered.
     */
    class CARBON_API Scene
    {
    public:

        virtual ~Scene() {}

        /**
         * Returns the name of this scene.
         */
        virtual const String& getName() const = 0;

        /**
         * Returns whether depth testing should be enabled when rendering this scene.
         */
        virtual bool isDepthTestEnabled() const = 0;

        /**
         * Returns whether a depth clear will be done prior to rendering this scene.
         */
        virtual bool isDepthClearEnabled() const = 0;

        /**
         * Returns whether to use deferred lighting when rendering this scene.
         */
        virtual bool isDeferredLightingEnabled() const = 0;

        /**
         * Gathers visible geometry from this scene.
         */
        virtual void gatherGeometry(const Vec3& cameraPosition, const ConvexHull& frustum, EffectQueueArray& queues) = 0;

        /**
         * Gathers shadow-casting geometry from this scene.
         */
        virtual void gatherShadowGeometry(const Vec3& cameraPosition, const ConvexHull& frustum, EffectQueueArray& queues,
                                          AABB* extraWorldSpaceShadowCasterExtents = nullptr) = 0;

        /**
         * Gathers all the lights affecting the given area in this scene, and also returns the ambient light color.
         */
        virtual void gatherLights(const ConvexHull& area, Vector<Light*>& lights, Color& ambientLightColor) = 0;

        /**
         * Returns the set of post-process effects to use when rendering this scene.
         */
        virtual const EffectQueueArray& getPostProcessEffects() const = 0;

        /**
         * Returns whether post-process pass-through is enabled for this scene, when this is enabled the render output of this
         * scene is combined with the render output of other scenes with post-processing enabled that are rendered after it,
         * i.e. they are all written to the same temporary offscreen target. Then, when a subsequent scene with active
         * post-process effects is rendered, its post-process effects will be applied to the combined output from all scene that
         * have been passed through up to that point. This system enables one set of post-process effects to be applied to the
         * combined rendering result of multiple scenes. If post-process pass-through is enabled on this scene and no
         * subsequently rendered scene has any post-process effects set on it then the output of this scene will be lost.
         * Post-process pass-through is disabled by default.
         */
        virtual bool isPostProcessPassThroughEnabled() const = 0;
    };

    /**
     * Initializes the renderer. Returns success flag.
     */
    bool setup();

    /**
     * Returns the 2D diffuse texture that is used a placeholder when other textures fail to load. This is a texture with the
     * words "Texture error" in white on an orange background and is loaded from the built-in "TextureError.png" file.
     */
    const Texture* getErrorTexture() const;

    /**
     * Returns the 2D normal map that is used when normal maps fail to load or a placeholder normal map is needed. This is a 1x1
     * normal map texture with a unit normal in Z, i.e. RGB(0.5 0.5 1.0) and is loaded from the built-in "FlatNormalMap.png"
     * file.
     */
    const Texture* getFlatNormalMap() const;

    /**
     * Returns the built-in 1x1 2D "White" texture. This is used in shaders as a fallback texture, and materials can access it
     * by specifying "White" as the texture name.
     */
    const Texture* getWhiteTexture() const;

    /**
     * Returns the built-in 1x1 2D "Black" texture. This is used in shaders as a fallback texture, and materials can access it
     * by specifying "Black" as the texture name.
     */
    const Texture* getBlackTexture() const;

    /**
     * Returns the built-in 1x1 cubemap "WhiteCube" texture. This is used in shaders as a fallback texture and materials can
     * access it by specifying "WhiteCube" as the texture name.
     */
    const Texture* getWhiteCubemapTexture() const;

    /**
     * Returns the built-in 1x1 cubemap "BlackCube" texture. This is used in shaders as a fallback texture and materials can
     * access it by specifying "BlackCube" as the texture name.
     */
    const Texture* getBlackCubemapTexture() const;

    /**
     * Returns the current camera being used.
     */
    const Camera& getCamera() const { return *cameras_.back(); }

    /**
     * Returns the world transform of the geometry chunk currently being drawn. For use in shaders.
     */
    const SimpleTransform& getCurrentTransform() const { return currentTransform_; }

    /**
     * Returns the scale factors for the geometry chunk currently being drawn. For use in shaders.
     */
    const Vec3& getCurrentScale() const { return currentScale_; }

    /**
     * Returns the 3x3 matrix that is the inverse of the current orientation returned in Renderer::getCurrentTransform(). For
     * use in shaders.
     */
    const Matrix3& getCurrentOrientationInverseMatrix() const;

    /**
     * Returns the 4x4 matrix that is the inverse of the current transform returned by Renderer::getCurrentTransform() and the
     * scale returned by Renderer::getCurrentScale(). For use in shaders.
     */
    const Matrix4& getCurrentTransformInverseMatrix() const;

    /**
     * Returns the position of the camera in the local space of the geometry chunk currently being drawn. For use in shaders.
     */
    const Vec3& getLocalSpaceCameraPosition() const;

    /**
     * Returns the current model-view matrix. For use in shaders.
     */
    const Matrix4& getModelViewMatrix() const { return modelViewMatrix_; }

    /**
     * Returns the final model-view-projection matrix that should be used by shaders to transform vertices.
     */
    const Matrix4& getModelViewProjectionMatrix() const;

    /**
     * Returns a that will project a 2D texture out over the whole viewport.
     */
    const Matrix4& getScreenProjectionMatrix() const;

    /**
     * The current ambient light color. For use in shaders.
     */
    const Color& getAmbientLightColor() const { return ambientLightColor_; }

    /**
     * The current primary directional light direction. For use in shaders.
     */
    const Vec3& getDirectionalLightDirection() const { return directionalLightDirection_; }

    /**
     * The current primary directional light color. For use in shaders.
     */
    const Color& getDirectionalLightColor() const { return directionalLightColor_; }

    /**
     * For use by shaders, returns a vector of all the lights that intersect with the passed AABB in local model space. The
     * return value indicates whether lighting is enabled for the scene currently being rendered.
     */
    bool gatherLights(const AABB& localAABB, Vector<const Light*>& lights);

    /**
     * Queues a scene for rendering. Normally client applications will use Scene::queueForRendering() rather than calling this
     * method directly.
     */
    void queueForRendering(Scene* scene, const Camera& camera, int priority,
                           GraphicsInterface::OutputDestination outputDestination);

    /**
     * This is the main renderer entry point, it renders all the scenes that have been queued with
     * Renderer::queueForRendering().
     */
    void render();

    /**
     * Renders the given scene into the passed 2D texture. Returns success flag.
     */
    bool renderIntoTexture(Scene* scene, const Vector<Camera>& cameras, Texture2D* texture);

    /**
     * Renders the given scene into the passed cubemap texture. Returns success flag.
     */
    bool renderIntoTexture(Scene* scene, const Vector<Camera>& cameras, TextureCubemap* texture);

    /**
     * Returns the 2D refraction texture for use by shaders. This will return a correct texture when called from inside a shader
     * that has specified itself as needing a refraction texture as an input, otherwise the content of this texture is
     * undefined.
     */
    const Texture* getRefractionTexture() const { return refractionTexture_; }

    /**
     * Returns the 2D reflection texture for use by shaders. This will return a correct texture when called from inside a shader
     * that has specified itself as needing a reflection texture as an input, otherwise the content of this texture is
     * undefined.
     */
    const Texture* getReflectionTexture() const { return reflectionTexture_; }

    /**
     * Returns the 2D texture that contains deferred lighting information. This is only for use in shaders that are doing
     * deferred lighting.
     */
    const Texture* getDeferredLightingTexture() const { return deferredLightingTexture_; }

    /**
     * This method is for use by multipass post-process shaders only and it sets the texture to render the output of the current
     * shader pass to. This allows the use of arbitrary intermediate textures by post processing shaders to run algorithms such
     * as separable blurs. This method also sets the States::Viewport state to the rectangle of the passed texture.
     */
    bool setPostProcessIntermediateTargetTexture(const Texture* texture);

    /**
     * Requests a temporary texture with the given dimensions and pixel format, this should only be used for temporary textures
     * that are needed as part of a single frame, they should not generally be held across multiple frames, and should always be
     * released as soon as possible to maximize texture reuse. This method will return null if the requested temporary texture
     * can't be supplied.
     */
    const Texture* requestTemporaryTexture(unsigned int width, unsigned int height, Image::PixelFormat pixelFormat,
                                           TextureProperties::TextureFilter filter = TextureProperties::NearestFilter);

    /**
     * This method is a convenience overload for Renderer::requestTemporaryTexture() that uses the width and height of the given
     * Rect. See Renderer::requestTemporaryTexture() for more details.
     */
    const Texture* requestTemporaryTexture(const Rect& rect, Image::PixelFormat pixelFormat,
                                           TextureProperties::TextureFilter filter = TextureProperties::NearestFilter)
    {
        return requestTemporaryTexture(uint(rect.getWidth()), uint(rect.getHeight()), pixelFormat, filter);
    }

    /**
     * Requests a temporary texture with the given dimensions, this should only be used for temporary textures that are needed
     * as part of a single frame, they should not generally be held across multiple frames, and should always be released as
     * soon as possible to maximize texture reuse. This method will return either an RGB or RGBA temporary texture depending on
     * whether \a includeAlpha is true. If \a forceHDR is true then a floating point format will be returned even if HDR
     * rendering isn't enabled on th renderer, otherwise if \a forceHDR is false the returned texture will only be floating
     * point if HDR is enabled on the renderer. This method will return null if the requested temporary texture can't be
     * supplied.
     */
    const Texture* requestTemporaryTexture(unsigned int width, unsigned int height, bool includeAlpha, bool forceHDR,
                                           TextureProperties::TextureFilter filter = TextureProperties::NearestFilter);

    /**
     * This method is a convenience overload for Renderer::requestTemporaryTexture() that uses the width and height of the given
     * Rect. See Renderer::requestTemporaryTexture() for more details.
     */
    const Texture* requestTemporaryTexture(const Rect& rect, bool includeAlpha, bool forceHDR,
                                           TextureProperties::TextureFilter filter = TextureProperties::NearestFilter)
    {
        return requestTemporaryTexture(uint(rect.getWidth()), uint(rect.getHeight()), includeAlpha, forceHDR, filter);
    }

    /**
     * Releases a temporary texture that was allocated by a call to Renderer::requestTemporaryTexture(). Always release
     * temporary textures as soon as they are no longer being used.
     */
    void releaseTemporaryTexture(const Texture* texture);

    /**
     * This is called during rendering when an offscreen render target is required, such as when rendering a reflection. If such
     * a render target is available then it will be returned for use, otherwise null will be returned. Render targets allocated
     * by this method should be released as soon as they are no longer needed so that they can be reused.
     */
    const RenderTarget* requestTemporaryRenderTarget(unsigned int width, unsigned int height);

    /**
     * This is a convenience overload for Renderer::requestTemporaryRenderTarget() that gets the width and height from the
     * passed \a rect parameter.
     */
    const RenderTarget* requestTemporaryRenderTarget(const Rect& rect)
    {
        return requestTemporaryRenderTarget(uint(rect.getWidth()), uint(rect.getHeight()));
    }

    /**
     * Releases a temporary render target that was allocated by Renderer::requestTemporaryRenderTarget(). Always release
     * temporary render targets as soon as they are no longer being used.
     */
    void releaseTemporaryRenderTarget(const RenderTarget* target);

    /**
     * Returns the scale factor to use on the dimensions of the off-screen render targets used when doing fullscreen effects
     * such as post-processing and deferred lighting, this can be used to cause these effects to be done with textures that are
     * larger than the main framebuffer. The higher resolution offscreen buffers will then be downsampled when rendering the
     * final framebuffer output. This is useful when using post-processing effects that distort the final image in such a way
     * that subsampling would become apparent unless higher resolution initial renders are used. Defaults to 1.0.
     */
    float getFullscreenRenderTargetDimensionsScaleFactor() const { return fullscreenRenderTargetDimensionsScaleFactor_; }

    /**
     * Sets the scale factor to use on the dimensions of the off-screen render targets used when doing fullscreen effects such
     * as post-processing and deferred lighting. The passed value is clamped in the range 0.1 - 10.0. Defaults to 1.0. See
     * Renderer::getFullscreenRenderTargetDimensionsScaleFactor() for more details.
     */
    void setFullscreenRenderTargetDimensionsScaleFactor(float scale)
    {
        fullscreenRenderTargetDimensionsScaleFactor_ = Math::clamp(scale, 0.1f, 10.0f);
    }

    /**
     * Returns the post-process effects to apply globally to the combined output of all rendered scenes.
     */
    EffectQueueArray& getGlobalPostProcessEffects() { return globalPostProcessEffects_; }

    /**
     * Returns whether the FPS is being displayed in the bottom left hand corner of the window.
     */
    bool getShowFPS() const { return showFPS_; }

    /**
     * Sets whether to display the FPS in the bottom left hand corner of the window.
     */
    void setShowFPS(bool showFPS);

    /**
     * Returns whether debug info is being displayed in the bottom left hand corner of the window.
     */
    bool getShowDebugInfo() const { return showDebugInfo_; }

    /**
     * Sets whether to display debug info in the bottom left hand corner of the window.
     */
    void setShowDebugInfo(bool showDebugInfo);

    /**
     * Returns whether frame timer rendering is currently enabled.
     */
    bool isFrameTimerRenderingEnabled() const { return frameTimerRenderingEnabled_; }

    /**
     * Sets whether frame timer rendering is currently enabled.
     */
    void setFrameTimerRenderingEnabled(bool enabled);

    /**
     * Returns whether HDR rendering is supported on the current hardware.
     */
    bool isHDRSupported() const;

    /**
     * Returns whether HDR is currently enabled.
     */
    bool isHDREnabled() const { return isHDREnabled_; }

    /**
     * Sets whether HDR is enabled. HDR can only be enabled when it is supported in hardware.
     */
    bool setHDREnabled(bool enabled);

    /**
     * Returns whether this hardware supports doing deferred lighting on scenes.
     */
    bool isDeferredLightingSupported() const;

    /**
     * Returns whether this hardware supports shadow map rendering.
     */
    bool areShadowMapsSupported() const;

    /**
     * Returns the size of the shadow map that will be used when doing directional light shadow maps.
     */
    unsigned int getShadowMapSize() const { return shadowMapSize_; }

    /**
     * Sets the size of the shadow map that will be used when doing directional light shadow maps, must be a power of two.
     */
    bool setShadowMapSize(unsigned int size);

    /**
     * Adds a string to be rendered in the debugging info. Typically the easiest way to add a renderer debug string is with the
     * CARBON_RENDER_INFO() and CARBON_RENDER_VALUE() macros.
     */
    void addDebugString(const UnicodeString& s);

    /**
     * Causes the render queues gathered in the next frame to be printed out using LOG_DEBUG(). Useful when debugging.
     */
    void printRenderQueues() { printRenderQueues_ = true; }

    /**
     * Sets the texture to render as a debug overlay, this is useful for inspecting the contents of textures at runtime during
     * development and debugging. To disable texture debugging pass an empty string for \a name.
     */
    void setDebugTexture(const String& name, unsigned int frame = 0, unsigned int mipmap = 0, bool renderAlpha = false,
                         float scale = 1.0f);

    /**
     * Internal helper class used by the \ref CARBON_RENDER_INFO macro, it houses a temporary UnicodeString instance that is
     * added as a renderer debug string on destruction.
     */
    class DebugStringWriter : private Noncopyable
    {
    public:

#ifdef CARBON_INCLUDE_LOGGING
        ~DebugStringWriter() { renderer().addDebugString(string_); }
#endif

#ifdef CARBON_INCLUDE_LOGGING
        /**
         * Concatenates the passed argument onto the internal string.
         */
        template <typename T> UnicodeString& operator<<(T&& argument) { return string_ << std::forward<T>(argument); }
#else
        template <typename T> LogfileWriter& operator<<(T&&) { return *this; }
#endif

    private:

#ifdef CARBON_INCLUDE_LOGGING
        UnicodeString string_;
#endif
    };

private:

    Renderer() {}
    ~Renderer() override;
    friend class Globals;

    // Effects used directly by the renderer
    Effect* baseColoredEffect_ = nullptr;
    Effect* baseSurfaceEffect_ = nullptr;
    Effect* fontEffect_ = nullptr;
    Effect* baseShadowMappingEffect_ = nullptr;
    Effect* deferredLightingSetupEffect_ = nullptr;
    Effect* deferredLightingDirectionalLightEffect_ = nullptr;
    Effect* deferredLightingPointLightEffect_ = nullptr;
    Effect* deferredLightingSurfaceEffect_ = nullptr;

    // Built-in textures
    mutable const Texture* errorTexture_ = nullptr;
    mutable const Texture* errorNormalMap_ = nullptr;
    mutable const Texture* whiteTexture_ = nullptr;
    mutable const Texture* blackTexture_ = nullptr;
    mutable const Texture* whiteCubemapTexture_ = nullptr;
    mutable const Texture* blackCubemapTexture_ = nullptr;

    // Rectangle geometry chunk with unit texture coordinates, this is used when rendering rectangle render queue items and
    // whenever a simple textured quad is needed
    GeometryChunk unitRectangleGeometry_;
    void createUnitRectangleGeometry();
    void drawUnitRectangle();

    // Queued scenes waiting to be rendered
    struct QueuedScene
    {
        Scene* scene = nullptr;
        Camera camera;
        int priority = 0;
        GraphicsInterface::OutputDestination outputDestination = GraphicsInterface::OutputDefault;

        QueuedScene() {}

        QueuedScene(Scene* scene_, const Camera& camera_, int priority_,
                    GraphicsInterface::OutputDestination outputDestination_)
            : scene(scene_), camera(camera_), priority(priority_), outputDestination(outputDestination_)
        {
        }
    };
    Vector<QueuedScene> queuedScenes_;

    // The camera stack, the current camera is the last one in the vector and one will always be available when rendering
    Vector<const Camera*> cameras_;
    void pushCamera(const Camera& camera);
    void popCamera() { cameras_.popBack(); }

    // Lighting details for the scene currently being rendered
    Vector<Light*> currentLights_;
    Color ambientLightColor_;

    // The current ambient light color and direction/color of the main directional light source in the scene
    Vec3 directionalLightDirection_;
    Color directionalLightColor_;

    // The current transform, scale and model-view matrices are stored here during rendering
    SimpleTransform currentTransform_;
    Vec3 currentScale_;
    Matrix4 modelViewMatrix_;

    // Various transforms involving the above transforms and the projection matrix are used by shaders, these are calculated JIT
    // and are cached here
    mutable Matrix3 currentOrientationInverseMatrix_;
    mutable bool isCurrentOrientationInverseMatrixCached_ = false;
    mutable Matrix4 currentTransformInverseMatrix_;
    mutable bool isCurrentTransformInverseMatrixCached_ = false;
    mutable Matrix4 modelViewProjectionMatrix_;
    mutable bool isModelViewProjectionMatrixCached_ = false;
    mutable Matrix4 screenProjectionMatrix_;
    mutable bool isScreenProjectionMatrixCached_ = false;
    mutable Vec3 localSpaceCameraPosition_;
    mutable bool isLocalSpaceCameraPositionCached_ = false;
    void clearCachedTransforms();

    bool isHDREnabled_ = false;
    float fullscreenRenderTargetDimensionsScaleFactor_ = 1.0f;

    bool printRenderQueues_ = false;

    // FPS calculation
    bool showFPS_ = false;
    unsigned int lastFPS_ = 0;
    unsigned int frameCount_ = 0;
    TimeValue lastFPSTime_;

    // Debug info
    bool showDebugInfo_ = false;
    Vector<UnicodeString> debugStrings_;

    // Frame statistics
    uint64_t frameDrawCallCount_ = 0;
    uint64_t lastDrawCallCount_ = 0;
    uint64_t frameTriangleCount_ = 0;
    uint64_t lastTriangleCount_ = 0;
    uint64_t frameAPICallCount_ = 0;
    uint64_t lastAPICallCount_ = 0;

    bool processEvent(const Event& e) override;

    // Temporary textures are any textures that are needed by the renderer during the course of processing a frame, usually for
    // doing things such as off-screen rendering, refractions, reflections, post processing, and so on. These textures are
    // cached by the renderer and handed out on as-needed basis. Once the texture is finished being used then it is released
    // back so that it can be given out in a subsequent request. The same texture can potentially be re-used many times
    // throughout the life of a single frame. Temporary textures are used both internally by the renderer as well as by shaders
    // that need intermediate textures.

    struct TemporaryTexture
    {
        Texture* texture = nullptr;
        bool inUse = false;

        TemporaryTexture() {}
        TemporaryTexture(Texture* texture_, bool inUse_) : texture(texture_), inUse(inUse_) {}
    };
    Vector<TemporaryTexture> temporaryTextures_;

    void clearTemporaryTextures(bool depthTexturesOnly = false);

    // Temporary render targets
    Vector<RenderTarget*> temporaryRenderTargets_;
    void clearTemporaryRenderTargets();

    RenderTarget renderToTextureRenderTarget_;

    const Texture* reflectionTexture_ = nullptr;
    const Texture* refractionTexture_ = nullptr;

    // The primary entry point for drawing is the draw() method, this method renders the passed scene using the passed camera
    // into the currently active render target. Internally it may recurse in order to render reflections or do various offscreen
    // renders as part of other effects. It relies on a number of additional methods to sort lists of geometry, for deferred
    // lighting, to do the actual rendering, and for post-processing.

    void draw(Scene* scene, const Camera& camera, bool clearColorBuffer, bool clearDepthStencilBuffer,
              bool allowPostProcessPassThrough = false, unsigned int recursionDepth = 0);

    void sortEffectQueues(EffectQueueArray& queues, const Camera& camera, Vector<EffectQueue*>& normalGeometry,
                          Vector<EffectQueue*>& refractiveGeometry, Vector<Plane>& reflectionPlanes);

    int getReflectionPlaneIndex(const RenderQueueItemArray& items, const Camera& camera, Vector<Plane>& reflectionPlanes) const;

    // These methods are where a set of render queue items actually gets drawn, implemented in RendererDraw.cpp
    enum BlendedGeometrySetting
    {
        DrawBlendedGeometry,
        SkipBlendedGeometry,
        OnlyDrawBlendedGeometry
    };
    void drawEffectQueues(const Vector<EffectQueue*>& queues,
                          BlendedGeometrySetting blendedGeometrySetting = DrawBlendedGeometry,
                          Effect* overrideEffect = nullptr);
    void executeRenderQueueItem(const ChangeTransformRenderQueueItem& item);
    void executeRenderQueueItem(const DrawTextRenderQueueItem& item, Shader* shader);
    void executeRenderQueueItem(const DrawRectangleRenderQueueItem& item, Shader* shader, const ParameterArray& params,
                                const ParameterArray& internalParams, unsigned int sortKey);
    void executeRenderQueueItem(const DrawGeometryChunkRenderQueueItem& item, Effect* effect, Shader* shader,
                                const ParameterArray& params, const ParameterArray& internalParams, unsigned int sortKey);

    // Post processing
    RenderTarget scenePostProcessRenderTarget_;
    RenderTarget globalPostProcessRenderTarget_;
    EffectQueueArray globalPostProcessEffects_;
    bool setupForPostProcessing(RenderTarget& renderTarget, const EffectQueueArray& postProcessEffects, const Rect& viewport,
                                bool* clearColorBuffer = nullptr);
    void drawPostProcess(RenderTarget& renderTarget, const EffectQueueArray& postProcessEffects,
                         GraphicsInterface::RenderTargetObject finalRenderTargetObject, const Rect& finalViewport,
                         bool isPostProcessPassThroughActive = false);
    void checkPostProcessPassThroughsCompleted(RenderTarget& renderTarget);
    RenderTarget* activePostProcessRenderTarget_ = nullptr;

    // Deferred lighting is done as an initial series of steps prior to rendering the actual scene, implemented in
    // RendererDeferredLighting.cpp
    RenderTarget deferredLightingRenderTarget_;
    const Texture* deferredLightingTexture_ = nullptr;
    bool renderDeferredLightingTexture(Scene* scene, const ConvexHull& frustum, const Vector<EffectQueue*>& normalGeometry,
                                       Vector<const Texture*>& allocatedTemporaryTextures);

    // Shadows
    RenderTarget shadowMapRenderTarget_;
    unsigned int shadowMapSize_ = 0;
    const Texture* renderDirectionalShadowMap(Scene* scene, const Light* light, Matrix4& lightViewProjectionMatrix);
    const Texture* renderSpotShadowMap(Scene* scene, const Light* light, const Matrix4& lightViewProjectionMatrix);

    // Debug overlays are the console, debug info, debug texture output and the frame timers graph. Drawing of these is
    // implemented in RendererDebugOverlays.cpp
    void drawDebugOverlays();
    void drawDebugInfo(const Font* font, float fontSize, float padding, EffectQueueArray& queues);
    void drawConsole(const Font* font, float fontSize, float padding, EffectQueueArray& queues);

    // Debug texture overlay
    struct
    {
        String name;
        unsigned int frame = 0;
        unsigned int mipmap = 0;
        float scale = 1.0f;
        bool renderAlpha = false;
    } debugTexture_;
    void drawDebugTexture(EffectQueueArray& queues);
    void drawDebugTextureSurface(const Texture* texture, float scale);
    void drawDebugTextureSurfaceGeometry(const Texture* texture, float scale, Shader* shader, const ParameterArray& params);

    // The frame timers graph uses two geometry chunks
    bool frameTimerRenderingEnabled_ = false;
    GeometryChunk timerResultsGeometryChunk_;
    GeometryChunk timerGraphAxesGeometryChunk_;
    bool updateFrameTimersGraph_ = false;
    TimeValue lastFrameTimersGraphUpdateTime_;
    void onFrameTimersSamplingDataReady(FrameTimers& sender, TimeValue time);
    void setupFrameTimersGraphGeometryChunks(unsigned int timerCount);
    void drawFrameTimersGraph(const Font* font, float fontSize);
};

/**
 * \file
 */

/**
 * Macro that adds a renderer debug string in the same fashion as writing to the logfile. This is more convenient than calling
 * Renderer::addDebugString() directly.
 */
#define CARBON_RENDER_INFO Renderer::DebugStringWriter()

/**
 * Macro that adds a renderer debug string based on the evalaution of the specified \a Data. The format used to display is
 * `<name>: <value>`, so for example `CARBON_RENDER_VALUE(myInteger);` would show a debug string of `myInteger: 42`.
 */
#define CARBON_RENDER_VALUE(Data) CARBON_RENDER_INFO << #Data ": " << Data

}
