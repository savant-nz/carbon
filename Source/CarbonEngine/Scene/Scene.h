/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Core/FileSystem/SimpleFileSystemVolume.h"
#include "CarbonEngine/Core/Runnable.h"
#include "CarbonEngine/Core/UnorderedPointerSet.h"
#include "CarbonEngine/Exporters/ExportInfo.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Scene/Entity.h"
#include "CarbonEngine/Scene/IntersectionResult.h"

namespace Carbon
{

/**
 * A scene handles a set of interacting entities and can be either 2D or 3D.
 */
class CARBON_API Scene : public EventHandler, public Renderer::Scene, private Noncopyable
{
public:

    /**
     * The scene directory, currently "Scenes/".
     */
    static const UnicodeString SceneDirectory;

    /**
     * The scene file extension, currently ".scene".
     */
    static const UnicodeString SceneExtension;

    /**
     * When a scene is loaded any entities that have a name starting with the static mesh conversion prefix are turned into
     * static meshes and placed into the scene. The prefix is stripped off to get the name of the mesh to embed. Currently
     * "mesh_".
     */
    static const String StaticMeshConversionPrefix;

    ~Scene() override;

    /**
     * Copy constructor (not implemented).
     */
    Scene(const Scene& other);

    /**
     * Constructs this scene with the given name and whether or not it is 2D.
     */
    Scene(const String& name = String::Empty, bool is2D = false);

    /**
     * Clears the entire contents of this scene and reverts to default state, this internally calls Scene::removeAllEntities()
     * as part of its processing.
     */
    void clear();

    /**
     * Removes all entities from this scene, this also clears static meshes and collision/physics data that may be present on
     * this scene, but other settings are left intact.
     */
    void removeAllEntities();

    const String& getName() const override { return name_; }

    /**
     * Sets the name of this scene.
     */
    void setName(const String& name) { name_ = name; }

    /**
     * Returns whether this scene is 2D, 2D scenes can manage input handling and do not require a camera in them in order to
     * render, though one can be used. Scenes are not 2D by default.
     */
    bool is2D() const { return is2D_; }

    /**
     * Returns whether this scene is 3D, this simply returns the opposite of Scene::is2D().
     */
    bool is3D() const { return !is2D(); }

    /**
     * Sets the scene type of this scene, see Scene::is2D() for details.
     */
    void setIs2D(bool is2D);

    /**
     * Returns the current background material for this scene.
     */
    const String& getBackgroundMaterial() const { return backgroundMaterial_; }

    /**
     * Sets the background material for this scene, the background will be drawn over the whole window if this is a 2D scene. An
     * empty string means no background material will be drawn.
     */
    void setBackgroundMaterial(const String& material) { backgroundMaterial_ = material; }

    /**
     * Returns whether this scene is enabled.
     */
    bool isEnabled() const { return isEnabled_; }

    /**
     * Sets whether this scene is enabled. Disabled scenes will not process input. For 2D scenes this means they will not
     * respond to user input or fire GUI events.
     */
    void setEnabled(bool enabled);

    /**
     * Returns whether this scene is visible. Defaults to true. A scene can only be drawn if it is visible, so if an invisible
     * scene is queued for rendering it will not be drawn. For a scene to be drawn it must be both visible and queued for
     * rendering using Scene::queueForRendering().
     */
    bool isVisible() const { return isVisible_; }

    /**
     * Sets whether this scene is visible.
     */
    void setVisible(bool visible) { isVisible_ = visible; }

    /**
     * Returns whether a depth clear will be done prior to rendering this scene. This is enabled by default, however
     * applications that render multiple separate scenes to build up a final rendered image may want to share a depth buffer
     * between them and can do so by disabling depth clearing.
     */
    bool isDepthClearEnabled() const override { return isDepthClearEnabled_; }

    /**
     * Sets whether a depth clear will be done prior to rendering this scene, for more details see Scene::isDepthClearEnabled().
     */
    void setDepthClearEnabled(bool enabled) { isDepthClearEnabled_ = enabled; }

