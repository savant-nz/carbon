/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Math/Interpolate.h"
#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Physics/PhysicsInterface.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Render/Effect.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"
#include "CarbonEngine/Scene/Camera.h"
#include "CarbonEngine/Scene/Entity.h"
#include "CarbonEngine/Scene/EntityController/AlphaFadeEntityController.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/Mesh/Mesh.h"
#include "CarbonEngine/Scene/Mesh/MeshManager.h"
#include "CarbonEngine/Scene/Region.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Scene/SceneEvents.h"

namespace Carbon
{

CARBON_DEFINE_SUBCLASS_REGISTRY(Entity)

const auto EntityVersionInfo = VersionInfo(1, 14);
const auto EntityAttachedMeshVersionInfo = VersionInfo(1, 1);

#ifdef CARBON_DEBUG

// Report leaked entities in debug builds
static auto entityInstances = std::unordered_set<Entity*>();
static void logLeakedEntities()
{
    Globals::increaseLeakedResourceCount(uint(entityInstances.size()));

    for (auto entity : entityInstances)
    {
        LOG_WARNING_WITHOUT_CALLER << "Leaked an entity of type " << entity->getEntityTypeName() << " at " << entity
                                   << ", name: " << entity->getName();
    }
}
CARBON_REGISTER_SHUTDOWN_FUNCTION(logLeakedEntities, 0)

#endif

Entity::Entity() : onDestroyEvent(this)
{
#ifdef CARBON_DEBUG
    entityInstances.insert(this);
#endif

    clear();
}

Entity::~Entity()
{
    onDestruct();
    clear();

#ifdef CARBON_DEBUG
    entityInstances.erase(this);
#endif
}

void Entity::onDestruct()
{
    onDestroyEvent.fire(this);
    onDestroyEvent.clear();

    if (scene_)
    {
        LOG_WARNING_WITHOUT_CALLER << "Entity destructor called while still in scene '" << scene_->getName()
                                   << "', this may result in a crash. Entity details: " << *this;
    }
}

bool Entity::removeFromScene()
{
    return parent_ && parent_->removeChild(this);
}

bool Entity::setAttachmentPoint(const String& name, bool useAttachmentPointOrientation)
{
    attachmentPoint_ = name;
    useAttachmentPointOrientation_ = useAttachmentPointOrientation;

    if (!attachmentPoint_.length())
        return true;

    if (!parent_)
        return false;

    auto unused = SimpleTransform();

    return parent_->getAttachmentPointLocalTransform(attachmentPoint_, unused);
}

bool Entity::getAttachmentPointLocalTransform(const String& name, SimpleTransform& transform) const
{
    transform = SimpleTransform::Identity;

    return name.length() == 0;
}

bool Entity::hasAttachmentPointsStartingWith(const String& prefix) const
{
    auto names = Vector<String>();

    getAttachmentPointNames(names, prefix);

    return !names.empty();
}

Entity::operator UnicodeString() const
{
    auto info = Vector<UnicodeString>();

    info.append(getEntityTypeName());

    if (getName().length())
        info.append("name: '" + getName() + "'");

    if (isInternalEntity())
        info.append("internal: true");

    if (!isVisibleIgnoreAlpha())
        info.append("visible: false");

    if (getAlpha() != 1.0f && getAlpha() != getFinalAlpha())
        info.append(UnicodeString() + "alpha: " + getAlpha());

    if (getFinalAlpha() != 1.0f)
        info.append(UnicodeString() + "final alpha: " + getFinalAlpha());

    info.append(UnicodeString() + "position: " + getWorldPosition());

    if (getWorldOrientation() != Quaternion::Identity)
        info.append(UnicodeString() + "orientation: " + getWorldOrientation());

    if (isShadowCaster())
        info.append(", shadow caster: true");

    info.append(UnicodeString() + "render priority: " + getRenderPriority());

    if (getMeshCount())
    {
        auto meshes = meshes_.map<String>([](const AttachedMesh& m) { return m.name; });

        info.append("meshes: [" + String(meshes, " ") + "]");
    }

    if (getControllerCount())
    {
        auto controllers = controllers_.map<String>(
            [](const EntityController* c) { return SubclassRegistry<EntityController>::getPublicTypeName(c); });

        info.append("controllers: [" + String(controllers, " ") + "]");
    }

    return UnicodeString(info);
}

bool Entity::isVisibleIgnoreAlpha(bool checkParent) const
{
    if (checkParent)
    {
        if (isCachedIsVisibleIgnoreAlphaDirty_)
        {
            cachedIsVisibleIgnoreAlpha_ = (getParent() ? getParent()->isVisibleIgnoreAlpha() : true) && isVisible_;
            isCachedIsVisibleIgnoreAlphaDirty_ = false;
        }

        return cachedIsVisibleIgnoreAlpha_;
    }

    return isVisible_;
}

void Entity::setVisible(bool visible)
{
    if (isVisible_ == visible)
        return;

    isVisible_ = visible;

    invalidateIsVisibleIgnoreAlpha();
}

void Entity::setWorldPosition(const Vec3& p)
{
    setLocalPosition(parent_ ? parent_->worldToLocal(p) : p);
}

void Entity::setWorldOrientation(const Quaternion& q)
{
    setLocalOrientation(parent_ ? parent_->worldToLocal(q) : q);
}

void Entity::setWorldTransform(const SimpleTransform& transform)
{
    setLocalTransform(parent_ ? parent_->worldToLocal(transform) : transform);
}

void Entity::onLocalTransformChanged()
{
    invalidateWorldTransform();
    invalidateParentExtents();

    if (allowPhysicsTransformUpdate_)
    {
        if (rigidBody_)
            physics().setBodyTransform(rigidBody_, getWorldTransform());
        else if (characterController_)
            physics().setCharacterControllerPosition(characterController_,
                                                     getWorldPosition() - Vec3(0.0f, characterControllerOffset_, 0.0f));
    }
}

void Entity::updateWorldTransform() const
{
    if (isWorldTransformDirty_)
    {
        auto parent = getParent();

        // Update this node's absolute position and orientation
        if (parent)
        {
            if (attachmentPoint_.length())
            {
                // Get attachment point transform from the parent
                auto transform = SimpleTransform();
                parent->getAttachmentPointLocalTransform(attachmentPoint_, transform);

                if (!useAttachmentPointOrientation_)
                    transform.setOrientation(Quaternion::Identity);

                worldTransform_ = parent->localToWorld(transform) * localTransform_;
            }
            else
                worldTransform_ = parent->getWorldTransform() * localTransform_;
        }
        else
            worldTransform_ = localTransform_;

        isWorldTransformDirty_ = false;
    }
}

void Entity::rotateAroundPoint(const Vec3& point, const Quaternion& rotation)
{
    setWorldTransform({rotation * (getWorldPosition() - point) + point, getWorldOrientation() * rotation});
}

bool Entity::invalidateWorldTransform(const String& attachmentPoint)
{
    // If this entity does not have an up to date world transform then by definition every entity below it in the scene graph
    // does not have one either. This is because computing any entity's world involves computing world transforms for every one
    // of its parent entities right up to the root entity. Also, because an entity's world space AABB and extents are dependent
    // on its world transform every entity below this one in the scene graph does not have those up to date either.
    // Consequently, if this entity's world transform is already dirty then there is no need to propagate this invalidation, so
    // we can simply swallow it and avoid the overhead.
    if (isWorldTransformDirty_)
        return false;

    // If an attachment point is specified then check that it matches this entity's attachment point
    if (attachmentPoint.length() && attachmentPoint != attachmentPoint_)
        return false;

    isWorldTransformDirty_ = true;
    isWorldAABBDirty_ = true;
    areWorldExtentsDirty_ = true;

    return true;
}

const String& Entity::getEntityTypeName() const
{
    return SubclassRegistry<Entity>::getPublicTypeName(this);
}

void Entity::clear()
{
    setVisible(true);
    setIsInternalEntity(false);

    isLocalAABBDirty_ = true;
    isWorldAABBDirty_ = true;
    areLocalExtentsDirty_ = true;
    areWorldExtentsDirty_ = true;

    setName(String::Empty);

    setLocalTransform(SimpleTransform::Identity);

    parameters_.clear();

    clearMeshes();

    setMaterialRoot(String::Empty);
    setOverrideMaterial(String::Empty);

    setMeshScale(Vec3::One);

    isWorldGeometry_ = false;
    defaultGeometryShadowCasterValue_ = false;

    for (auto controller : controllers_)
        SubclassRegistry<EntityController>::destroy(controller);
    controllers_.clear();

    makeNotPhysical();
    removeCharacterController();
    allowPhysicsTransformUpdate_ = true;

    setAttachmentPoint(String::Empty);

    renderPriority_ = 0;

    setAlpha(1.0f);

    // Clean up material overrides, each parameter is cleared individually to ensure any per-parameter cleanup is done
    for (auto& overrideParameters : materialOverrideParameters_)
    {
        for (const auto& parameter : overrideParameters.params)
        {
            if (!Parameter::isHiddenParameterName(parameter.getName()))
                setMaterialOverrideParameter(overrideParameters.material, parameter.getName(), Parameter::Empty);
        }
    }
    materialOverrideParameters_.clear();

    // Invalidate any cached data relating to this entity
    invalidateWorldTransform();
    invalidateIsVisibleIgnoreAlpha();
    invalidateFinalAlpha();
    invalidateParentExtents();
    recheckIsPerFrameUpdateRequired();
}

void Entity::setDirection(const Vec3& direction)
{
    if (direction.length() < Math::Epsilon)
        return;

    auto d = -direction.normalized();

    auto v0 = Vec3();
    auto v1 = Vec3();
    d.constructBasis(v0, v1);

    setWorldOrientation(Quaternion::createFromRotationMatrix({v0.x, v1.x, d.x, v0.y, v1.y, d.y, v0.z, v1.z, d.z}));
}

bool Entity::isPerFrameUpdateRequired() const
{
    return rigidBody_ || characterController_ || !controllers_.empty();
}

void Entity::recheckIsPerFrameUpdateRequired()
{
    if (getScene())
        getScene()->recheckEntityIsPerFrameUpdateRequired(this);
}

void Entity::update()
{
    // Update all entity controllers
    for (auto i = 0U; i < controllers_.size(); i++)
    {
        if (controllers_[i]->isEnabled())
        {
            if (!controllers_[i]->update(platform().getTimePassed()))
            {
                SubclassRegistry<EntityController>::destroy(controllers_[i]);
                controllers_.erase(i--);
            }
        }
    }

    // Set this entity's transform from the transform of its rigid body
    if (rigidBody_)
    {
        auto transform = SimpleTransform();
        if (physics().getBodyTransform(rigidBody_, transform))
        {
            allowPhysicsTransformUpdate_ = false;
            setWorldTransform(transform);
            allowPhysicsTransformUpdate_ = true;
        }
    }

    // Set this entity's position from the position of its character controller
    if (characterController_)
    {
        allowPhysicsTransformUpdate_ = false;
        setWorldPosition(physics().getCharacterControllerPosition(characterController_) +
                         Vec3(0.0f, characterControllerOffset_, 0.0f));
        allowPhysicsTransformUpdate_ = true;
    }
}

void Entity::intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry)
{
    if (!isVisibleIgnoreAlpha() || meshes_.empty())
        return;

    // Check if the intersection is only targeted for world geometry
    if (onlyWorldGeometry && !isWorldGeometry())
        return;

    // Transform the ray into local space
    auto localRay = getWorldTransform().getInverse() * ray;

    // Check ray against all the meshes
    loadMeshes();
    for (const auto& mesh : meshes_)
    {
        // Transform local space ray into mesh space
        auto meshRay = mesh.transform.getInverse() * localRay;
        meshRay.setOrigin(meshRay.getOrigin() * meshScale_);

        auto meshIntersections = Vector<IntersectionResult>();
        mesh.mesh->intersectRay(meshRay, meshIntersections);

        for (const auto& intersection : meshIntersections)
        {
            auto material = getMaterialRoot() + intersection.getMaterial();

            if (!onlyWorldGeometry || (getScene() && getScene()->isWorldGeometryMaterial(material)))
            {
                auto position = localToWorld(mesh.transform * (intersection.getPoint() * meshScale_));
                auto normal = getWorldOrientation() * mesh.transform.getOrientation() * intersection.getNormal();

                intersections.emplace(ray.getOrigin().distance(position), position, normal, this, material);
            }
        }
    }
}

