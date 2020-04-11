/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventDelegate.h"
#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Core/SubclassRegistry.h"
#include "CarbonEngine/Math/AABB.h"
#include "CarbonEngine/Math/SimpleTransform.h"
#include "CarbonEngine/Physics/PhysicsInterface.h"
#include "CarbonEngine/Scene/IntersectionResult.h"

namespace Carbon
{

/**
 * This is the base class for the scene graph, every type of object in the scene inherits from this class. There are
 * basic provisions for a name, transform, parameters, attaching meshes, and other common functionality. There is no
 * tree hierarchy framework provided by this class, that is the function of ComplexEntity.
 */
class CARBON_API Entity : private Noncopyable
{
public:

    Entity();
    virtual ~Entity();

    /**
     * This event is fired right at the end of the destructor for this entity and can be handled by other entities that
     * hold references to this entity in order to remove any remaining references to it.
     */
    EventDispatcher<Entity, Entity*> onDestroyEvent;

    /**
     * Returns the scene that this entity is in, or null if it is not currently in a scene.
     */
    const Scene* getScene() const { return scene_; }

    /**
     * Returns the scene that this entity is in, or null if it is not currently in a scene.
     */
    Scene* getScene() { return scene_; }

    /**
     * Removes this entity from the scene hierarchy it is currently in by calling ComplexEntity::removeChild() on its
     * parent entity. Returns success flag.
     */
    bool removeFromScene();

    /**
     * Returns this entity's parent entity, or null if this entity has no parent.
     */
    const ComplexEntity* getParent() const { return parent_; }

    /**
     * Returns this entity's parent entity, or null if this entity has no parent.
     */
    ComplexEntity* getParent() { return parent_; }

    /**
     * Returns this entity's parent entity cast to the specified type, null is returned if this entity's parent is not
     * castable to the specified type.
     */
    template <typename ParentType> const ParentType* getParent() const { return dynamic_cast<ParentType*>(parent_); }

    /**
     * Returns this entity's parent entity cast to the specified type, null is returned if this entity's parent is not
     * castable to the specified type.
     */
    template <typename ParentType> ParentType* getParent() { return dynamic_cast<ParentType*>(parent_); }

    /**
     * Sets the name of the attachment point in the parent entity for this entity to base its transforms off. The
     * default (an empty string) means that the parent entity's position and location will be used. Attachment points
     * allow entities to define static or dynamic local space transforms for child entities to inherit from rather than
     * always inheriting off their parent's base transform.
     */
    bool setAttachmentPoint(const String& name, bool useAttachmentPointOrientation = true);

    /**
     * Returns the name of the current attachment point on its parent entity that this entity is using. The default is
     * an empty string which means the parent entity's base transform is used.
     */
    const String& getAttachmentPoint() const { return attachmentPoint_; }

    /**
     * Returns the current local entity space transform of an attachment point. If the attachment point name supplied is
     * not valid or is an empty string then an identity transform will be returned. The return value indicates whether
     * the attachment point name supplied was valid.
     */
    virtual bool getAttachmentPointLocalTransform(const String& name, SimpleTransform& transform) const;

    /**
     * Puts the name of all the attachment points on this entity into the \a names vector. A required prefix can be used
     * to filter the list of attachment points. Note that the \a names vector is not cleared by this method and matching
     * attachment points are just appended to the vector.
     */
    virtual void getAttachmentPointNames(Vector<String>& names, const String& requiredPrefix = String::Empty) const {}

    /**
     * Returns whether this entity has any attachement points starting with the given prefix.
     */
    bool hasAttachmentPointsStartingWith(const String& prefix) const;

    /**
     * Returns the name of this entity.
     */
    const String& getName() const { return name_; }

    /**
     * Sets the name of this entity.
     */
    void setName(const String& name) { name_ = name; }

    /**
     * Returns a short information string on this entity.
     */
    virtual operator UnicodeString() const;

    /**
     * Returns whether this entity is visible. By default this will also check that this entity's parent is also
     * visible, but this check can be skipped if desired. If this entity's parent, or another parent entity further up
     * the tree, is not visible then this entity will not be visible either. Entities with a zero final alpha value will
     * return false from this method, to ignore alpha values use Entity::isVisibleIgnoreAlpha().
     */
    bool isVisible(bool checkParent = true) const
    {
        return isVisibleIgnoreAlpha(checkParent) && getFinalAlpha() != 0.0f;
    }