    /**
     * Returns whether depth testing will be enabled when rendering this scene, currently depth testing is enabled for 3D scenes
     * and disabled for 2D scenes.
     */
    bool isDepthTestEnabled() const override { return is3D(); }

    /**
     * Adds an existing entity as a child of the root entity of this scene. Returns success flag. To add an entity as a child of
     * an existing entity use ComplexEntity::addChild().
     */
    bool addEntity(Entity* entity);

    /**
     * Creates a new entity of the specified type and adds it to this scene, returning the new entity instance. The name of the
     * new entity can be specified, and the new entity's `initialize()` method will be called with any additional arguments that
     * are passed.
     */
    template <typename EntityType, typename... ArgTypes>
    EntityType* addEntity(const String& name = String::Empty,
                          ArgTypes&&... args CARBON_CLANG_PRE_3_7_PARAMETER_PACK_BUG_WORKAROUND)
    {
        auto entity = SubclassRegistry<Entity>::create<EntityType>();
        if (!entity || !addEntity(entity))
        {
            SubclassRegistry<Entity>::destroy(entity);
            return nullptr;
        }

        entity->setName(name);
        initializeIfArgsPassed(entity, std::forward<ArgTypes>(args)...);

        return entity;
    }

    /**
     * Returns the entity in this scene with the given name and type, or null if one is not found. If \a name is an empty string
     * then name checking is not performed and only the type requirement will be enforced.
     */
    template <typename EntityType = Entity> EntityType* getEntity(const String& name = String::Empty)
    {
        for (auto entity : entities_)
        {
            auto e = dynamic_cast<EntityType*>(entity);

            if (e && (!name.length() || e->getName() == name))
                return e;
        }

        return nullptr;
    }

    /**
     * \copydoc getEntity(const String &)
     */
    template <typename EntityType = Entity> const EntityType* getEntity(const String& name = String::Empty) const
    {
        for (auto entity : entities_)
        {
            auto e = dynamic_cast<const EntityType*>(entity);

            if (e && (!name.length() || e->getName() == name))
                return e;
        }

        return nullptr;
    }

    /**
     * Returns the first Camera entity found in this scene, or null if there are no cameras currently in this scene.
     */
    Camera* getDefaultCamera();

    /**
     * Returns the first Camera entity found in this scene, or null if there are no cameras currently in this scene.
     */
    const Camera* getDefaultCamera() const;

    /**
     * For use in 2D scenes, this returns the orthographic viewing rectangle of the default camera, based on the camera's
     * position and the return value from Camera::getOrthographicSize(). If there is no camera in the 2D scene then the viewport
     * rect is returned, which is equivalent to a camera on the origin with an orthographic size of the main window's width and
     * height.
     */
    Rect getDefaultCameraOrthographicRect() const;

    /**
     * This is a helper method for setting up a 2D orthographic camera in this scene with the specified orthographic width and
     * height, see Camera::setOrthographicSize() for details on how these two values are interpreted. This method automatically
     * calls Scene::setIs2D(true). The created Camera instance is returned for use by the application and will be automatically
     * cleaned up along with the scene.
     */
    Camera* create2DCamera(float orthographicWidth, float orthographicHeight);

    /**
     * Returns the number of lights in this scene.
     */
    unsigned int getLightCount() const { return lights_.size(); }

    /**
     * Returns the root entity of this scene.
     */
    ComplexEntity* getRootEntity();

    /**
     * \copydoc Scene::getRootEntity()
     */
    const ComplexEntity* getRootEntity() const;

    /**
     * Returns the number of entities in this scene.
     */
    unsigned int getEntityCount() const { return entities_.size(); }

    /**
     * If this is a 2D scene then one of the GUIWindow entities in the scene can potentially have focus. This returns the window
     * with focus or null if no window currently has focus.
     */
    GUIWindow* getFocusWindow() { return focusWindow_; }

    /**
     * Sets the GUIWindow entity that currently has focus. Calling this method with an argument of null causes no window to have
     * focus. Appropriate GUIWindowLoseFocus and GUIWindowGainFocus events will be sent. Returns success flag.
     */
    bool setFocusWindow(GUIWindow* window);