bool Entity::gatherGeometry(GeometryGather& gather)
{
    if (!isVisible())
        return false;

    if (shouldProcessGather(gather))
    {
        auto hasOverrideMaterial = overrideMaterial_.length() != 0;

        gather.changePriority(getRenderPriority());

        loadMeshes();
        for (const auto& attachedMesh : meshes_)
        {
            if (gather.isShadowGeometryGather() && !attachedMesh.isShadowCaster)
                continue;

            auto& mesh = *attachedMesh.mesh;
            auto meshTransform = getWorldTransform() * attachedMesh.transform;

            auto hasSetMeshTransform = false;

            for (auto& meshComponent : mesh.getMeshComponents())
            {
                auto& geometryChunk = meshComponent.getGeometryChunk();

                // Check the bounding sphere and then the AABB for this mesh component
                if (!gather.getFrustum().intersect(
                        geometryChunk.getSphere().getTransformedAndScaled(meshTransform, meshScale_)) ||
                    !gather.getFrustum().intersect(geometryChunk.getAABB(), meshTransform, meshScale_))
                    continue;

                auto material = materialRoot_ + meshComponent.getMaterial();
                if (hasOverrideMaterial)
                    material = overrideMaterial_;

                auto overrideParameters = getMaterialOverrideParameters(material);

                if (!hasSetMeshTransform)
                {
                    gather.changeTransformation(meshTransform.getPosition(), meshTransform.getOrientation(), meshScale_);
                    hasSetMeshTransform = true;
                }

                gather.changeMaterial(material, overrideParameters);
                gather.addGeometryChunk(geometryChunk);
            }
        }
    }

    return true;
}