    /**
     * This method is the same as Entity::isVisible() except that it doesn't look at this entity's final alpha value,
     * this means that entities which have a zero alpha will still be considered visible.
     */
    bool isVisibleIgnoreAlpha(bool checkParent = true) const;

    /**
     * Sets whether this entity is visible.
     */
    void setVisible(bool visible);

    /**
     * Returns this entity's current local space transform.
     */
    const SimpleTransform& getLocalTransform() const { return localTransform_; }

    /**
     * Sets this entity's current local space transform.
     */
    void setLocalTransform(const SimpleTransform& transform)
    {
        localTransform_ = transform;
        onLocalTransformChanged();
    }

    /**
     * Returns the local position of this entity relative to its parent.
     */
    const Vec3& getLocalPosition() const { return localTransform_.getPosition(); }

    /**
     * Sets the local position of this entity relative to its parent, this only sets the X and Y values, the Z value is
     * left unchanged.
     */
    void setLocalPosition(const Vec2& p) { setLocalPosition(p.x, p.y); }

    /**
     * \copydoc setLocalPosition(const Vec2 &)
     */
    void setLocalPosition(float x, float y) { setLocalPosition({x, y, getLocalPosition().z}); }

    /**
     * Sets the local position of this entity relative to its parent.
     */
    void setLocalPosition(const Vec3& p)
    {
        localTransform_.setPosition(p);
        onLocalTransformChanged();
    }

    /**
     * Returns the local orientation of this entity relative to its parent.
     */
    const Quaternion& getLocalOrientation() const { return localTransform_.getOrientation(); }

    /**
     * Sets the local orientation of this entity relative to its parent.
     */
    void setLocalOrientation(const Quaternion& orientation)
    {
        localTransform_.setOrientation(orientation);
        onLocalTransformChanged();
    }

    /**
     * Transforms this entity by the passed transform.
     */
    void transform(const SimpleTransform& transform) { setLocalTransform(localTransform_ * transform); }

    /**
     * Moves this entity by the given offset amount.
     */
    void move(const Vec3& v) { setLocalPosition(getLocalPosition() + v); }

    /**
     * Rotates this entity by the given quaternion.
     */
    void rotate(const Quaternion& rotation) { setLocalOrientation(getLocalOrientation() * rotation); }

    /**
     * Rotates this entity around the X axis by the specified angle.
     */
    void rotateAroundX(float radians) { rotate(Quaternion::createRotationX(radians)); }

    /**
     * Rotates this entity around the Y axis by the specified angle.
     */
    void rotateAroundY(float radians) { rotate(Quaternion::createRotationY(radians)); }

    /**
     * Rotates this entity around the Z axis by the specified angle.
     */
    void rotateAroundZ(float radians) { rotate(Quaternion::createRotationZ(radians)); }

    /**
     * Rotates this entity around the specified axis by the given angle.
     */
    void rotateAxisAngle(const Vec3& axis, float radians) { rotate(Quaternion::createFromAxisAngle(axis, radians)); }

    /**
     * Rotates this entity around a point in world space.
     */
    void rotateAroundPoint(const Vec3& point, const Quaternion& rotation);

    /**
     * Returns this entity's current world space transform.
     */
    const SimpleTransform& getWorldTransform() const
    {
        updateWorldTransform();
        return worldTransform_;
    }

    /**
     * Sets this entity's current world space transform.
     */
    void setWorldTransform(const SimpleTransform& transform);

    /**
     * Returns the world position of this entity.
     */
    const Vec3& getWorldPosition() const
    {
        updateWorldTransform();
        return worldTransform_.getPosition();
    }

    /**
     * Sets the world position of this entity, this only sets the X and Y values, the Z value is left unchanged.
     */
    void setWorldPosition(const Vec2& p) { setWorldPosition(p.x, p.y); }

    /**
     * \copydoc setWorldPosition(const Vec2 &)
     */
    void setWorldPosition(float x, float y) { setWorldPosition({x, y, getWorldPosition().z}); }

    /**
     * Sets the world position of this entity. This transforms the specified world space point into a local entity space
     * point and then calls Entity::setLocalPosition() with the result.
     */
    void setWorldPosition(const Vec3& p);

    /**
     * Returns the world orientation of this entity.
     */
    const Quaternion& getWorldOrientation() const
    {
        updateWorldTransform();
        return worldTransform_.getOrientation();
    }

    /**
     * Sets the world orientation of this entity. This does a conversion from the world space orientation to a local
     * entity space orientation and then calls Entity::setLocalOrientation() with the new quaternion.
     */
    void setWorldOrientation(const Quaternion& q);