    /**
     * Handles the translation of keyboard and mouse events to GUI events in 2D scenes.
     */
    bool processEvent(const Event& e) override;

    void gatherGeometry(const Vec3& cameraPosition, const ConvexHull& frustum, EffectQueueArray& queues) override;
    void gatherShadowGeometry(const Vec3& cameraPosition, const ConvexHull& frustum, EffectQueueArray& queues,
                              AABB* extraWorldSpaceShadowCasterExtents) override;
    void gatherLights(const ConvexHull& area, Vector<Renderer::Light*>& lights, Color& ambientLightColor) override;

    /**
     * Saves this scene to a file stream.
     */
    void save(FileWriter& file) const;

    /**
     * Saves this scene to a file. Returns success flag.
     */
    bool save(const String& name) const;

    /**
     * Loads a scene from a file stream.
     */
    void load(FileReader& file, ExportInfo& exportInfo = ExportInfo::TempExportInfo);

    /**
     * Loads a scene from a file.
     */
    bool load(const String& filename);

    /**
     * Rewrites all .mesh files that are stored as embedded resources in this scene, this just involves loading and then saving
     * them back to the same resource which can be useful when upgrading to newer file formats. Embedded meshes are how scenes
     * store their geometry. Returns success flag.
     */
    bool rewriteEmbeddedMeshes();

    /**
     * Returns whether or not geometry with the specified material can be considered to be part of the world geometry. By
     * default all materials will return true from this method, however if Scene::addWorldGeometryMaterial() is used then only
     * those materials added using that method will return true from this method. Note that world geometry must first be flagged
     * as such by using Entity::setIsWorldGeometry().
     */
    bool isWorldGeometryMaterial(const String& materialName) const;

    /**
     * Adds the specified material to the list of allowed world geometry materials. By default this list is empty, which means
     * all geometry can be considered to be world geometry regardless of its material. Once this list has been started, only
     * geometry that uses one of the materials added to it can be considered part of the world geometry for this scene. this can
     * be used to customize what is and what isn't treated as world geometry when doing things such as intersection testing via
     * Scene::intersect(). Note that world geometry must first be flagged as such by using Entity::setWorldGeometry().
     */
    void addWorldGeometryMaterial(const String& materialName);

    /**
     * Clears the whitelist of world geometry materials, see Scene::addWorldGeometryMaterial() for details.
     */
    void clearWorldGeometryMaterials();

    /**
     * Intersects a ray with this scene. If there was an intersection then the details of the closest intersection are returned.
     * If no intersection occurred then the return value is an empty IntersectionResult that will evaluate to false. If \a
     * onlyWorldGeometry is true then only entities flagged as world geometry will be tested for intersection.
     */
    IntersectionResult intersect(const Ray& ray, bool onlyWorldGeometry = false);

    /**
     * Intersects a ray with this scene, where the ray is cast through the given screen pixel of the given camera. If no camera
     * is given then the first camera in the scene is used. If there was an intersection then the details of the closest
     * intersection are returned. If no intersection occurred then the return value is an empty IntersectionResult that will
     * evaluate to false. If \a onlyWorldGeometry is true then only entities flagged as world geometry will be tested for
     * intersection.
     */
    IntersectionResult intersect(const Vec2& pixel, Camera* camera = nullptr, bool onlyWorldGeometry = false);

    /**
     * Intersects a ray with this scene. All the intersection results are returned in the \a results vector sorted from nearest
     * to farthest. The return value indicates whether any intersections were detected. If \a onlyWorldGeometry is true then
     * only entities flagged as world geometry will be tested for intersection.
     */
    bool intersect(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry = false);

    /**
     * Intersects a ray with this scene, where the ray is cast through the given screen pixel of the given camera. If no camera
     * is given then the first camera in the scene is used. All the intersection results are returned in the \a results vector
     * sorted from nearest to farthest. The return value indicates whether any intersections were detected. If \a
     * onlyWorldGeometry is true then only entities flagged as world geometry will be tested for intersection.
     */
    bool intersect(const Vec2& pixel, Vector<IntersectionResult>& intersections, Camera* camera = nullptr,
                   bool onlyWorldGeometry = false);