void Entity::precache()
{
    if (!overrideMaterial_.length())
    {
        // Load all the meshes and precache all mesh materials
        loadMeshes();
        for (auto& attachedMesh : meshes_)
        {
            for (auto& meshComponent : attachedMesh.mesh->getMeshComponents())
                materials().getMaterial(materialRoot_ + meshComponent.getMaterial()).precache();
        }
    }
    else
    {
        // Precache the override material only
        materials().getMaterial(overrideMaterial_).precache();
    }

    // Make sure the extents are up to date
    getLocalExtents();
    getWorldExtents();
}

void Entity::save(FileWriter& file) const
{
    // Write header
    file.beginVersionedSection(EntityVersionInfo);

    // Save entity data
    file.writeBytes(nullptr, 4);
    file.write(name_, isVisible_, localTransform_.getPosition(), localTransform_.getOrientation(), parameters_);

    // Unused
    file.writeBytes(nullptr, 49);

    // Save parent
    getScene()->saveEntityReference(file, parent_);

    // Save attached meshes (old version for compatibility)
    file.write(meshes_.size());
    for (auto& attachedMesh : meshes_)
    {
        file.writeBytes(nullptr, 2);
        file.write(attachedMesh.name);
    }

    file.write(materialRoot_, overrideMaterial_, defaultGeometryShadowCasterValue_);

    // Unused
    file.writeBytes(nullptr, 4);

    // Save world geometry flag
    file.write(isWorldGeometry_);

    // Unused
    file.writeBytes(nullptr, 4);

    file.write(attachmentPoint_, useAttachmentPointOrientation_, renderPriority_, meshScale_, alpha_);

    // Save material override parameters, cutting out any hidden parameters that start with '.'
    file.write(materialOverrideParameters_.size());
    for (auto& overrideParameters : materialOverrideParameters_)
    {
        file.write(overrideParameters.material);

        auto materialOverrideParameters = ParameterArray();
        for (const auto& parameter : overrideParameters.params)
        {
            if (!Parameter::isHiddenParameterName(parameter.getName()))
                materialOverrideParameters[parameter.getName()] = parameter.getValue();
        }
        file.write(materialOverrideParameters);
    }

    // Save internal flag
    file.write(isInternalEntity());

    // Entity controllers
    file.write(controllers_.size());
    for (auto controller : controllers_)
        file.write(SubclassRegistry<EntityController>::getPublicTypeName(controller), *controller);

    // Attached meshes (new version)
    file.write(meshes_);

    file.endVersionedSection();
}