    /**
     * Calling this invalidates any cached world transforms on this entity and anything below it in the scene graph. If
     * the attachment point given is an empty string then the invalidation will be processed, alternatively if the
     * attachment point is not an empty string then the invalidation will only be processed if the attchment point is
     * the same as this entity's attachment point (as returned by Entity::getAttachmentPoint()). The return value
     * indicates whether the invalidation was accepted by this entity. This method does not generally need to be called
     * by applications because correct world transform invalidation is handled automatically.
     */
    virtual bool invalidateWorldTransform(const String& attachmentPoint = String::Empty);

    /**
     * Calling this invalidates any cached results of Entity::isVisibleIgnoreAlpha() which is essentially just a binary
     * AND of the individual visibility settings of this entity and its parents all the way up to the root of the scene
     * graph. This method does not generally need to be called by applications because correct invalidation is handled
     * automatically.
     */
    virtual void invalidateIsVisibleIgnoreAlpha() { isCachedIsVisibleIgnoreAlphaDirty_ = true; }

    /**
     * Calling this invalidates any cached results of Entity::getFinalAlpha() which is essentially just a multiplication
     * of the individual alpha values of this entity and its parents all the way up to the root of the scene graph. This
     * method does not generally need to be called by applications because correct invalidation is handled
     * automatically.
     */
    virtual void invalidateFinalAlpha() { cachedFinalAlpha_ = -1.0f; }

    /**
     * Converts a transform in local entity space to a transform in world space.
     */
    SimpleTransform localToWorld(const SimpleTransform& t) const { return getWorldTransform() * t; }

    /**
     * Converts a point in local entity space to a point in world space.
     */
    Vec3 localToWorld(const Vec3& p) const { return getWorldTransform() * p; }

    /**
     * Converts a rotation in local entity space to a rotation in world space.
     */
    Quaternion localToWorld(const Quaternion& q) const { return getWorldOrientation() * q; }

    /**
     * Converts a point in world space to a point in local entity space.
     */
    Vec3 worldToLocal(const Vec3& p) const { return getWorldTransform().getInverse() * p; }

    /**
     * Converts a rotation in world space to a rotation in local entity space.
     */
    Quaternion worldToLocal(const Quaternion& q) const { return getWorldOrientation().getInverse() * q; }

    /**
     * Converts a transform in world space to a transform in local entity space.
     */
    SimpleTransform worldToLocal(const SimpleTransform& t) const { return getWorldTransform().getInverse() * t; }

    /**
     * Returns the direction of this entity, this is particularly relevant for entities such as cameras, directional
     * lights and spot lights. The direction is the same as this entity's negative world space Z axis.
     */
    Vec3 getDirection() const { return getWorldTransform().getDirection(); }

    /**
     * Sets this entity's direction, which is the same as setting its world-space Z axis. See Entity::getDirection() for
     * details.
     */
    void setDirection(const Vec3& direction);

    /**
     * Returns whether this entity currently requires its Entity::update() method to be called automatically every
     * frame. Entities that don't require this should return false to avoid the overhead of calling Entity::update()
     * every frame. When something on an entity changes that could affect the return value of this method the entity
     * must call Entity::recheckIsPerFrameUpdateRequired() so that the scene's cached list of the entities that require
     * a per-frame update can be kept up to date. Keeping this list avoids having to check the return value from this
     * method on every entity every frame.
     */
    virtual bool isPerFrameUpdateRequired() const;

    /**
     * Entities must call this method when something changes that could affect the return value of the
     * Entity::isPerFrameUpdateRequired() method. See Entity::isPerFrameUpdateRequired() for details.
     */
    void recheckIsPerFrameUpdateRequired();

    /**
     * This update method is called automatically every frame on entities that return true from
     * Entity::isPerFrameUpdateRequired() and can be used by entities to run per-frame update code when needed. See
     * Entity::isPerFrameUpdateRequired() for details.
     */
    virtual void update();

    /**
     * Returns whether this entity has a value for the given parameter set.
     */
    template <typename LookupType> bool hasParameter(const LookupType& lookup) const { return parameters_.has(lookup); }

    /**
     * Returns the value of the given parameter, if there is no parameter with the given name then Parameter::Empty is
     * returned.
     */
    template <typename LookupType> const Parameter& getParameter(const LookupType& lookup) const
    {
        return parameters_[lookup];
    }