    /**
     * Checks line of sight between two points in this scene.
     */
    bool checkLineOfSight(const Vec3& p0, const Vec3& p1);

    /**
     * Ensures all data the scene needs to render is ready to go so there will be no stuttering as new parts of the scene come
     * into view.
     */
    void precache();

    /**
     * Returns whether the contents of the scene is internally consistent. This does a number of sanity and correctness checks
     * on the scene graph and reports any problems it finds as warnings. The return value indicates whether any problems were
     * found.
     */
    bool verifyIntegrity() const;

    /**
     * Returns the set of post-process materials to use when rendering this scene.
     */
    const Vector<String>& getPostProcessMaterials() const { return postProcessMaterials_; }

    /**
     * Sets the current post-process materials being used when rendering this scene. If multiple post-process materials are
     * specified then they are applied consecutively.
     */
    void setPostProcessMaterials(const Vector<String>& materials)
    {
        postProcessMaterials_ = materials;
        isPostProcessEffectsDirty_ = true;
    }

    /**
     * Adds a material to use for post-processing on this scene, the new material is appended to the existing list of
     * post-process materials for this scene.
     */
    void addPostProcessMaterial(const String& material)
    {
        postProcessMaterials_.append(material);
        isPostProcessEffectsDirty_ = true;
    }

    /**
     * Removes the specified post-process material from this scene if it is currently present. Returns success flag.
     */
    bool removePostProcessMaterial(const String& material)
    {
        isPostProcessEffectsDirty_ = true;
        return postProcessMaterials_.eraseValue(material);
    }

    /**
     * Removes all post-process materials from this scene.
     */
    void clearPostProcessMaterials()
    {
        postProcessMaterials_.clear();
        isPostProcessEffectsDirty_ = true;
    }

    const EffectQueueArray& getPostProcessEffects() const override;
    bool isPostProcessPassThroughEnabled() const override { return isPostProcessPassThroughEnabled_; }

    /**
     * Sets whether post-process pass-through is enabled for this scene, see Scene::isPostProcessPassThroughEnabled() for
     * details.
     */
    void setPostProcessPassThroughEnabled(bool enabled) { isPostProcessPassThroughEnabled_ = enabled; }

    /**
     * For 2D scenes, this adds an offset to the root entity so that all the GUI elements in the scene are centered in the
     * window.
     */
    void center();

    /**
     * Clears all the resources embedded in this scene.
     */
    void clearEmbeddedResources() { embeddedResources_.clear(); }

    /**
     * Returns whether or not this scene has an embedded resource with the given name.
     */
    bool hasEmbeddedResource(const UnicodeString& name) const;

    /**
     * Returns a list of the names of all the resources embedded in this scene.
     */
    Vector<UnicodeString> getEmbeddedResources() const;

    /**
     * Sets up the passed FileReader instance to read from the embedded resource with the given name. Returns success flag.
     */
    bool getEmbeddedResource(const UnicodeString& name, FileReader& file) const;

    /**
     * Adds an embedded resource to this scene. If there is already an embedded resource with the given name then false is
     * returned. Returns success flag.
     */
    bool addEmbeddedResource(const UnicodeString& name, const Vector<byte_t>& data);

    /**
     * Adds an embedded resource to this scene. If there is already an embedded resource with the given name then false is
     * returned. Returns success flag.
     */
    template <typename T> bool addEmbeddedResource(const UnicodeString& name, T& object)
    {
        auto file = FileWriter();
        file.openMemoryFile();

        try
        {
            object.save(file);
        }
        catch (const Exception&)
        {
            return false;
        }

        return addEmbeddedResource(name, static_cast<const Vector<byte_t>&>(file.getMemoryFileData()));
    }

    /**
     * Removes the specified embedded resource from this scene. Returns success flag.
     */
    bool removeEmbeddedResource(const UnicodeString& name);

    /**
     * Sets the list of collision triangles for this scene from the given triangles.
     */
    bool setupCollisionTriangles(const TriangleArraySet& triangleSet, Runnable& r = Runnable::Empty);

    /**
     * Registers the collision geometry of this scene with the physics system so that rigid bodies can interact with it.
     */
    void makePhysical();