void Entity::load(FileReader& file)
{
    try
    {
        clear();

        auto readVersion = file.beginVersionedSection(EntityVersionInfo);

        // Read entity data
        file.skip(4);
        auto position = Vec3();
        auto orientation = Quaternion();
        file.read(name_, isVisible_, position, orientation, parameters_);
        localTransform_.setPosition(position);
        localTransform_.setOrientation(orientation);

        // Unused
        file.skip(25);

        // v1.1, unused
        if (readVersion.getMinor() >= 1)
            file.skip(24);

        // v1.2, parent entity
        if (readVersion.getMinor() >= 2)
            parent_ = static_cast<ComplexEntity*>(scene_->loadEntityReference(file));

        // v1.3, attached meshes (old version for compatibility)
        if (readVersion.getMinor() >= 3)
        {
            auto size = 0U;
            file.read(size);
            try
            {
                meshes_.resize(size);
            }
            catch (const std::bad_alloc&)
            {
                throw Exception("Failed reading meshes vector, memory allocation failed");
            }

            for (auto& attachedMesh : meshes_)
            {
                file.skip(1);
                auto isEmbedded = false;
                file.read(isEmbedded);
                if (isEmbedded)
                    throw Exception("Scene file is too old, please re-export");

                file.read(attachedMesh.name);
            }
        }

        // v1.4, material root, material override
        if (readVersion.getMinor() >= 4)
        {
            file.read(materialRoot_, overrideMaterial_);
            file.skip(1);
        }

        // v1.5, unused
        if (readVersion.getMinor() >= 5)
            file.skip(4);

        // v1.6, isWorldGeometry flag
        if (readVersion.getMinor() >= 6)
            file.read(isWorldGeometry_);

        // v1.7, not used
        if (readVersion.getMinor() >= 7)
            file.skip(4);

        // v1.8, attachment point
        if (readVersion.getMinor() >= 8)
            file.read(attachmentPoint_);

        // v1.9, whether to use the attachment point's orientation, and render priority
        if (readVersion.getMinor() >= 9)
            file.read(useAttachmentPointOrientation_, renderPriority_);

        // v1.10, mesh scale as a Vec3 rather than a float
        if (readVersion.getMinor() >= 10)
            file.read(meshScale_);

        // v1.11, alpha and material override parameters
        if (readVersion.getMinor() >= 11)
        {
            auto alpha = 0.0f;
            file.read(alpha);
            setAlpha(alpha);

            auto materialOverrideParametersCount = 0U;
            file.read(materialOverrideParametersCount);
            for (auto i = 0U; i < materialOverrideParametersCount; i++)
            {
                auto material = String();
                file.read(material);

                auto materialOverrideParameters = ParameterArray();
                file.read(materialOverrideParameters);

                // Set each parameter individually so any per-parameter setup takes place
                for (const auto& parameter : materialOverrideParameters)
                    setMaterialOverrideParameter(material, parameter.getName(), parameter.getValue());
            }
        }

        // v1.12, internal flag
        if (readVersion.getMinor() >= 12)
            file.read(isInternalEntity_);

        // v1.13, controllers
        if (readVersion.getMinor() >= 13)
        {
            controllers_.clear();

            auto controllerCount = 0U;
            file.read(controllerCount);

            for (auto i = 0U; i < controllerCount; i++)
            {
                auto controllerType = String();
                file.read(controllerType);

                auto controller = SubclassRegistry<EntityController>::create(controllerType);
                if (!controller)
                    throw Exception() << "Unknown entity controller type: " << controllerType;

                controllers_.append(controller);

                controller->load(file);
                if (!controller->setEntity(this))
                    throw Exception("Failed setting entity on entity controller");
            }
        }

        // v1.14, attached meshes (replaces those stored as of v1.3)
        if (readVersion.getMinor() >= 14)
            file.read(meshes_);

        file.endVersionedSection();
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

bool Entity::addController(EntityController* controller)
{
    if (!controller || !controller->setEntity(this))
        return false;

    controllers_.append(controller);

    recheckIsPerFrameUpdateRequired();

    return true;
}

void Entity::lookAtPoint(const Vec3& p)
{
    auto dir = p - getWorldPosition();

    // Check there is some distance between us and the target point
    if (dir.length() < 0.01f)
        return;

    // Calculate pitch as the angle to tilt up by
    auto pitch = acosf(dir.normalized().dot(Vec3(dir.x, 0.0f, dir.z).normalized())) * -Math::getSign(dir.y);

    // Calculate yaw rotation to point towards the target point
    auto yaw = atan2f(dir.x, -dir.z);

    // Construct rotation
    setWorldOrientation(Quaternion::createRotationXY(pitch, yaw));
}

void Entity::AttachedMesh::save(FileWriter& file) const
{
    file.beginVersionedSection(EntityAttachedMeshVersionInfo);
    file.write(name, transform.getPosition(), transform.getOrientation(), isShadowCaster);
    file.endVersionedSection();
}

void Entity::AttachedMesh::load(FileReader& file)
{
    auto readVersion = file.beginVersionedSection(EntityAttachedMeshVersionInfo);

    auto position = Vec3();
    auto orientation = Quaternion();
    file.read(name, position, orientation);
    transform.setPosition(position);
    transform.setOrientation(orientation);

    // v1.1, shadow caster flag
    if (readVersion.getMinor() >= 1)
        file.read(isShadowCaster);

    file.endVersionedSection();
}

void Entity::attachMesh(const String& name, const SimpleTransform& transform)
{
    meshes_.emplace(name, transform, defaultGeometryShadowCasterValue_);
    onLocalAABBChanged();
}

const String& Entity::getMeshName(unsigned int index) const
{
    return index < meshes_.size() ? meshes_[index].name : String::Empty;
}

const Mesh* Entity::getMesh(unsigned int index) const
{
    return index < meshes_.size() ? meshes_[index].mesh : nullptr;
}

void Entity::clearMeshes()
{
    for (auto& attachedMesh : meshes_)
        meshes().releaseMesh(attachedMesh.mesh);
    meshes_.clear();

    onLocalAABBChanged();
}

void Entity::loadMeshes() const
{
    for (auto& attachedMesh : meshes_)
    {
        if (!attachedMesh.mesh)
            attachedMesh.mesh = meshes().getMesh(attachedMesh.name);
    }
}

bool Entity::isShadowCaster() const
{
    return meshes_.has([](const AttachedMesh& m) { return m.isShadowCaster; });
}

void Entity::setShadowCaster(bool isShadowCaster)
{
    for (auto& attachedMesh : meshes_)
        attachedMesh.isShadowCaster = isShadowCaster;

    defaultGeometryShadowCasterValue_ = isShadowCaster;
}

void Entity::setShadowCaster(const String& meshName, bool isShadowCaster)
{
    for (auto& attachedMesh : meshes_)
    {
        if (attachedMesh.name == meshName)
            attachedMesh.isShadowCaster = isShadowCaster;
    }
}

void Entity::calculateLocalAABB() const
{
    if (meshes_.size())
    {
        localAABB_ = AABB();

        loadMeshes();
        for (auto& attachedMesh : meshes_)
        {
            for (auto& meshComponent : attachedMesh.mesh->getMeshComponents())
                localAABB_.merge(meshComponent.getGeometryChunk().getAABB(), attachedMesh.transform, meshScale_);
        }
    }
    else
        localAABB_ = AABB(Vec3::Zero, Vec3::Zero);

    isLocalAABBDirty_ = false;
}

const AABB& Entity::getLocalAABB() const
{
    if (isLocalAABBDirty_)
        calculateLocalAABB();

    return localAABB_;
}

const AABB& Entity::getWorldAABB() const
{
    if (isWorldAABBDirty_)
    {
        if (getLocalAABB() == AABB())
            worldAABB_ = AABB();
        else
            worldAABB_ = AABB(getLocalAABB(), getWorldTransform());

        isWorldAABBDirty_ = false;
    }

    return worldAABB_;
}

void Entity::alignToWorldGeometry()
{
    setWorldPosition(getWorldPosition() + Vec3(0.0f, -getHeightAboveWorldGeometry(), 0.0f));
}

bool Entity::makePhysical(float mass, bool fixed)
{
    if (hasCharacterController())
    {
        LOG_ERROR << "This entity can't be made physical because it has a character controller";
        return false;
    }

    if (!getScene())
    {
        LOG_ERROR << "This entity can't be made physical because it is not in a scene";
        return false;
    }

    // Delete any existing body for this entity
    makeNotPhysical();

    // Create new rigid body
    rigidBody_ = createInternalRigidBody(mass, fixed);
    if (!rigidBody_)
        return false;

    if (getScene()->is2D() && !fixed)
    {
        if (!physics().constrainBodyToXYPlane(rigidBody_))
            LOG_WARNING << "Unable to constrain this entity to the XY plane, 2D physics will probably not work correctly";
    }

    recheckIsPerFrameUpdateRequired();

    return true;
}

PhysicsInterface::BodyObject Entity::createInternalRigidBody(float mass, bool fixed)
{
    if (getLocalAABB().getVolume() < Math::Epsilon)
    {
        LOG_ERROR << "This entity can't be made physical because it has no volume";
        return nullptr;
    }

    return physics().createBoundingBoxBody(getLocalAABB(), mass, fixed, this, getWorldTransform());
}

void Entity::makeNotPhysical()
{
    // Delete any existing body on this entity
    physics().deleteBody(rigidBody_);
    rigidBody_ = nullptr;

    recheckIsPerFrameUpdateRequired();
}

void Entity::applyWorldForce(const Vec3& force, PhysicsInterface::ForceMode mode)
{
    physics().applyForceToBody(rigidBody_, force, mode);
}

void Entity::applyLocalForce(const Vec3& force, PhysicsInterface::ForceMode mode)
{
    physics().applyForceToBody(rigidBody_, getWorldOrientation() * force, mode);
}

void Entity::applyWorldTorque(const Vec3& torque, PhysicsInterface::ForceMode mode)
{
    physics().applyTorqueToBody(rigidBody_, torque, mode);
}

void Entity::applyLocalTorque(const Vec3& torque, PhysicsInterface::ForceMode mode)
{
    physics().applyTorqueToBody(rigidBody_, getWorldOrientation() * torque, mode);
}

Vec3 Entity::getLinearVelocity() const
{
    return physics().getBodyLinearVelocity(rigidBody_);
}

void Entity::setLinearVelocity(const Vec3& velocity) const
{
    physics().setBodyLinearVelocity(rigidBody_, velocity);
}

Vec3 Entity::getAngularVelocity() const
{
    return physics().getBodyAngularVelocity(rigidBody_);
}

void Entity::setAngularVelocity(const Vec3& velocity) const
{
    physics().setBodyAngularVelocity(rigidBody_, velocity);
}

bool Entity::useCharacterController(float height, float radius, float offset)
{
    if (isPhysical())
    {
        LOG_ERROR << "Character controllers can't be used on physical entities";
        return false;
    }

    if (!getScene())
    {
        LOG_ERROR << "This entity can't use a character controller because it is not in a scene";
        return false;
    }

    if (radius < Math::Epsilon || height < Math::Epsilon)
    {
        LOG_ERROR << "Character controller dimensions invalid: " << radius << ", " << height;
        return false;
    }

    // Erase any existing character controller
    removeCharacterController();

    // Create character controller
    characterController_ = physics().createCharacterController(height, radius, this);
    if (!characterController_)
        return false;

    // Set character controller position
    characterControllerOffset_ = offset;
    physics().setCharacterControllerPosition(characterController_,
                                             getWorldPosition() - Vec3(0.0f, characterControllerOffset_, 0.0f));

    recheckIsPerFrameUpdateRequired();

    return true;
}

void Entity::removeCharacterController()
{
    physics().deleteCharacterController(characterController_);
    characterController_ = nullptr;
    characterControllerOffset_ = 0.0f;

    recheckIsPerFrameUpdateRequired();
}

void Entity::setMeshScale(const Vec3& scale)
{
    if (scale == meshScale_)
        return;

    if (scale.getSmallestComponent() < 0.0f || !scale.isFinite())
    {
        LOG_ERROR << "Invalid mesh scale: " << scale;
        return;
    }

    meshScale_ = scale;
    onLocalAABBChanged();
}

float Entity::getHeightAboveWorldGeometry() const
{
    if (!scene_)
    {
        LOG_ERROR << "This entity is not in a scene";
        return 0.0f;
    }

    // Get world space bounding points
    auto corners = std::array<Vec3, 8>();
    getWorldAABB().getCorners(corners);

    // Get the set of bounding points at the lowest XZ plane
    auto lowestPoints = Vector<Vec3>{corners[0]};
    for (auto i = 1U; i < 8; i++)
    {
        if (fabsf(lowestPoints[0].y - corners[i].y) < Math::Epsilon)
            lowestPoints.append(corners[i]);
        else if (corners[i].y < lowestPoints[0].y)
        {
            lowestPoints.resize(1);
            lowestPoints[0] = corners[i];
        }
    }

    // Average lowest points to find the XZ point where the ray will be cast from
    auto lowestPoint = lowestPoints.getAverage();

    // Cast ray to find the world geometry intersection
    auto result = scene_->intersect(Ray(Vec3(lowestPoint.x, 10000.0f, lowestPoint.z), -Vec3::UnitY), true);

    return result ? lowestPoint.y - result.getPoint().y : 0.0f;
}

void Entity::setAlpha(float alpha)
{
    alpha_ = std::max(alpha, 0.0f);

    invalidateFinalAlpha();
}

void Entity::setAlphaFade(float start, float end, float time)
{
    addController<AlphaFadeEntityController>(start, end, time);
}

float Entity::getFinalAlpha() const
{
    if (cachedFinalAlpha_ == -1.0f)
        cachedFinalAlpha_ = Math::clamp01((getParent() ? getParent()->getFinalAlpha() : 1.0f) * getAlpha());

    return cachedFinalAlpha_;
}

ParameterArray Entity::getMaterialOverrideParameters(const String& material) const
{
    auto parameters = ParameterArray();

    for (auto& overrideParameters : materialOverrideParameters_)
    {
        if (overrideParameters.material == String::Empty || overrideParameters.material == material)
            parameters.merge(overrideParameters.params);
    }

    auto alpha = getFinalAlpha();

    if (alpha != 1.0f)
    {
        // Entity alpha fading works by multiplying the alpha component of the diffuseColor parameter of the entity's materials
        // by this entity's alpha value and forcing blending to be on. Note that this will only work if the material's effect
        // supports the $diffuseColor and $blend parameters.

        // Get diffuse color for the window from the material
        auto diffuseColor = Color();
        if (parameters.has(Parameter::diffuseColor))
            diffuseColor = parameters[Parameter::diffuseColor].getColor();
        else
            diffuseColor = materials().getMaterial(material).getParameters()[Parameter::diffuseColor].getColor();

        diffuseColor.a *= alpha;

        parameters.set(Parameter::diffuseColor, diffuseColor);
        parameters.set(Parameter::blend, true);
    }

    return parameters;
}

void Entity::setMaterialOverrideParameter(const String& material, const String& name, const Parameter& value)
{
    if (!Parameter::isValidParameterName(name))
    {
        LOG_ERROR << "Invalid parameter name: " << name;
        return;
    }

    if (Parameter::isHiddenParameterName(name))
    {
        LOG_ERROR << "Can not set hidden parameters";
        return;
    }

    auto isTextureParameter = Effect::isTextureParameter(name);

    // Find the params to put this new parameter value in
    auto params = pointer_to<ParameterArray>::type();
    for (auto& overrideParameters : materialOverrideParameters_)
    {
        if (overrideParameters.material == material)
        {
            params = &overrideParameters.params;
            break;
        }
    }

    // Create a new entry in materialOverrideParameters_ if needed
    if (!params)
    {
        materialOverrideParameters_.emplace(material);
        params = &materialOverrideParameters_.back().params;
    }

    // Remove this parameter if it is already present
    if (params->remove(name))
    {
        // Release texture if referenced
        if (isTextureParameter)
            textures().releaseTexture((*params)[Parameter::getHiddenParameterName(name)].getPointer<Texture>());
    }

    if (&value == &Parameter::Empty)
        return;

    // Set the new value
    (*params)[name] = value;

    // If this is a texture parameter then take a texture reference, currently 2D WorldDiffuse textures are assumed
    if (isTextureParameter)
    {
        (*params)[Parameter::getHiddenParameterName(name)].setPointer(
            textures().setupTexture(GraphicsInterface::Texture2D, value.getString(), "WorldDiffuse"));
    }
}

bool Entity::shouldProcessGather(const GeometryGather& gather) const
{
    return !gather.isShadowGeometryGather() || isShadowCaster();
}

const AABB& Entity::getLocalExtents() const
{
    if (areLocalExtentsDirty_)
        calculateLocalExtents();

    return localExtents_;
}

const AABB& Entity::getWorldExtents() const
{
    if (areWorldExtentsDirty_)
        calculateWorldExtents();

    return worldExtents_;
}

void Entity::calculateLocalExtents() const
{
    localExtents_ = AABB(getLocalAABB(), getLocalTransform());

    areLocalExtentsDirty_ = false;
}

void Entity::calculateWorldExtents() const
{
    worldExtents_ = getWorldAABB();

    areWorldExtentsDirty_ = false;
}

void Entity::onLocalAABBChanged()
{
    isLocalAABBDirty_ = true;
    isWorldAABBDirty_ = true;

    invalidateParentExtents();
}

void Entity::invalidateParentExtents()
{
    auto e = this;
    while (e)
    {
        e->areLocalExtentsDirty_ = true;
        e->areWorldExtentsDirty_ = true;
        e = e->getParent();
    }
}

bool Entity::intersect(const Entity* entity) const
{
    return getLocalAABB().orientedIntersect(getWorldTransform(), entity->getLocalAABB(), entity->getWorldTransform());
}

}