    /**
     * Sets the value of the given parameter.
     */
    template <typename LookupType> void setParameter(const LookupType& lookup, const Parameter& value)
    {
        parameters_.set(lookup, value);
    }

    /**
     * Removes the given parameter from this entity. Returns success flag.
     */
    template <typename LookupType> bool removeParameter(const LookupType& lookup) { return parameters_.remove(lookup); }

    /**
     * Removes all the parameters stored on this entity.
     */
    void clearParameters() { parameters_.clear(); }

    /**
     * Returns the array of parameters stored on this entity.
     */
    const ParameterArray& getParameters() const { return parameters_; }

    /**
     * Returns the type of this entity as a String. This is a helper method that just passes 'this' to
     * SubclassRegistry<Entity>::getEntityTypeName() and returns the result.
     */
    const String& getEntityTypeName() const;

    /**
     * Returns whether this entity is able to be cast to the given entity type, the test is implemented using
     * `dynamic_cast`.
     */
    template <typename EntityType> bool isEntityType() const { return asEntityType<EntityType>() != nullptr; }

    /**
     * Attempts to cast this entity to the specified entity type, null is returned if the cast fails. This method uses
     * `dynamic_cast`.
     */
    template <typename EntityType> const EntityType* asEntityType() const
    {
        return dynamic_cast<const EntityType*>(this);
    }

    /**
     * Attempts to cast this entity to the specified entity type, null is returned if the cast fails. This method uses
     * `dynamic_cast`.
     */
    template <typename EntityType> EntityType* asEntityType() { return dynamic_cast<EntityType*>(this); }

    /**
     * Clears this entity's data. Should be extended by subclasses of Entity to include their cleanup code as well. This
     * does not alter this entity's status in the scene it is currently in, if it's in one, it only clears the entity
     * local data at this node.
     */
    virtual void clear();

    /**
     * Gathers visible geometry from this entity and all its children.
     */
    virtual bool gatherGeometry(GeometryGather& gather);

    /**
     * Intersects the passed world space ray with this entity and all its children.
     */
    virtual void intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry);

    /**
     * Tells this entity and all entities below it in the scene graph to precache any resources that they require for
     * rendering so that just-in-time loading does not occur during use or rendering, this includes resources such as
     * meshes, materials, textures, and so on.
     */
    virtual void precache();

    /**
     * Saves this entity to a file stream. Subclasses of Entity which need to be able to be serialized must implement
     * their own saving method that builds on top of this. Throws an Exception if an error occurs.
     */
    virtual void save(FileWriter& file) const;

    /**
     * Loads this entity from a file stream. Subclasses of Entity which need to be able to be serialized must implement
     * their own loading method that builds on top of this. Throws an Exception if an error occurs.
     */
    virtual void load(FileReader& file);

    /**
     * Rotates this entity so that its world space negative Z axis is pointing towards the given point.
     */
    virtual void lookAtPoint(const Vec3& p);

    /**
     * Rotates this entity so it is facing the given entity's world position.
     */
    void lookAtEntity(const Entity* entity) { lookAtPoint(entity->getWorldPosition()); }

    /**
     * Returns the current material root for this entity. The material root string is prefixed to all materials used by
     * this entity prior to searching for the material, and can be used to help organize materials for different
     * entities into separate directories. Multiple directories may be included in the material root using forward slash
     * characters as a separator. The material root defaults to an empty string.
     */
    const String& getMaterialRoot() const { return materialRoot_; }

    /**
     * Sets the material root for this entity. See Entity::getMaterialRoot() for details.
     */
    void setMaterialRoot(const String& root) { materialRoot_ = root; }

    /**
     * Returns the current override material if one is set. When an override material is set, all the meshes attached to
     * this entity are rendered using that material. If the override material is not set set, i.e. it's an empty string,
     * then the mesh materials are used.
     */
    const String& getOverrideMaterial() const { return overrideMaterial_; }

    /**
     * Sets the material override for this mesh. See Entity::getMaterialOverride() for details.
     */
    void setOverrideMaterial(const String& material) { overrideMaterial_ = material; }

    /**
     * Attaches a mesh to this entity and returns its index.
     */
    void attachMesh(const String& name, const SimpleTransform& transform = SimpleTransform::Identity);

    /**
     * Returns the number of meshes attached to this entity.
     */
    unsigned int getMeshCount() const { return meshes_.size(); }

    /**
     * Returns the name of the mesh at the given index, or zero if the index is invalid.
     */
    const String& getMeshName(unsigned int index) const;