    /**
     * Adds a static mesh directly into this scene with the given transform. Returns success flag.
     */
    bool addStaticMesh(const String& name, const SimpleTransform& transform);

    /**
     * Returns the current root that is prepended to all static meshes added with Scene::addStaticMesh(). This allows easy
     * scoping of all static meshes to a single directory. Defaults to "Static/".
     */
    const String& getStaticMeshRoot() const { return staticMeshRoot_; }

    /**
     * Sets the current static mesh root. See Scene::getStaticMeshRoot() for details.
     */
    void setStaticMeshRoot(const String& root) { staticMeshRoot_ = root; }

    /**
     * Flags all the static meshes that have been added by Scene::addStaticMesh() as shadow casters.
     */
    void setStaticMeshesAsShadowCasters();

    /**
     * This method is intended for use when this scene is being saved and one of its entities has an Entity pointer that it
     * needs to persist. The pointer can't be written directly, so this method must be used to save a bit of data that will
     * allow the entity pointer to be re-established when the scene is loaded. This should be used with
     * Scene::loadEntityReference(). Throws an Exception if an error occurs.
     */
    void saveEntityReference(FileWriter& file, const Entity* entity) const;

    /**
     * Loads an entity reference saved by Scene::saveEntityReference().
     */
    Entity* loadEntityReference(FileReader& file) const;

    /**
     * Queues a scene for rendering with the given camera. If \a camera is null then the first camera found in the scene will be
     * used.
     */
    void queueForRendering(const Camera* camera = nullptr, int priority = 0);

    /**
     * Renders this scene into the given 2D texture using the given camera. If \a camera is null then the first camera found in
     * the scene will be used. This method requires that the graphics hardware supports offscreen render targets, support for
     * this feature can be queried using GraphicsInterface::isRenderTargetSupported(). Returns success flag.
     */
    bool renderToTexture(Texture2D* texture, Camera* camera = nullptr);

    /**
     * Renders this scene into the given cubemap texture using the given camera. This method is only supported on 3D scenes. If
     * \a camera is null then the first camera found in the scene will be used. This method requires that the graphics hardware
     * supports cubemaps and offscreen render targets. Returns success flag.
     */
    bool renderToTexture(TextureCubemap* texture, Camera* camera = nullptr);

    bool isDeferredLightingEnabled() const override { return isDeferredLightingEnabled_; }

    /**
     * Sets whether deferred lighting should be used when rendering this scene. Renderer::isDeferredLightingSupported() can be
     * used to check for hardware support if desired. Defaults to false.
     */
    void setDeferredLightingEnabled(bool enabled) { isDeferredLightingEnabled_ = enabled; }

    /**
     * Adds a colored line to this scene's immediate geometry. Immediate geometry is simple colored geometry that is rendered as
     * part of the scene but isn't stored in a separate entity. It is often convenient when debugging but can be used for other
     * purposes as well.
     */
    void addImmediateGeometry(const Vec3& start, const Vec3& end, const Color& startColor, const Color& endColor);

    /**
     * \copydoc addImmediateGeometry(const Vec3 &, const Vec3 &, const Color &, const Color &)
     */
    void addImmediateGeometry(const Vec3& start, const Vec3& end, const Color& color = Color::White)
    {
        addImmediateGeometry(start, end, color, color);
    }

    /**
     * Adds the passed AABB to this scene's immediate geometry, internally this adds the edges of the AABB as individual lines.
     */
    void addImmediateGeometry(const AABB& aabb, const SimpleTransform& transform = SimpleTransform::Identity,
                              const Color& color = Color::White);

    /**
     * Adds the passed line loop to this scene's immediate geometry.
     */
    void addImmediateGeometry(const Vector<Vec3>& vertices, const SimpleTransform& transform = SimpleTransform::Identity,
                              const Color& color = Color::White);

    /**
     * Clears any immediate geometry currently present on this scene, see Scene::addImmediateGeometry() for details.
     */
    void clearImmediateGeometry();

