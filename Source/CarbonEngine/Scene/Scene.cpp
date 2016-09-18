/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Exporters/ExportInfo.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Geometry/TriangleArraySet.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Math/HashFunctions.h"
#include "CarbonEngine/Math/Line.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Physics/PhysicsInterface.h"
#include "CarbonEngine/Platform/FrameTimers.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/SimpleTimer.h"
#include "CarbonEngine/Render/DataBufferManager.h"
#include "CarbonEngine/Render/Effect.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/TextureCubemap.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"
#include "CarbonEngine/Scene/Camera.h"
#include "CarbonEngine/Scene/ComplexEntity.h"
#include "CarbonEngine/Scene/CullingNode.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/GUI/GUIEvents.h"
#include "CarbonEngine/Scene/GUI/GUIWindow.h"
#include "CarbonEngine/Scene/IntersectionResult.h"
#include "CarbonEngine/Scene/Light.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/Mesh/Mesh.h"
#include "CarbonEngine/Scene/Mesh/MeshManager.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Scene/SceneEvents.h"

namespace Carbon
{

const UnicodeString Scene::SceneDirectory = "Scenes/";
const UnicodeString Scene::SceneExtension = ".scene";
const String Scene::StaticMeshConversionPrefix = "mesh_";

const auto SceneHeaderID = FileSystem::makeFourCC("cscn");
const auto SceneVersionInfo = VersionInfo(1, 13);

CARBON_DEFINE_FRAME_TIMER(SceneGatherTimer, Color(0.5f, 0.3f, 1.0f))

Vector<Scene*> Scene::allScenes_;
Vector<String> Scene::globalPostProcessMaterials_;

Scene::Scene(const String& name, bool is2D)
    : embeddedResources_("Scene" + UnicodeString::toHex(HashFunctions::hash(this)))
{
    clear();
    setName(name);
    setIs2D(is2D);

    fileSystem().addVolume(&embeddedResources_);

    allScenes_.append(this);

    events().addHandler<UpdateEvent>(this, true);
}

Scene::~Scene()
{
    clear();
    events().removeHandler(this);

    fileSystem().removeVolume(&embeddedResources_);

    allScenes_.eraseValue(this);
}

void Scene::clear()
{
    removeAllEntities();

    setName(String::Empty);
    setIs2D(false);
    setEnabled(true);
    setVisible(true);
    setDepthClearEnabled(true);

    entitiesBeingLoaded_.clear();

    staticMeshRoot_ = "Static/";

    backgroundMaterial_.clear();
    postProcessMaterials_.clear();
    postProcessEffects_.clear();
    isPostProcessEffectsDirty_ = false;
    isPostProcessPassThroughEnabled_ = false;

    clearEmbeddedResources();

    isDeferredLightingEnabled_ = false;

    immediateGeometryChunk_.clear();
    usedImmediateVertexCount_ = 0;

    releasePrecachedTextures();

    worldGeometryMaterials_.clear();

    oculusRiftMode_ = OculusRiftDisabled;
}

void Scene::removeAllEntities()
{
    clearBodies();
    staticMeshes_.clear();

    // Clear out the scene graph
    if (entities_.size())
    {
        // Get a list of all the entities in this scene ordered such that children always appear after their parent in
        // the list
        auto allEntities = Vector<Entity*>(1, getRootEntity());
        allEntities.reserve(entities_.size());
        for (auto i = 0U; i < allEntities.size(); i++)
        {
            auto entity = allEntities[i];

            if (auto complex = dynamic_cast<ComplexEntity*>(entity))
            {
                for (auto child : complex->children_)
                    allEntities.append(child);
            }
        }

        // Starting at the leaves of the scene graph work backwards towards the root wiping the hierarchy at each level
        for (auto i = 1U; i < allEntities.size(); i++)
            allEntities[allEntities.size() - i]->removeFromScene();

        // Delete the root entity
        allEntities[0]->scene_ = nullptr;
        SubclassRegistry<Entity>::destroy(allEntities[0]);
    }

    entities_.clear();
    entitiesRequiringUpdate_.clear();
    cameras_.clear();
    lights_.clear();

    rootEntity_ = nullptr;

    focusWindow_ = nullptr;

    collisionVertices_.clear();
    collisionTriangles_.clear();
    preProcessedPhysicsData_.clear();
}

void Scene::setIs2D(bool is2D)
{
    if (is2D_ == is2D)
        return;

    if (this->is2D())
    {
        events().removeHandler<CharacterInputEvent>(this);
        events().removeHandler<GameControllerButtonDownEvent>(this);
        events().removeHandler<GameControllerButtonUpEvent>(this);
        events().removeHandler<KeyDownEvent>(this);
        events().removeHandler<KeyUpEvent>(this);
        events().removeHandler<MouseButtonDownEvent>(this);
        events().removeHandler<MouseButtonUpEvent>(this);
        events().removeHandler<MouseMoveEvent>(this);
        events().removeHandler<MouseWheelEvent>(this);
        events().removeHandler<TouchBeginEvent>(this);
        events().removeHandler<TouchEndEvent>(this);
        events().removeHandler<TouchMoveEvent>(this);
    }
    else
    {
        events().addHandler<CharacterInputEvent>(this);
        events().addHandler<GameControllerButtonDownEvent>(this);
        events().addHandler<GameControllerButtonUpEvent>(this);
        events().addHandler<KeyDownEvent>(this);
        events().addHandler<KeyUpEvent>(this);
        events().addHandler<MouseButtonDownEvent>(this);
        events().addHandler<MouseButtonUpEvent>(this);
        events().addHandler<MouseMoveEvent>(this);
        events().addHandler<MouseWheelEvent>(this);
        events().addHandler<TouchBeginEvent>(this);
        events().addHandler<TouchEndEvent>(this);
        events().addHandler<TouchMoveEvent>(this);
    }

    is2D_ = is2D;
}

void Scene::setEnabled(bool enabled)
{
    isEnabled_ = enabled;

    // Disabling a scene stops any current window drags and resizes that are going on, this is because now that the
    // scene is disabled it won't get the mouse button released event that normally terminates these operations
    if (!enabled && is2D())
    {
        for (auto entity : entities_)
        {
            if (auto window = dynamic_cast<GUIWindow*>(entity))
            {
                window->setIsBeingDragged(false);
                window->isBeingResized_ = false;
            }
        }
    }
}

bool Scene::addEntity(Entity* entity)
{
    ensureRootEntityExists();
    return getRootEntity()->addChild(entity);
}

void Scene::ensureRootEntityExists()
{
    // Setup root entity if this is the first entity added
    if (entities_.empty())
    {
        rootEntity_ = SubclassRegistry<Entity>::create<ComplexEntity>();
        rootEntity_->setName("Root");
        rootEntity_->scene_ = this;

        entities_.insert(rootEntity_);
    }
}

Rect Scene::getDefaultCameraOrthographicRect() const
{
    auto camera = getDefaultCamera();
    if (!camera)
        return {0.0f, 0.0f, platform().getWindowWidthf(), platform().getWindowHeightf()};

    auto size = camera->getOrthographicSize();

    auto& cameraWorldPosition = camera->getWorldPosition();

    return {cameraWorldPosition.x, cameraWorldPosition.y, cameraWorldPosition.x + size.x,
            cameraWorldPosition.y + size.y};
}

Camera* Scene::create2DCamera(float orthographicWidth, float orthographicHeight)
{
    setIs2D(true);

    auto camera = addEntity<Camera>();

    camera->setOrthographicSize(orthographicWidth, orthographicHeight);
    camera->setNearPlaneDistance(-100.0f);
    camera->setFarPlaneDistance(100.0f);

    return camera;
}

ComplexEntity* Scene::getRootEntity()
{
    ensureRootEntityExists();
    return static_cast<ComplexEntity*>(rootEntity_);
}

const ComplexEntity* Scene::getRootEntity() const
{
    return entities_.empty() ? nullptr : static_cast<ComplexEntity*>(rootEntity_);
}

Camera* Scene::getDefaultCamera()
{
    return cameras_.size() ? cameras_[0] : nullptr;
}

const Camera* Scene::getDefaultCamera() const
{
    return cameras_.size() ? cameras_[0] : nullptr;
}

bool Scene::setFocusWindow(GUIWindow* window)
{
    if (window && window->getScene() != this)
    {
        LOG_ERROR << "Window is not in this scene";
        return false;
    }

    // Remove focus from previous window
    if (focusWindow_)
        focusWindow_->setHasFocus(false);

    focusWindow_ = window;

    // Give focus to new window
    if (window)
        window->setHasFocus(true);

    return true;
}

bool Scene::processEvent(const Event& e)
{
    if (e.as<UpdateEvent>())
    {
        // Call Entity::update() on all entities in this scene that currently return true from
        // Entity::isPerFrameUpdateRequired(). Note that the current list of entities in this scene that return true
        // from this method is cached to avoid checking every entity every frame. See
        // Entity::recheckIsPerFrameUpdateRequired() for details.

        for (auto entity : entitiesRequiringUpdate_)
            entity->update();

        return true;
    }

    if (!isEnabled_)
        return true;

    if (is2D())
    {
        // For 2D scenes, handle keyboard and mouse events

        // The entity with focus gets first go at all incoming scene events and can stop them from reaching other GUI
        // windows by returning false
        auto originalFocusWindow = getFocusWindow();
        auto focusWindowSwallowedEvent = originalFocusWindow && !originalFocusWindow->processEvent(e);

        // Do a pass over all the GUI windows for mouse enter/exit handling and for focus change handling
        auto currentFocusWindow = originalFocusWindow;
        for (auto entity : entities_)
        {
            auto window = dynamic_cast<GUIWindow*>(entity);

            if (window && window->isInteractive())
            {
                // Handle mouse enter and exit events
                if (auto mme = e.as<MouseMoveEvent>())
                    window->doMouseEnterExitHandling(mme->getPosition());

                // Handle focus changes at the scene level
                if (auto mbde = e.as<MouseButtonDownEvent>())
                {
                    if (!focusWindowSwallowedEvent && window != currentFocusWindow)
                    {
                        if (mbde->getButton() == LeftMouseButton &&
                            window->intersect(screenToWorld(mbde->getPosition())) && window->isVisible())
                        {
                            if (currentFocusWindow)
                                currentFocusWindow->setHasFocus(false);

                            // Update window with focus
                            focusWindow_ = window;
                            window->setHasFocus(true);
                            currentFocusWindow = window;
                        }
                    }
                }
            }
        }

        // Send event to all other GUI windows as long as the window with focus didn't swallow it
        if (!focusWindowSwallowedEvent)
        {
            auto swallower = std::find_if(entities_.begin(), entities_.end(), [&](Entity* entity) {
                auto window = dynamic_cast<GUIWindow*>(entity);

                // The GUIWindow must be interactive to get passed events
                if (window && window != originalFocusWindow && window->isInteractive())
                {
                    // If the window swallows the event then propagate that swallow to the
                    // entire engine
                    if (!window->processEvent(e))
                        return true;
                }

                return false;
            });

            if (swallower != entities_.end())
                return false;
        }
    }

    return true;
}

void Scene::gatherGeometry(const Vec3& cameraPosition, const ConvexHull& frustum, EffectQueueArray& queues)
{
    auto timer = ScopedFrameTimer(SceneGatherTimer);

    auto gather = GeometryGather(cameraPosition, frustum, false, queues);

    // Put the background material for 2D scenes on first if there is one set
    if (is2D() && backgroundMaterial_.length())
    {
        gather.changePriority(INT_MIN);

        auto backgroundMaterialOverrideParameters = getRootEntity()->getMaterialOverrideParameters(backgroundMaterial_);
        gather.changeMaterial(backgroundMaterial_, backgroundMaterialOverrideParameters);

        auto rect = getDefaultCameraOrthographicRect();

        gather.changeTransformation(Vec3(rect.getLeft(), rect.getBottom()));
        gather.addRectangle(rect.getWidth(), rect.getHeight());
    }

    if (usedImmediateVertexCount_)
    {
        immediateGeometryChunk_.registerWithRenderer();

        gather.changePriority(INT_MAX);
        gather.changeMaterial("ImmediateGeometry");
        gather.changeTransformation(getRootEntity()->getWorldTransform(), Vec3::One);
        gather.addGeometryChunk(immediateGeometryChunk_);
    }

    getRootEntity()->gatherGeometry(gather);
}

void Scene::gatherShadowGeometry(const Vec3& cameraPosition, const ConvexHull& frustum, EffectQueueArray& queues,
                                 AABB* extraWorldSpaceShadowCasterExtents)
{
    auto timer = ScopedFrameTimer(SceneGatherTimer);

    auto gather = GeometryGather(cameraPosition, frustum, true, queues);

    getRootEntity()->gatherGeometry(gather);

    if (extraWorldSpaceShadowCasterExtents)
        *extraWorldSpaceShadowCasterExtents = gather.getExtraWorldSpaceShadowCasterExtents();
}

void Scene::gatherLights(const ConvexHull& area, Vector<Renderer::Light*>& lights, Color& ambientLightColor)
{
    lights.clear();

    if (lights_.empty())
    {
        ambientLightColor = Color::White;
        return;
    }

    auto directionalLightCount = 0U;
    auto pointLightCount = 0U;

    ambientLightColor = Color::Zero;

    // Get all the lights
    for (auto light : lights_)
    {
        // Accumulate the ambient light color
        if (light->getType() == Light::AmbientLight)
        {
            ambientLightColor += light->getColor();
            continue;
        }

        // Cull point lights and spot lights
        if ((light->isPointLight() || light->isSpotLight()) &&
            (light->getRadius() <= 0.0f || !area.intersect(Sphere(light->getWorldPosition(), light->getRadius()))))
            continue;

        // The resulting lights vector is sorted according to light type and properties in order to reduce state changes
        // in the renderer. The ordering of lights is directional, point, spot. TODO: also sort by whether or not
        // specular and a 2D projection texture is enabled

        if (light->isDirectionalLight())
            lights.insert(directionalLightCount++, light);
        else if (light->isPointLight())
        {
            lights.insert(directionalLightCount, light);
            pointLightCount++;
        }
        else if (light->isSpotLight())
        {
            lights.insert(directionalLightCount + pointLightCount, light);

#ifdef CARBON_DEBUG
            if (light->getMinimumConeAngle() > light->getMaximumConeAngle())
            {
                LOG_WARNING << "The spotlight '" << light->getName() << "' at position " << light->getWorldPosition()
                            << " has incongruent cone min/max angles, this will cause lighting artifacts";
            }
#endif
        }
    }
}

void Scene::save(FileWriter& file) const
{
    // Write header
    file.write(SceneHeaderID);
    file.beginVersionedSection(SceneVersionInfo);

    // Write basic scene info
    file.write(name_);
    file.writeBytes(nullptr, 1);
    file.write(isEnabled_);
    file.writeBytes(nullptr, 4);
    file.write(entities_.size());
    file.writeBytes(nullptr, 12);

    // Go through entities writing them to the file
    auto entitiesVector = Vector<const Entity*>();
    for (auto entity : entities_)
        entitiesVector.append(entity);
    entitiesBeingSaved_ = &entitiesVector;
    try
    {
        for (auto entity : entitiesVector)
        {
            file.writeBytes(nullptr, 1);
            file.write(entity->getEntityTypeName(), *entity);
        }

        entitiesBeingSaved_ = nullptr;
    }
    catch (...)
    {
        entitiesBeingSaved_ = nullptr;
        throw;
    }

    // Write background material
    file.write(backgroundMaterial_);

    // Write scene color (now unused, so just write zeros)
    file.writeBytes(nullptr, sizeof(Color));

    // Write 2D flag
    file.write(is2D_);
    file.writeBytes(nullptr, 3);

    // Write embedded resources
    file.write(embeddedResources_);

    // Write collision vertices and triangles
    file.write(collisionVertices_, collisionTriangles_);

    file.write(ExportInfo::get(), postProcessMaterials_, String::Empty, isVisible_, preProcessedPhysicsData_,
               worldGeometryMaterials_, isDepthClearEnabled_);

    file.endVersionedSection();

    LOG_INFO << "Saved scene - '" << getName() << "'";
}

bool Scene::save(const String& name) const
{
    try
    {
        auto file = FileWriter();
        fileSystem().open(SceneDirectory + name + SceneExtension, file);

        save(file);

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        return false;
    }
}

void Scene::load(FileReader& file, ExportInfo& exportInfo)
{
    try
    {
        // Clear the scene but maintain the static mesh root as it will be reset by clear() but is needed later in this
        // method when changing the static mesh entities into real static meshes
        auto originalStaticMeshRoot = getStaticMeshRoot();
        clear();
        setStaticMeshRoot(originalStaticMeshRoot);

        exportInfo.clear();

        // Check header
        if (file.readFourCC() != SceneHeaderID)
            throw Exception("Not a scene file");

        auto readVersion = file.beginVersionedSection(SceneVersionInfo);

        // Read scene data
        auto entityCount = 0U;
        auto unusedParameters = ParameterArray();
        file.read(name_);
        file.skip(1);
        file.read(isEnabled_);
        file.skip(4);
        file.read(entityCount);
        file.skip(8);
        file.read(unusedParameters);

        try
        {
            entitiesBeingLoaded_.resize(entityCount);
            auto entityFileOffsets = Vector<unsigned int>(entityCount);

            // Instantiate entities, don't actually load them yet
            for (auto i = 0U; i < entityCount; i++)
            {
                file.skip(1);

                // Read entity type
                auto entityType = String();
                file.read(entityType);

                // Instantiate this entity. The type string in the scene file should be the fully qualified entity class
                // name, however older scenes leave off the 'Carbon::' namespace prefix so if the type string as-is
                // isn't recognized then 'Carbon::' is prepended and the lookup is attempted again.
                auto entity = SubclassRegistry<Entity>::create(entityType);
                if (!entity)
                {
                    entity = SubclassRegistry<Entity>::create("Carbon::" + entityType);
                    if (!entity)
                        throw Exception() << "Failed creating entity of type " << entityType;
                }

                // Store the offset to the start of this entity's data and then skip over it
                entityFileOffsets[i] = file.getPosition();
                while (true)
                {
                    auto id = byte_t();
                    file.read(id);
                    file.setPosition(file.getPosition() - 1);
                    if (id != FileSystem::SectionBeginID)
                        break;

                    // Skip past a versioned section, we have no idea what the contents are
                    static const auto UnknownVersionInfo = VersionInfo(1000, 0);
                    file.beginVersionedSection(UnknownVersionInfo);
                    file.endVersionedSection();
                }

                entitiesBeingLoaded_[i] = entity;
                entities_.insert(entity);
            }

            // Read entities
            for (auto i = 0U; i < entityCount; i++)
            {
                auto e = entitiesBeingLoaded_[i];

                file.setPosition(entityFileOffsets[i]);
                e->scene_ = this;
                e->load(file);

                // Detect the root entity
                if (e->isEntityType<ComplexEntity>() && e->getParent() == nullptr && e->getName() == "Root")
                    rootEntity_ = static_cast<ComplexEntity*>(entitiesBeingLoaded_[i]);
            }

            entitiesBeingLoaded_.clear();

            // Check that a root entity was found
            if (!rootEntity_)
                throw Exception("Scene has no root entity");
        }
        catch (const Exception&)
        {
            // Safely clean up all entities, some may be loaded and others may not, but just get rid of everything
            for (auto entity : entities_)
            {
                entity->clear();
                entity->scene_ = nullptr;
                entity->parent_ = nullptr;

                if (auto complex = dynamic_cast<ComplexEntity*>(entity))
                    complex->children_.clear();
            }

            for (auto entity : entities_)
                SubclassRegistry<Entity>::destroy(entity);

            entities_.clear();
            entitiesBeingLoaded_.clear();
            rootEntity_ = nullptr;

            throw;
        }

        // v1.1, background material
        if (readVersion.getMinor() >= 1)
            file.read(backgroundMaterial_);

        // v1.2, scene color (unused)
        if (readVersion.getMinor() >= 2)
            file.skip(sizeof(Color));

        // v1.3, scene 2D flag
        if (readVersion.getMinor() >= 3)
        {
            auto is2D = false;
            file.read(is2D);
            setIs2D(is2D);

            file.skip(3);
        }

        // v1.4, embedded resources
        if (readVersion.getMinor() >= 4)
        {
            file.read(embeddedResources_);

            // Fix any old embedded .mesh files that use a lowercase "meshes/" rather than "Meshes/" as their directory
            auto resourceNames = Vector<UnicodeString>();
            embeddedResources_.enumerateFiles("", "", true, resourceNames);
            for (auto resourceName : resourceNames)
            {
                if (resourceName.startsWith(Mesh::MeshDirectory.asLower()))
                {
                    embeddedResources_.renameFile(resourceName, Mesh::MeshDirectory +
                                                      resourceName.substr(Mesh::MeshDirectory.length()));
                }
            }
        }

        // v1.5, collision vertices and triangles
        if (readVersion.getMinor() >= 5)
            file.read(collisionVertices_, collisionTriangles_);

        // v1.6, export info
        if (readVersion.getMinor() >= 6)
            file.read(exportInfo);

        // v1.7, post-process materials
        if (readVersion.getMinor() >= 7)
            file.read(postProcessMaterials_);

        // v1.8, GUID, no longer used
        if (readVersion.getMinor() >= 8)
        {
            auto unused = String();
            file.read(unused);
        }

        // v1.9, visibility flag
        if (readVersion.getMinor() >= 9)
            file.read(isVisible_);

        // v1.10, preprocessed physics data
        if (readVersion.getMinor() >= 10)
            file.read(preProcessedPhysicsData_);

        // v1.11 removed entity IDs

        // v1.12, world geometry materials
        if (readVersion.getMinor() >= 12)
            file.read(worldGeometryMaterials_);

        // v1.13, depth clear enabled flag
        if (readVersion.getMinor() >= 13)
            file.read(isDepthClearEnabled_);

        file.endVersionedSection();

        // Check the root entity is present and correct
        if (entities_.size() > 1)
        {
            if (!getRootEntity()->isEntityType<ComplexEntity>() || getRootEntity()->getName() != "Root")
                throw Exception("Invalid root entity");
        }

        getRootEntity()->invalidateWorldTransform();

        // Convert static mesh entities into proper static meshes in the scene
        auto staticMeshEntities = Vector<Entity*>();
        for (auto entity : entities_)
        {
            if (entity->getName().startsWith(StaticMeshConversionPrefix))
                staticMeshEntities.append(entity);
        }
        for (auto entity : staticMeshEntities)
        {
            // Strip off prefix to get the mesh name
            auto meshName = entity->getName().substr(StaticMeshConversionPrefix.length());

            // Everything after the last underscore is removed as this is used to make the entity names unique and so
            // shouldn't be treated as part of the mesh name
            auto index = meshName.findLastOf("_");
            if (index != -1)
                meshName = meshName.substr(0, index);

            // Add the new static mesh
            if (!addStaticMesh(meshName, entity->getWorldTransform()))
                LOG_WARNING << "Failed adding static mesh: " << meshName;

            entity->removeFromScene();
        }

        // Initialize the entitiesRequiringUpdate_, cameras_ and lights_ lists
        for (auto entity : entities_)
        {
            entity->recheckIsPerFrameUpdateRequired();

            if (entity->isEntityType<Camera>())
                cameras_.append(static_cast<Camera*>(entity));
            if (entity->isEntityType<Light>())
                lights_.append(static_cast<Light*>(entity));
        }
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

bool Scene::load(const String& filename)
{
    try
    {
        auto timer = SimpleTimer();

        auto file = FileReader();
        fileSystem().open(SceneDirectory + filename + SceneExtension, file);

        auto exportInfo = ExportInfo();
        load(file, exportInfo);

        if (!file.isEOF())
            LOG_WARNING << "Scene load was successful, but not all data in the file was read";

        LOG_INFO << "Loaded scene - '" << filename << "' - export info: " << exportInfo << ", time: " << timer;

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << filename << "' - " << e;

        return false;
    }
}

bool Scene::rewriteEmbeddedMeshes()
{
    try
    {
        for (const auto& name : getEmbeddedResources())
        {
            if (name.startsWith(Mesh::MeshDirectory) && name.endsWith(Mesh::MeshExtension))
            {
                auto file = FileReader();
                if (getEmbeddedResource(name, file))
                {
                    auto mesh = Mesh();
                    mesh.load(file);

                    addEmbeddedResource(name, mesh);
                }
            }
        }

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        return false;
    }
}

bool Scene::isWorldGeometryMaterial(const String& materialName) const
{
    return worldGeometryMaterials_.empty() || worldGeometryMaterials_.has(materialName);
}

void Scene::addWorldGeometryMaterial(const String& materialName)
{
    worldGeometryMaterials_.append(materialName);
}

void Scene::clearWorldGeometryMaterials()
{
    worldGeometryMaterials_.clear();
}

IntersectionResult Scene::intersect(const Ray& ray, bool onlyWorldGeometry)
{
    auto intersections = Vector<IntersectionResult>();

    if (!intersect(ray, intersections, onlyWorldGeometry))
        return {};

    return intersections[0];
}

IntersectionResult Scene::intersect(const Vec2& pixel, Camera* camera, bool onlyWorldGeometry)
{
    auto intersections = Vector<IntersectionResult>();

    if (!intersect(pixel, intersections, camera, onlyWorldGeometry))
        return {};

    return intersections[0];
}

bool Scene::intersect(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry)
{
    intersections.clear();

    // Verify that the ray is correctly formed before passing it through the scene
    if (!ray.getOrigin().isFinite() || !ray.getDirection().isFinite() || ray.getDirection().length() < 0.95f)
        return false;

    // Get all intersections
    getRootEntity()->intersectRay(ray, intersections, onlyWorldGeometry);

    // Sort the results
    intersections.sort();

    return !intersections.empty();
}

bool Scene::intersect(const Vec2& pixel, Vector<IntersectionResult>& intersections, Camera* camera,
                      bool onlyWorldGeometry)
{
    if (!camera)
    {
        camera = getDefaultCamera();
        if (!camera)
        {
            if (is2D())
            {
                // For 2D scenes fall back to casting a ray directly using the pixel as a 2D world position
                return intersect(Ray(Vec3(pixel.x, pixel.y, 1000.0f), -Vec3::UnitZ), intersections, onlyWorldGeometry);
            }

            LOG_ERROR << "No camera";
            return false;
        }
    }

    return intersect(camera->getRayThroughPixel(pixel), intersections, onlyWorldGeometry);
}

bool Scene::checkLineOfSight(const Vec3& p0, const Vec3& p1)
{
    auto intersection = intersect(Ray(p0, p1 - p0));

    if (!intersection)
        return true;

    return p0.distance(p1) <= p0.distance(intersection.getPoint());
}

void Scene::precache()
{
    auto timer = SimpleTimer();

    // Precache all entities
    getRootEntity()->precache();

    // Precache the post-process materials
    for (const auto& material : postProcessMaterials_)
        materials().getMaterial(material).precache();

    LOG_INFO << "Precached scene '" << getName() << "' - time: " << timer;
}

void Scene::center()
{
    if (is3D() || entities_.empty())
        return;

    auto windowCount = 0U;
    auto aabb = AABB();

    for (auto entity : entities_)
    {
        if (entity->isEntityType<GUIWindow>())
        {
            aabb.merge(entity->getWorldAABB());
            windowCount++;
        }
    }

    if (windowCount == 0)
        return;

    getRootEntity()->move(-aabb.getCenter() + getDefaultCameraOrthographicRect().getPoint(0.5f, 0.5f));
}

bool Scene::hasEmbeddedResource(const UnicodeString& name) const
{
    return embeddedResources_.doesFileExist(name);
}

Vector<UnicodeString> Scene::getEmbeddedResources() const
{
    auto resources = Vector<UnicodeString>();

    embeddedResources_.enumerateFiles(UnicodeString::Empty, UnicodeString::Empty, true, resources);

    return resources;
}

bool Scene::getEmbeddedResource(const UnicodeString& name, FileReader& file) const
{
    return embeddedResources_.open(name, file) == NoFileSystemError;
}

bool Scene::addEmbeddedResource(const UnicodeString& name, const Vector<byte_t>& data)
{
    try
    {
        auto file = FileWriter();

        if (embeddedResources_.open(name, file, false) != NoFileSystemError)
            throw Exception("Failed opening file");

        file.writeBytes(data.getData(), data.size());

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << name << " - " << e;
        return false;
    }
}

bool Scene::removeEmbeddedResource(const UnicodeString& name)
{
    return embeddedResources_.deleteFile(name) == NoFileSystemError;
}

bool Scene::setupCollisionTriangles(const TriangleArraySet& triangleSet, Runnable& r)
{
    // Build vertex and index lists for the given triangle set. We're only interested in the vertex xyz position The
    // GeometryChunk::optimizeVertexData() method is used to do the list building.

    auto geometryChunk = GeometryChunk();

    // Fill the chunk with vertex data
    geometryChunk.addVertexStream({VertexStream::Position, 3});
    if (!geometryChunk.setVertexCount(triangleSet.getTriangleCount() * 3))
        return false;

    auto vertices = geometryChunk.lockVertexData<Vec3>();
    for (auto triangles : triangleSet)
    {
        for (auto& triangle : *triangles)
        {
            *vertices++ = triangle.getVertexPosition(0);
            *vertices++ = triangle.getVertexPosition(1);
            *vertices++ = triangle.getVertexPosition(2);
        }
    }
    geometryChunk.unlockVertexData();

    // Fill the chunk with index data
    geometryChunk.setIndexDataStraight();

    // Optimize the vertex array
    if (!geometryChunk.optimizeVertexData(r))
        return false;

    // Store optimized vertices
    collisionVertices_.resize(geometryChunk.getVertexCount());
    vertices = geometryChunk.lockVertexData<Vec3>();
    for (auto& collisionVertex : collisionVertices_)
        collisionVertex = *vertices++;

    // Store optimized triangle indices
    collisionTriangles_.resize(triangleSet.getTriangleCount());
    for (auto i = 0U; i < collisionTriangles_.size(); i++)
    {
        collisionTriangles_[i].setIndex(0, geometryChunk.getIndexValue(i * 3));
        collisionTriangles_[i].setIndex(1, geometryChunk.getIndexValue(i * 3 + 1));
        collisionTriangles_[i].setIndex(2, geometryChunk.getIndexValue(i * 3 + 2));
    }

    // Attempt to preprocess physics data for this scene
    physics().preProcessGeometry(collisionVertices_, collisionTriangles_, preProcessedPhysicsData_);

    return true;
}

void Scene::makePhysical()
{
    LOG_INFO << "Making scene '" << getName() << "' physical with " << collisionVertices_.size() << " vertices and "
             << collisionTriangles_.size() << " triangles";

    clearBodies();

    // Create body template, either from the preprocessed data if present and valid or from the geometry itself directly
    auto bodyTemplate = PhysicsInterface::BodyTemplateObject();
    if (preProcessedPhysicsData_.size())
        bodyTemplate = physics().createBodyTemplateFromPreProcessedGeometry(preProcessedPhysicsData_, true);

    if (!bodyTemplate)
        bodyTemplate = physics().createBodyTemplateFromGeometry(collisionVertices_, collisionTriangles_, true);

    // Create static physics body for the scene geometry
    if (bodyTemplate)
        bodies_.append(physics().createGeometryBodyFromTemplate(bodyTemplate, 0.0f, true, nullptr));

    // Create bodies for the static meshes
    for (auto& staticMesh : staticMeshes_)
    {
        auto mesh = meshes().getMesh(staticMesh.name);

        bodyTemplate = mesh->getPhysicsBodyTemplate();
        if (bodyTemplate)
        {
            bodies_.append(
                physics().createGeometryBodyFromTemplate(bodyTemplate, 0.0f, true, nullptr, staticMesh.transform));
        }

        meshes().releaseMesh(mesh);
    }
}

void Scene::clearBodies()
{
    for (auto body : bodies_)
        physics().deleteBody(body);

    bodies_.clear();
}

bool Scene::addStaticMesh(const String& name, const SimpleTransform& transform)
{
    auto cullingNodes = getRootEntity()->getChildren<CullingNode>();
    if (cullingNodes.empty())
    {
        LOG_ERROR << "Scene does not have a culling root node";
        return false;
    }

    auto cullingRoot = cullingNodes[0];

    while (true)
    {
        auto done = true;

        for (auto i = 0U; i < cullingRoot->getChildCount(); i++)
        {
            auto child = cullingRoot->getChild(i);
            if (child->isEntityType<CullingNode>())
            {
                if (child->getWorldExtents().intersect(transform.getPosition()))
                {
                    cullingRoot = static_cast<CullingNode*>(child);
                    done = false;
                    break;
                }
            }
        }

        if (done)
            break;
    }

    // Add mesh to the node
    auto fullName = staticMeshRoot_ + name;
    cullingRoot->attachMesh(fullName, transform);

    staticMeshes_.emplace(fullName, transform, cullingRoot);

    return true;
}

void Scene::setStaticMeshesAsShadowCasters()
{
    for (auto& staticMesh : staticMeshes_)
        staticMesh.node->setShadowCaster(staticMesh.name, true);
}

void Scene::saveEntityReference(FileWriter& file, const Entity* entity) const
{
    auto index = -1;
    if (entity)
    {
        // Find the index of this entity in the entities vector
        index = entitiesBeingSaved_->find(entity);
        if (index == -1)
            throw Exception("Entity is not in the scene");
    }

    // Save the index, or -1 if it's a null pointer
    file.write(index);
}

Entity* Scene::loadEntityReference(FileReader& file) const
{
    // Determine what version of scene file this is
    auto readVersion = file.findVersionedSection(SceneVersionInfo);

    auto index = 0;

    // v1.11 removed entity IDs and so indices are used temporarily when saving and loading the scene
    if (readVersion.getMinor() < 11)
    {
        // Read the old ID value and convert it into an index by subtracting one
        auto id = 0U;
        file.read(id);
        index = int(id) - 1;
    }
    else
        file.read(index);

    return (index >= 0 && index < int(entitiesBeingLoaded_.size())) ? entitiesBeingLoaded_[index] : nullptr;
}

Vector<std::pair<Renderer::Camera, GraphicsInterface::OutputDestination>>
    Scene::getRendererCameras(const Camera* camera, const Vec2& targetDimensions, float targetFinalDisplayAspectRatio,
                              bool isOculusRiftEnabled)
{
    auto result = Vector<std::pair<Renderer::Camera, GraphicsInterface::OutputDestination>>();

    if (isVisible())
    {
        if (!camera)
            camera = getDefaultCamera();

        auto renderToDefaultOutput =
            (oculusRiftMode_ == OculusRiftDisabled || oculusRiftMode_ == OculusRiftAndDefaultOutput);
        auto renderToOculusRift =
            (oculusRiftMode_ == OculusRiftAndDefaultOutput || oculusRiftMode_ == OculusRiftExclusive);

        if (camera)
        {
            if (renderToDefaultOutput)
            {
                result.emplace(camera->getRendererCamera(targetDimensions, targetFinalDisplayAspectRatio,
                                                         GraphicsInterface::OutputDefault),
                               GraphicsInterface::OutputDefault);
            }

            if (renderToOculusRift)
            {
                result.emplace(camera->getRendererCamera(targetDimensions, targetFinalDisplayAspectRatio,
                                                         GraphicsInterface::OutputOculusRiftLeftEye),
                               GraphicsInterface::OutputOculusRiftLeftEye);

                result.emplace(camera->getRendererCamera(targetDimensions, targetFinalDisplayAspectRatio,
                                                         GraphicsInterface::OutputOculusRiftRightEye),
                               GraphicsInterface::OutputOculusRiftRightEye);

                const_cast<Camera*>(camera)->setWorldOrientation(
                    platform().getOculusRiftTransformLeftEye().getOrientation().slerp(
                        platform().getOculusRiftTransformRightEye().getOrientation(), 0.5f));
            }
        }
        else
        {
            if (is2D())
            {
                auto viewport = Rect::One * targetDimensions;

                // Default camera for 2D scenes

                if (renderToDefaultOutput)
                {
                    result.emplace(Renderer::Camera(SimpleTransform::Identity, viewport,
                                                    Matrix4::getOrthographicProjection(
                                                        Rect(0.0f, 0.0f, viewport.getWidth(), viewport.getHeight()),
                                                        -100.0f, 100.0f),
                                                    -100.0f, 100.0f),
                                   GraphicsInterface::OutputDefault);
                }

                if (renderToOculusRift)
                {
                    viewport = platform().getOculusRiftTextureDimensions();

                    result.emplace(Renderer::Camera(SimpleTransform::Identity, viewport,
                                                    Matrix4::getOrthographicProjection(
                                                        Rect(0.0f, 0.0f, viewport.getWidth(), viewport.getHeight()),
                                                        -100.0f, 100.0f),
                                                    -100.0f, 100.0f),
                                   GraphicsInterface::OutputOculusRiftLeftEye);

                    result.emplace(Renderer::Camera(SimpleTransform::Identity, viewport,
                                                    Matrix4::getOrthographicProjection(
                                                        Rect(0.0f, 0.0f, viewport.getWidth(), viewport.getHeight()),
                                                        -100.0f, 100.0f),
                                                    -100.0f, 100.0f),
                                   GraphicsInterface::OutputOculusRiftRightEye);
                }
            }
            else
                LOG_ERROR_WITHOUT_CALLER << "Scene '" << getName() << "' has no cameras";
        }
    }

    return result;
}

void Scene::queueForRendering(const Camera* camera, int priority)
{
    auto cameras = getRendererCameras(camera, Vec2(platform().getWindowWidthf(), platform().getWindowHeightf()),
                                      platform().getFinalDisplayAspectRatio(), oculusRiftMode_ != OculusRiftDisabled);

    for (const auto& c : cameras)
        renderer().queueForRendering(this, c.first, priority, c.second);
}

bool Scene::renderToTexture(Texture2D* texture, Camera* camera)
{
    if (!texture)
        return false;

    auto rect = texture->getRect();
    auto cameras = getRendererCameras(camera, rect.getMaximum(), rect.getAspectRatio(), false);
    auto cameraDefinitions = cameras.map<Renderer::Camera>(
        [](const std::pair<Renderer::Camera, GraphicsInterface::OutputDestination>& entry) { return entry.first; });

    return renderer().renderIntoTexture(this, cameraDefinitions, texture);
}

bool Scene::renderToTexture(TextureCubemap* texture, Camera* camera)
{
    if (!camera)
        camera = getDefaultCamera();

    if (!texture || !camera)
        return false;

    // Force the camera to use 90 degree field of view when rendering a cubemap
    auto fov = camera->getFieldOfView();
    camera->setFieldOfView(Math::HalfPi);

    auto rect = texture->getRect();
    auto cameras = getRendererCameras(camera, rect.getMaximum(), rect.getAspectRatio(), false);
    auto cameraDefinitions = cameras.map<Renderer::Camera>(
        [](const std::pair<Renderer::Camera, GraphicsInterface::OutputDestination>& entry) { return entry.first; });

    // Put back the original field of view
    camera->setFieldOfView(fov);

    return renderer().renderIntoTexture(this, cameraDefinitions, texture);
}

struct ImmediateGeometryVertex
{
    Vec3 position;
    Vec2 tc;
    unsigned int color = 0;
};

void Scene::addImmediateGeometry(const Vec3& start, const Vec3& end, const Color& startColor, const Color& endColor)
{
    if (usedImmediateVertexCount_ + 2 > immediateGeometryChunk_.getVertexCount())
    {
        if (immediateGeometryChunk_.getVertexStreams().empty())
        {
            immediateGeometryChunk_.addVertexStream({VertexStream::Position, 3});
            immediateGeometryChunk_.addVertexStream({VertexStream::DiffuseTextureCoordinate, 2});
            immediateGeometryChunk_.addVertexStream({VertexStream::Color, 4, TypeUInt8});

            immediateGeometryChunk_.setDynamic(true);
        }

        immediateGeometryChunk_.unregisterWithRenderer();

        immediateGeometryChunk_.setVertexCount(immediateGeometryChunk_.getVertexCount() + 2);

        auto indices = Vector<unsigned int>(immediateGeometryChunk_.getVertexCount());
        for (auto i = 0U; i < indices.size(); i++)
            indices[i] = i;

        immediateGeometryChunk_.setupIndexData(Vector<DrawItem>{{GraphicsInterface::LineList, indices.size(), 0}},
                                               indices);
    }

    auto vertexLayout = immediateGeometryChunk_.lockVertexData<ImmediateGeometryVertex>();

    vertexLayout[usedImmediateVertexCount_].position = start;
    vertexLayout[usedImmediateVertexCount_].tc = Vec2::Zero;
    vertexLayout[usedImmediateVertexCount_].color = startColor.toRGBA8();
    vertexLayout[usedImmediateVertexCount_ + 1].position = end;
    vertexLayout[usedImmediateVertexCount_ + 1].tc = Vec2::Zero;
    vertexLayout[usedImmediateVertexCount_ + 1].color = endColor.toRGBA8();

    immediateGeometryChunk_.unlockVertexData();

    usedImmediateVertexCount_ += 2;
}

void Scene::addImmediateGeometry(const AABB& aabb, const SimpleTransform& transform, const Color& color)
{
    // Get the edges of the AABB
    auto edges = std::array<Line, 12>();
    aabb.getEdges(edges, transform);

    for (auto& edge : edges)
    {
        addImmediateGeometry(edge.getOrigin(), edge.getOrigin() + (edge.getEnd() - edge.getOrigin()) * 0.3f, color);
        addImmediateGeometry(edge.getEnd(), edge.getEnd() + (edge.getOrigin() - edge.getEnd()) * 0.3f, color);
    }
}

void Scene::addImmediateGeometry(const Vector<Vec3>& vertices, const SimpleTransform& transform, const Color& color)
{
    for (auto i = 0U; i < vertices.size(); i++)
        addImmediateGeometry(transform * vertices[i], transform * vertices[(i + 1) % vertices.size()], color);
}

void Scene::clearImmediateGeometry()
{
    immediateGeometryChunk_.unregisterWithRenderer();

    memset(immediateGeometryChunk_.lockVertexData(), 0, immediateGeometryChunk_.getVertexDataSize());
    immediateGeometryChunk_.unlockVertexData();

    usedImmediateVertexCount_ = 0;
}

bool Scene::precacheTexture(const String& name, GraphicsInterface::TextureType textureType)
{
    auto texture = textures().setupTexture(textureType, name);
    if (!texture)
    {
        LOG_ERROR << "Failed precaching texture: " << name;
        return false;
    }

    textureReferences_.append(texture);

    return true;
}

void Scene::precacheTextureDirectory(const String& directory, bool recursive,
                                     GraphicsInterface::TextureType textureType)
{
    auto fullPath = (directory.startsWith("/") ? String::Empty : A(Texture::TextureDirectory)) + directory;

    auto files = Vector<UnicodeString>();
    fileSystem().enumerateFiles(fullPath, "", recursive, files);

    for (const auto& file : files)
        precacheTexture(A(directory.length() ? FileSystem::joinPaths(directory, file) : file), textureType);
}

bool Scene::releasePrecachedTexture(const String& name)
{
    for (auto texture : textureReferences_)
    {
        if (textures().areTextureNamesEquivalent(name, texture->getName()))
        {
            textures().releaseTexture(texture);
            return true;
        }
    }

    return false;
}

void Scene::releasePrecachedTextures()
{
    for (auto texture : textureReferences_)
        textures().releaseTexture(texture);

    textureReferences_.clear();
}

void Scene::debugTrace(const Entity* root) const
{
    if (!root)
    {
        LOG_DEBUG << "Debug trace for scene '" << getName() << "', enabled: " << isEnabled_
                  << ", entity count: " << entities_.size()
                  << ", entities requiring update: " << entitiesRequiringUpdate_.size();

        root = getRootEntity();
    }

    if (!root)
        return;

    // Work out how deep in the tree this entity is
    auto depth = 0U;
    auto e = root->getParent();
    while (e)
    {
        e = e->getParent();
        depth++;
    }

    LOG_DEBUG << String(' ', depth * 4) << "- " << *root;

    // Recurse through all children
    if (auto complex = dynamic_cast<const ComplexEntity*>(root))
    {
        for (auto child : complex->getChildren())
            debugTrace(child);
    }
}

Vec3 Scene::screenToWorld(const Vec3& p) const
{
    auto camera = getDefaultCamera();

    return camera ? camera->screenToWorld(p) : p;
}

void Scene::addEntityToInternalArray(Entity* entity)
{
    entities_.insert(entity);

    if (entity->isPerFrameUpdateRequired())
        entitiesRequiringUpdate_.append(entity);

    entity->scene_ = this;

    // Update cached cameras_ and lights_ arrays
    if (entity->isEntityType<Camera>())
        cameras_.append(static_cast<Camera*>(entity));
    else if (entity->isEntityType<Light>())
        lights_.append(static_cast<Light*>(entity));

    entity->onAddedToScene();
}

void Scene::removeEntityFromInternalArray(Entity* entity)
{
    if (entity == focusWindow_)
        focusWindow_ = nullptr;

    if (!entities_.erase(entity))
    {
        LOG_ERROR << "Entity is not in this scene: " << *entity;
        return;
    }

    // When an entity is removed from the scene it must be removed from the physics simulation as well
    entity->makeNotPhysical();

    entitiesRequiringUpdate_.eraseValue(entity);
    entity->scene_ = nullptr;

    // Update cached cameras_ and lights_ arrays
    if (entity->isEntityType<Camera>())
        cameras_.eraseValue(static_cast<Camera*>(entity));
    else if (entity->isEntityType<Light>())
        lights_.eraseValue(static_cast<Light*>(entity));

    entity->onRemovedFromScene(this);
}

void Scene::recheckEntityIsPerFrameUpdateRequired(Entity* entity)
{
    if (entity->isPerFrameUpdateRequired())
    {
        if (!entitiesRequiringUpdate_.has(entity))
            entitiesRequiringUpdate_.append(entity);
    }
    else
        entitiesRequiringUpdate_.eraseValue(entity);
}

Scene* Scene::getScene(const String& name)
{
    return allScenes_.detect([&](const Scene* scene) { return scene->getName() == name; }, nullptr);
}

const EffectQueueArray& Scene::getPostProcessEffects() const
{
    if (isPostProcessEffectsDirty_)
    {
        updatePostProcessEffects(postProcessEffects_, postProcessMaterials_);
        isPostProcessEffectsDirty_ = false;
    }

    return postProcessEffects_;
}

void Scene::updatePostProcessEffects(EffectQueueArray& effects, const Vector<String>& postProcessMaterials)
{
    effects.clear();

    auto nextPriority = 0;

    for (auto& postProcessMaterial : postProcessMaterials)
    {
        // Get the material, check that it is a valid post-process material
        auto& material = materials().getMaterial(postProcessMaterial, false);
        if (!material.isLoaded() || !material.getEffect()->getName().startsWith("PostProcess"))
            continue;

        // Update the material
        material.update();

        // Create and setup a new effect queue for this post-process material
        auto queue = effects.create(nextPriority++, material.getEffect());
        material.setupEffectQueue(queue);
    }
}

void Scene::setGlobalPostProcessMaterials(const Vector<String>& materials)
{
    globalPostProcessMaterials_ = materials;
    updateRendererGlobalPostProcessEffects();
}

void Scene::addGlobalPostProcessMaterial(const String& material)
{
    globalPostProcessMaterials_.append(material);
    updateRendererGlobalPostProcessEffects();
}

bool Scene::removeGlobalPostProcessMaterial(const String& material)
{
    if (globalPostProcessMaterials_.eraseValue(material))
    {
        updateRendererGlobalPostProcessEffects();
        return true;
    }

    return false;
}

void Scene::clearGlobalPostProcessMaterials()
{
    globalPostProcessMaterials_.clear();
    updateRendererGlobalPostProcessEffects();
}

void Scene::updateRendererGlobalPostProcessEffects()
{
    updatePostProcessEffects(renderer().getGlobalPostProcessEffects(), getGlobalPostProcessMaterials());
}

bool Scene::setOculusRiftMode(OculusRiftMode mode)
{
    if (mode == OculusRiftDisabled || oculusRiftMode_ == mode)
    {
        oculusRiftMode_ = mode;
        return true;
    }

    if (!platform().isOculusRiftSupported())
    {
        LOG_ERROR << "Oculus Rift is not supported on this platform";
        return false;
    }

    if (!platform().isOculusRiftPresent())
    {
        LOG_ERROR << "There is no Oculus Rift device available";
        return false;
    }

    oculusRiftMode_ = mode;

    return true;
}

}