    /**
     * Returns the mesh at the given index, or null if the given index is invalid.
     */
    const Mesh* getMesh(unsigned int index) const;

    /**
     * Removes all meshes attached to this entity.
     */
    void clearMeshes();

    /**
     * Returns whether this entity is a container for static world geometry. This flag is used to sort out world
     * geometry entities from other static or dynamic scene entities.
     */
    bool isWorldGeometry() const { return isWorldGeometry_; }

    /**
     * Sets whether this entity is a container for static world geometry. See Entity::isWorldGeometry() for details.
     */
    void setIsWorldGeometry(bool value) { isWorldGeometry_ = value; }

    /**
     * Returns whether there is any geometry on this entity that is set to be a shadow caster.
     */
    virtual bool isShadowCaster() const;

    /**
     * Sets the shadow caster flag for all geometry currently on this entity, and all geometry subsequently attached
     * will have its initial shadow caster flag set to the passed value.
     */
    virtual void setShadowCaster(bool isShadowCaster);

    /**
     * Sets the shadow caster flag on all meshes with the given name to the specified value.
     */
    virtual void setShadowCaster(const String& meshName, bool isShadowCaster);

    /**
     * Returns this entity's local space bounding box around all its meshes.
     */
    const AABB& getLocalAABB() const;

    /**
     * Returns a world space bounding box around this entity's meshes.
     */
    const AABB& getWorldAABB() const;

    /**
     * Convenience method that uses the value returned by Entity::getHeightAboveWorldGeometry() to move this entity up
     * or down onto world geometry at its current x,z position. If there is no world geometry at the current x,z
     * position then this method does nothing.
     */
    void alignToWorldGeometry();

    /**
     * Puts this entity into the physics simulation as a rigid body, For this method to work this entity must be in a
     * scene, if this entity has not been added to a scene then false will be returned. The rigid body used is the
     * bounding box around this entity's meshes at the time this method is called. If \a fixed is true then the rigid
     * body for this entity will be immovable, that is, it will be a fixed object in the physical simulation. Returns
     * false if there are no meshes attached to this entity.
     */
    virtual bool makePhysical(float mass, bool fixed = false);

    /**
     * Returns whether this entity is currently in the physics simulation.
     */
    virtual bool isPhysical() const { return rigidBody_ != nullptr; }

    /**
     * Removes this entity from the physics simulation if it has been added to it using Entity::makePhysical().
     */
    virtual void makeNotPhysical();

    /**
     * Returns this entity's rigid body, or null if it is not currently physical.
     */
    PhysicsInterface::BodyObject getRigidBody() const { return rigidBody_; }

    /**
     * Applies a world space force to the rigid body of this entity if it is in a physics simulation. See
     * Entity::makePhysical() for more information.
     */
    void applyWorldForce(const Vec3& force, PhysicsInterface::ForceMode mode = PhysicsInterface::ForceStandard);

    /**
     * Applies a local entity space force to the rigid body of this entity if it is in a physics simulation. See
     * Entity::makePhysical() for more information.
     */
    void applyLocalForce(const Vec3& force, PhysicsInterface::ForceMode mode = PhysicsInterface::ForceStandard);

    /**
     * Applies a world space torque to the rigid body of this entity if it is in a physics simulation. See
     * Entity::makePhysical() for more information.
     */
    void applyWorldTorque(const Vec3& torque, PhysicsInterface::ForceMode mode = PhysicsInterface::ForceStandard);

    /**
     * Applies a local entity space torque to the rigid body of this entity if it is in a physics simulation. See
     * Entity::makePhysical() for more information.
     */
    void applyLocalTorque(const Vec3& torque, PhysicsInterface::ForceMode mode = PhysicsInterface::ForceStandard);

    /**
     * Returns the world space linear velocity of this entity if it is in a physics simulation, otherwise a zero vector
     * is returned. See Entity::makePhysical() for more information.
     */
    Vec3 getLinearVelocity() const;

    /**
     * Sets the world space linear velocity of this entity if it is in a physics simulation. See Entity::makePhysical()
     * for more information.
     */
    void setLinearVelocity(const Vec3& velocity) const;

    /**
     * Returns the world space angular velocity of this entity if it is in a physics simulation, otherwise a zero vector
     * is returned. See Entity::makePhysical() for more information.
     */
    Vec3 getAngularVelocity() const;