    /**
     * Causes the given texture to stay loaded regardless of whether any entities are holding a reference to it or not. This
     * effectively causes this scene to hold a reference to the given texture which ensures that it will not be unloaded during
     * the lifecycle of this scene. Texture references held by a scene can be released using Scene::releasePrecachedTexture() or
     * Scene::releasePrecachedTextures(), and all texture references held by the scene are automatically cleaned up on scene
     * shutdown. Returns success flag.
     */
    bool precacheTexture(const String& name, GraphicsInterface::TextureType textureType = GraphicsInterface::Texture2D);

    /**
     * Calls Scene::precacheTexture() for every file in the given directory.
     */
    void precacheTextureDirectory(const String& directory, bool recursive = true,
                                  GraphicsInterface::TextureType textureType = GraphicsInterface::Texture2D);

    /**
     * Releases the texture reference held by this scene on the given texture that was precached with either
     * Scene::precacheTexture() or Scene::precacheTextureDirectory(). Returns success flag.
     */
    bool releasePrecachedTexture(const String& name);

    /**
     * Releases any texture references being held by this scene, see Scene::precacheTexture() for details.
     */
    void releasePrecachedTextures();

    /**
     * Logs a debug trace showing the entire entity hierarchy of this scene with details on each entity.
     */
    void debugTrace(const Entity* root = nullptr) const;

    /**
     * Returns a list of all the entities in this scene that are of the specified type. The entities are returned in the passed
     * \a entities vector.
     */
    template <typename EntityType> void getEntities(Vector<EntityType*>& entities, bool includeInternalEntities = false)
    {
        entities.clear();

        for (auto entity : entities_)
        {
            auto e = dynamic_cast<EntityType*>(entity);

            if (e && (!e->isInternalEntity() || includeInternalEntities))
                entities.append(e);
        }
    }

    /**
     * Proxies through to Camera::screenToWorld() on this scene's default camera, or returns the point unchanged if this scene
     * has no cameras.
     */
    Vec3 screenToWorld(const Vec3& p) const;

    /**
     * 2D version of Scene::screenToWorld() that takes and returns a Vec2 rather than a Vec3.
     */
    Vec2 screenToWorld2D(const Vec2& p) const { return screenToWorld(p).toVec2(); }

    /**
     * Returns a vector containing all the instances of the Scene class.
     */
    static Vector<Scene*>& getAllScenes() { return allScenes_; }

    /**
     * Returns the scene with the given name, or null if there is no scene with that name.
     */
    static Scene* getScene(const String& name);

    /**
     * Returns the entity with the given name in the scene with the given name, or null if there is no scene or entity with the
     * specified names.
     */
    template <typename EntityType> static EntityType* getSceneEntity(const String& sceneName, const String& entityName)
    {
        auto scene = getScene(sceneName);
        if (!scene)
            return nullptr;

        return scene->getEntity<EntityType>(entityName);
    }

    /**
     * Returns the entity with the given name in the scene with the given name, or null if there is no scene or entity with the
     * specified names.
     */
    static Entity* getSceneEntity(const String& sceneName, const String& entityName)
    {
        auto scene = getScene(sceneName);
        if (!scene)
            return nullptr;

        return scene->getEntity(entityName);
    }

    /**
     * Returns the post-process materials that are being applied globally to the combined output of all rendered scenes. If
     * multiple post-process materials are specified then they are applied consecutively.
     */
    static const Vector<String>& getGlobalPostProcessMaterials() { return globalPostProcessMaterials_; }

    /**
     * Sets the post-process materials to apply globally to the combined output of all rendered scenes. If multiple post-process
     * materials are specified then they are applied consecutively.
     */
    static void setGlobalPostProcessMaterials(const Vector<String>& materials);

    /**
     * Adds a material to use for global post-processing on the combined output of all rendered scenes, the new material is
     * appended to the existing list of global post-process materials.
     */
    static void addGlobalPostProcessMaterial(const String& material);

    /**
     * Removes the specified global post-process material, returns success flag.
     */
    static bool removeGlobalPostProcessMaterial(const String& material);

    /**
     * Removes all global post-process materials.
     */
    static void clearGlobalPostProcessMaterials();