    /**
     * Sets the world space angular velocity of this entity if it is in a physics simulation. See Entity::makePhysical()
     * for more information.
     */
    void setAngularVelocity(const Vec3& velocity) const;

    /**
     * Makes this entity use a character controller to interact with the scene's physical simulation. The \a radius and
     * \a height values specify the dimensions of the capsule, and the \a offset value specifies the vertical offset of
     * the origin from the center of the capsule. The capsule is used to bound the entity and for scene interactions,
     * and will always remain vertical regardless of this entity's orientation. Returns success flag.
     */
    virtual bool useCharacterController(float height, float radius, float offset = 0.0f);

    /**
     * Removes the character controller from this entity if one is being used.
     */
    void removeCharacterController();

    /**
     * Returns whether this entity is currently using a character controller.
     */
    bool hasCharacterController() const { return characterController_ != nullptr; }

    /**
     * Returns the scale factors used for the meshes attached to this entity. Defaults to 1.0 for all components.
     */
    const Vec3& getMeshScale() const { return meshScale_; }

    /**
     * Sets the mesh scale factors to use when drawing the meshes attached to this entity.
     */
    void setMeshScale(const Vec3& scale);

    /**
     * \copydoc Entity::setMeshScale(const Vec3&).
     */
    void setMeshScale(float scale) { setMeshScale({scale, scale, scale}); }

    /**
     * Returns the vertical offset from the bottom-most point of this entity's world space bounding box to the highest
     * world geometry point at this entity's x,z location. The return value may be negative. If there is no world
     * geometry above or below this entity or this entity is not part of a scene then zero is returned.
     */
    float getHeightAboveWorldGeometry() const;

    /**
     * Returns the render priority of this entity, this affects the order in which entities are rendered. Entities with
     * lower priorities get drawn first, and entities with higher priorities are drawn last and so will appear on top of
     * any entities that have a lower priority. This is most useful in order to control the render order of entities
     * that have blending turned on, or in GUIs to make sure the render order is sensible.
     */
    virtual int getRenderPriority() const { return renderPriority_; }

    /**
     * Sets the render priority of this entity. See Entity::getRenderPriority() for details.
     */
    virtual void setRenderPriority(int priority) { renderPriority_ = priority; }

    /**
     * Returns the current alpha fade value on this entity, will be greater than or equal to zero.
     */
    float getAlpha() const { return alpha_; }

    /**
     * Sets the current alpha fade value for this entity, must be greater than or equal to zero.
     */
    void setAlpha(float alpha);

    /**
     * Sets a linear alpha fade on this entity over the given time period. This can be used to fade entities in and out.
     * The time is measured in seconds. Internally this just adds an AlphaFadeEntityController to this entity.
     */
    void setAlphaFade(float start, float end, float time);

    /**
     * Sets a linear alpha fade on this entity over the given time period. This is the same as Entity::setAlphaFade()
     * except the current alpha is used as the initial alpha so only the target alpha value is specified. The time is
     * measured in seconds.
     */
    void setAlphaFade(float targetAlpha, float transitionTime)
    {
        setAlphaFade(getAlpha(), targetAlpha, transitionTime);
    }

    /**
     * Returns the final alpha to use when rendering this entity. This is the product of this entity's alpha that is
     * returned by Entity::getAlpha() and the final alpha of this entity's parent, and is clamped to the range 0 - 1.
     */
    float getFinalAlpha() const;

    /**
     * Overrides a single parameter value in all the materials that are used by this entity. The underlying materials
     * are not altered, but when this entity is rendered it will use override values in place of the value specified in
     * the material file. To remove a material override parameter, set it to Parameter::Empty.
     */
    void setMaterialOverrideParameter(const String& name, const Parameter& value)
    {
        setMaterialOverrideParameter(String::Empty, name, value);
    }

    /**
     * Overrides a single parameter value in the specified material that is used by this entity. The underlying material
     * is not altered, but when this entity is rendered it will use the override value in place of the actual value
     * specified in the material file. To remove a material override parameter, set it to Parameter::Empty.
     */
    void setMaterialOverrideParameter(const String& material, const String& name, const Parameter& value);

    /**
     * Returns an AABB in the space defined by this entity's parent that encloses this entity and everything below it in
     * the scene graph.
     */
    const AABB& getLocalExtents() const;

    /**
     * Returns a world space AABB that encloses this entity and everything below it in the scene graph.
     */
    const AABB& getWorldExtents() const;

    /**
     * Returns whether this entity intersects with the given entity, the default implementation of this method only uses
     * AABBs to detect intersection, and subclasses can extend this to do more accurate intersection testing.
     */
    virtual bool intersect(const Entity* entity) const;

    /**
     * Returns whether or not this entity was created by the engine for internal use, entities that are flagged as
     * internal are excluded by default from the entity lists that are returned by methods such as
     * ComplexEntity::getChildren().
     */
    bool isInternalEntity() const { return isInternalEntity_; }

    /**
     * Sets whether or not this entity is flagged as having been created by the engine for internal use, entities that
     * are flagged as internal are excluded by default from the entity lists that are returned by methods such as
     * ComplexEntity::getChildren().
     */
    void setIsInternalEntity(bool value) { isInternalEntity_ = value; }

    /**
     * Returns the number of entity controllers currently active on this entity.
     */
    unsigned int getControllerCount() const { return controllers_.size(); }

    /**
     * Returns this entity's first entity controller of the specified type, or null if one is not found.
     */
    template <typename EntityControllerType> EntityControllerType* getController()
    {
        for (auto controller : controllers_)
        {
            auto candidate = dynamic_cast<EntityControllerType*>(controller);

            if (candidate)
                return candidate;
        }

        return nullptr;
    }

    /**
     * Adds a new entity controller of the specified type to this entity and returns a pointer to the new controller.
     * The new controller's `initialize()` method is called with the passed arguments if any are given. If the specified
     * entity controller type is not known, or the specified controller type can't be used on this entity, then null is
     * returned.
     */
    template <typename EntityControllerType, typename... ArgTypes>
    EntityControllerType* addController(ArgTypes&&... args)
    {
        auto controller = SubclassRegistry<EntityController>::create<EntityControllerType>();
        if (!controller)
        {
            LOG_ERROR << "Failed creating entity controller of type '" << typeid(EntityControllerType).name() << "'";
            return nullptr;
        }

        if (!addController(controller))
        {
            SubclassRegistry<EntityController>::destroy(controller);
            return nullptr;
        }

        initializeIfArgsPassed(controller, std::forward<ArgTypes>(args)...);

        return controller;
    }

protected:

    /**
     * An AABB in local entity space that surrounds everything that is part of this this entity (e.g. meshes). There is
     * an internal dirty state that indicates whether this AABB needs to be recalculated and the Entity::getLocalAABB()
     * method will automatically call Entity::calculateLocalAABB() if this dirty state is set. Entity subclasses that
     * add extra drawing or visible objects not known to the base Entity class must extend Entity::calculateLocalAABB()
     * to include their extra content otherwise there may be culling errors. They must also call
     * Entity::onLocalAABBChanged() when there is any change to the entity's local bounding volume, this will cause this
     * entity's local and world space AABBs to be flagged as dirty and will also propagate other necessary dirty flags
     * through the scene graph.
     */
    mutable AABB localAABB_;

    /**
     * Recalculates localAABB_ based on the content at this entity, this must be extended by subclasses if they add
     * custom drawing or additional visible objects. Note that this AABB only encloses the content at this entity and
     * does not know about boundings around any child entities that may be beneath it in the entity hierarchy.
     */
    virtual void calculateLocalAABB() const;

    /**
     * This method must be called when the local space AABB around this entity is changed. This does not cause the
     * bounding volume to be immediately recalculated but will set the appropriate dirty flags in the scene graph.
     * Affected bounding volumes and extents will then be recalculated as they are needed.
     */
    void onLocalAABBChanged();

    /**
     * This method should be called right at the start of the destructor of all entity subclasses in order to correctly
     * fire onDestroyEvent and warn if the entity is currently in a scene.
     */
    void onDestruct();

    /**
     * This method is called when this entity is added to a scene, the default implementation does nothing but entity
     * subclasses can override this to add their own functionality.
     */
    virtual void onAddedToScene() {}

    /**
     * This method is called when this entity has been removed from the scene it was in, the default implementation does
     * nothing but entity subclasses can override this to add their own functionality.
     */
    virtual void onRemovedFromScene(Scene* scene) {}

    /**
     * When an entity is made physical this method is called to creates the entity's underlying rigid body. The default
     * implementation creates a bounding box body based on this entity's AABB, and subclasses can override this method
     * to customize an entity's rigid body setup.
     */
    virtual PhysicsInterface::BodyObject createInternalRigidBody(float mass, bool fixed);