    /**
     * The supported Oculus Rift modes, the Rift can be disabled, enabled alongside the default rendering output which is useful
     * when testing, or rendered to exclusively with no output shown on the default graphics output.
     */
    enum OculusRiftMode
    {
        OculusRiftDisabled,
        OculusRiftAndDefaultOutput,
        OculusRiftExclusive
    };

    /**
     * Returns whether Oculus Rift rendering is currently enabled.
     */
    OculusRiftMode getOculusRiftMode() const { return oculusRiftMode_; }

    /**
     * Sets the Oculus Rift rendering is enabled. Returns success flag.
     */
    bool setOculusRiftMode(OculusRiftMode mode);

private:

    String name_;

    bool is2D_ = false;
    bool isEnabled_ = true;
    bool isVisible_ = true;
    bool isDepthClearEnabled_ = true;

    // Entities
    UnorderedPointerSet<Entity> entities_;
    ComplexEntity* rootEntity_ = nullptr;
    void ensureRootEntityExists();

    mutable Vector<const Entity*>* entitiesBeingSaved_ = nullptr;

    // These entities require Entity::update() to be called every frame, see Entity::isPerFrameUpdateRequired() for details
    Vector<Entity*> entitiesRequiringUpdate_;

    // The cameras and lights in the scene are cached for speed
    Vector<Camera*> cameras_;
    Vector<Light*> lights_;

    // Used to restore entity references when a scene is being loaded from file. Entity references are stored as indices into
    // this array. See Scene::saveEntityReference() and Scene::loadEntityReference() for details.
    Vector<Entity*> entitiesBeingLoaded_;

    String backgroundMaterial_;

    // For 2D scenes this is the window that currently has focus
    GUIWindow* focusWindow_ = nullptr;

    // The post-processing materials to apply to this scene
    Vector<String> postProcessMaterials_;

    mutable EffectQueueArray postProcessEffects_;
    mutable bool isPostProcessEffectsDirty_ = true;

    bool isPostProcessPassThroughEnabled_ = false;

    // Updates the specified list of post-process rendering effects from the specified post-process materials
    static void updatePostProcessEffects(EffectQueueArray& effects, const Vector<String>& postProcessMaterials);

    // Embedded resources are stored in a simple file system volume so they can be easily made accessible through the virtual
    // file system
    SimpleFileSystemVolume embeddedResources_;

    // Collision triangles
    Vector<Vec3> collisionVertices_;
    Vector<RawIndexedTriangle> collisionTriangles_;

    // Physical bodies used by the scene
    Vector<PhysicsInterface::BodyObject> bodies_;
    void clearBodies();
    Vector<byte_t> preProcessedPhysicsData_;

    String staticMeshRoot_;
    struct StaticMeshInfo
    {
        String name;
        SimpleTransform transform;

        // The node that this static mesh is attached to
        CullingNode* node = nullptr;

        StaticMeshInfo() {}
        StaticMeshInfo(String meshName, const SimpleTransform& transform_, CullingNode* node_)
            : name(std::move(meshName)), transform(transform_), node(node_)
        {
        }
    };
    Vector<StaticMeshInfo> staticMeshes_;

    bool isDeferredLightingEnabled_ = false;

    Vector<std::pair<Renderer::Camera, GraphicsInterface::OutputDestination>>
        getRendererCameras(const Camera* camera, const Vec2& targetDimensions, float targetFinalDisplayAspectRatio,
                           bool isOculusRiftEnabled);

    GeometryChunk immediateGeometryChunk_;
    unsigned int usedImmediateVertexCount_ = 0;

    Vector<const Texture*> textureReferences_;

    Vector<String> worldGeometryMaterials_;

    friend class Entity;
    friend class ComplexEntity;

    void addEntityToInternalArray(Entity* entity);
    void removeEntityFromInternalArray(Entity* entity);
    void recheckEntityIsPerFrameUpdateRequired(Entity* entity);

    static Vector<Scene*> allScenes_;

    // Global post-process materials
    static Vector<String> globalPostProcessMaterials_;
    static void updateRendererGlobalPostProcessEffects();

    OculusRiftMode oculusRiftMode_ = OculusRiftDisabled;
};

}