    /**
     * Returns all parameters of the passed material that should be overridden when rendering this entity. This will
     * include any parameters set using Entity::setMaterialOverrideParameter(), as well as any parameters needed to
     * implement an alpha fade if one is present on this entity.
     */
    ParameterArray getMaterialOverrideParameters(const String& material) const;

    /**
     * Returns whether or not this entity should process the passed GeometryGather, currently this only takes into
     * acount whether this entity is flagged as being a shadow caster when the gather is only for shadow casting
     * geometry.
     */
    bool shouldProcessGather(const GeometryGather& gather) const;

    /**
     * Returns the initial value to use for the shadow caster flag on any geometry that gets attached to this entity.
     */
    bool getDefaultGeometryShadowCasterValue() const { return defaultGeometryShadowCasterValue_; }

private:

    friend class Scene;
    friend class ComplexEntity;

    Scene* scene_ = nullptr;

    ComplexEntity* parent_ = nullptr;
    String name_;

    bool isInternalEntity_ = false;

    bool isVisible_ = true;
    mutable bool cachedIsVisibleIgnoreAlpha_ = false;
    mutable bool isCachedIsVisibleIgnoreAlphaDirty_ = false;

    // Entity transform in local parent space
    SimpleTransform localTransform_;
    void onLocalTransformChanged();

    // Entity transform in world space
    mutable SimpleTransform worldTransform_;
    mutable bool isWorldTransformDirty_ = true;
    void updateWorldTransform() const;

    void invalidateParentExtents();

    mutable bool isLocalAABBDirty_ = true;

    // World space AABB based on the local AABB, this is recalculated JIT as required
    mutable AABB worldAABB_;
    mutable bool isWorldAABBDirty_ = true;

    // Extents in parent space and in world space, these are recalculated JIT as required
    mutable AABB localExtents_;
    mutable AABB worldExtents_;
    mutable bool areLocalExtentsDirty_ = true;
    mutable bool areWorldExtentsDirty_ = true;
    virtual void calculateLocalExtents() const;
    virtual void calculateWorldExtents() const;

    // Parameters
    ParameterArray parameters_;

    // Entity controllers
    Vector<EntityController*> controllers_;
    bool addController(EntityController* controller);

    // Meshes
    struct AttachedMesh
    {
        String name;

        // The local transform of the mesh in entity space
        SimpleTransform transform;

        // Whether this mesh should cast shadows
        bool isShadowCaster = false;

        // The mesh object returned by the MeshManager class, will be null if the mesh is yet to be loaded
        mutable const Mesh* mesh = nullptr;

        AttachedMesh() {}
        AttachedMesh(String meshName, const SimpleTransform& transform_, bool isShadowCaster_)
            : name(std::move(meshName)), transform(transform_), isShadowCaster(isShadowCaster_)
        {
        }

        void save(FileWriter& file) const;
        void load(FileReader& file);
    };
    Vector<AttachedMesh> meshes_;
    void loadMeshes() const;

    String materialRoot_;
    String overrideMaterial_;

    Vec3 meshScale_;

    bool isWorldGeometry_ = false;
    bool defaultGeometryShadowCasterValue_ = false;

    // Attachment point
    String attachmentPoint_;
    bool useAttachmentPointOrientation_ = true;

    int renderPriority_ = 0;

    float alpha_ = 1.0f;
    mutable float cachedFinalAlpha_ = -1.0f;

    struct MaterialOverrideParameters
    {
        String material;
        ParameterArray params;

        MaterialOverrideParameters() {}
        MaterialOverrideParameters(String material_) : material(std::move(material_)) {}
    };
    Vector<MaterialOverrideParameters> materialOverrideParameters_;

    // Physical rigid-body simulation
    PhysicsInterface::BodyObject rigidBody_ = nullptr;
    bool allowPhysicsTransformUpdate_ = true;

    // Character controller
    PhysicsInterface::CharacterControllerObject characterController_ = nullptr;
    float characterControllerOffset_ = 0.0f;
    friend class PlatformerEntityController;
    friend class PlayerEntityController;

    friend class SubclassRegistry<Entity>;
    bool wasCreatedThroughSubclassRegistry_ = false;
};

CARBON_DECLARE_SUBCLASS_REGISTRY(Entity);

/**
 * \file
 */

/**
 * This macro must be put in the primary source file for every entity subclass in order to register the subclass type.
 */
#define CARBON_REGISTER_ENTITY_SUBCLASS(EntitySubclassType) CARBON_REGISTER_SUBCLASS(EntitySubclassType, Carbon::Entity)

}
