/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Math/AABB.h"
#include "CarbonEngine/Geometry/Triangle.h"
#include "CarbonEngine/Platform/TimeValue.h"

namespace Carbon
{

/**
 * Interface for rigid-body physics simulation.
 */
class CARBON_API PhysicsInterface : private Noncopyable
{
public:

    /**
     * Opaque physics body object. Null is reserved for 'no body'.
     */
    typedef void* BodyObject;

    /**
     * Opaque physics body template object. Null is reserved for 'no template'.
     */
    typedef void* BodyTemplateObject;

    /**
     * Opaque physics joint object. Null is reserved for 'no joint'.
     */
    typedef void* JointObject;

    /**
     * Opaque character controller object. Null is reserved for 'no character controller'.
     */
    typedef void* CharacterControllerObject;

    /**
     * The default gravity vector used. Currently this is (0.0, -9.8, 0.0).
     */
    static const Vec3 DefaultGravityVector;

    /**
     * The default world limits: (-5000, -5000, -5000) to (5000, 5000, 5000). See PhysicsInterface::setWorldLimits() for
     * more information.
     */
    static const AABB DefaultWorldLimits;

    /**
     * Enumeration containing the types of forces that can be applied to rigid bodies in the simulation.
     */
    enum ForceMode
    {
        /**
         * A standard force with units of mass * distance / time^2 that is applied on the first substep of timestep.
         */
        ForceStandard,

        /**
         * An impulse with units of mass * distance / time that is applied on the first substep of the next timestep.
         */
        ForceImpulse,

        /**
         * An impulse with units of mass * distance / time that is applied on every substep of the next timestep. Smooth
         * impulses are useful for bodies that are constantly having forces applied to them.
         */
        ForceSmoothImpulse
    };

    virtual ~PhysicsInterface() {}

    /**
     * Returns whether the physics manager implementation is available for use on the current system, for internal use
     * only.
     */
    virtual bool isAvailable() const { return true; }

    /**
     * Sets up the physics manager, for internal use only.
     */
    virtual bool setup() { return true; }

    /**
     * Shuts down the physics manager, for internal use only.
     */
    virtual void shutdown() {}

    /**
     * Sets whether the physics simulation should be updated.
     */
    void setUpdating(bool updating) { isUpdating_ = updating; }

    /**
     * Returns whether the physics simulation is currently being updated.
     */
    bool isUpdating() { return isUpdating_; }

    /**
     * Returns the name of the physics engine being used.
     */
    virtual String getEngineName() const { return "None"; }

    /**
     * Sets the size of the world that the physics simulation will act inside. This area should encompass the whole area
     * where objects are needed to be simulated, once objects are outside this area the simulation is undefined. The
     * default world limits used are given by the DefaultWorldLimits constant.
     */
    virtual void setWorldLimits(const AABB& aabb) {}

    /**
     * Creates and returns a new rigid body. The rigid body is described by an AABB and a mass value in kilograms. If \a
     * fixed is true then the body's mass is ignored and it is fixed statically in the scene. The entity that owns the
     * new rigid body is stored with the rigid body in the physics manager and is used to quickly match a body to an
     * entity. An initial transform for the body can be specified.
     */
    virtual BodyObject createBoundingBoxBody(const AABB& aabb, float mass, bool fixed, const Entity* entity,
                                             const SimpleTransform& initialTransform = SimpleTransform::Identity)
    {
        return nullptr;
    }

    /**
     * Creates and returns a new capsular rigid body. See PhysicsInterface::createBoundingBoxBody() for details on the
     * parameters.
     */
    virtual BodyObject createCapsuleBody(float height, float radius, float mass, bool fixed, const Entity* entity,
                                         const SimpleTransform& initialTransform = SimpleTransform::Identity)
    {
        return nullptr;
    }

    /**
     * Creates and returns a new rigid body template from the given geometry. The template can then be used to create an
     * actual rigid body that uses the template, which assists with reuse of physics data. Returns null on failure.
     */
    virtual BodyTemplateObject createBodyTemplateFromGeometry(const Vector<Vec3>& vertices,
                                                              const Vector<RawIndexedTriangle>& triangles,
                                                              bool deleteOnceUnused = false,
                                                              float customCollisionMargin = 0.0f)
    {
        return nullptr;
    }

    /**
     * Takes a set of geometry and preprocesses it into a binary blob that can then be used to create a rigid body
     * template. The binary blob will load significantly faster than the geometry itself, which allows the geometry
     * processing to be done in an offline step to reduce wait times when loading lots of geometry into the physics
     * layer. Returns success flag.
     */
    virtual bool preProcessGeometry(const Vector<Vec3>& vertices, const Vector<RawIndexedTriangle>& triangles,
                                    Vector<byte_t>& output)
    {
        return false;
    }

    /**
     * Creates and returns a new rigid body template from the given preprocessed geometry. The template can then be used
     * to create an actual rigid body that uses the template, which assists with reuse of physics data. Returns null on
     * failure.
     */
    virtual BodyTemplateObject createBodyTemplateFromPreProcessedGeometry(const Vector<byte_t>& data,
                                                                          bool deleteOnceUnused = false)
    {
        return nullptr;
    }

    /**
     * Creates a rigid body template from the given heightmap data. See
     * PhysicsInterface::createBodyTemplateFromGeometry() for more details.
     */
    virtual BodyTemplateObject createBodyTemplateFromHeightmap(unsigned int heightmapWidth,
                                                               unsigned int heightmapHeight,
                                                               const Vector<float>& heightmap,
                                                               bool deleteOnceUnused = false)
    {
        return nullptr;
    }

    /**
     * Deletes a body template. Body templates can only be deleted when not in use use by any rigid bodies. Returns
     * success flag.
     */
    virtual bool deleteBodyTemplate(BodyTemplateObject bodyTemplateObject) { return false; }

    /**
     * Creates and returns a new rigid body object based off the given template created with
     * PhysicsInterface::createBodyTemplateFromGeometry(). See PhysicsInterface::createBoundingBoxBody() for details on
     * the parameters.
     */
    virtual BodyObject
        createGeometryBodyFromTemplate(BodyTemplateObject bodyTemplateObject, float mass, bool fixed,
                                       const Entity* entity,
                                       const SimpleTransform& initialTransform = SimpleTransform::Identity)
    {
        return nullptr;
    }

    /**
     * Creates and returns a new rigid body based off the given template created with
     * PhysicsInterface::createBodyTemplateFromHeightmap(). See PhysicsInterface::createBoundingBoxBody() for details on
     * the parameters.
     */
    virtual BodyObject
        createHeightmapBodyFromTemplate(BodyTemplateObject bodyTemplateObject, float heightScale, float terrainScale,
                                        float mass, bool fixed, const Entity* entity,
                                        const SimpleTransform& initialTransform = SimpleTransform::Identity)
    {
        return nullptr;
    }

    /**
     * Deletes a rigid body. Bodies with attached joints can not be deleted without first deleting the joints using the
     * PhysicsInterface::deleteJoint() method. Returns success flag.
     */
    virtual bool deleteBody(BodyObject bodyObject) { return false; }

    /**
     * Returns the entity associated with the given physics body. This will be the value of the \a entity parameter
     * passed to the PhysicsInterface::create*Body() method when the body was created.
     */
    virtual const Entity* getBodyEntity(BodyObject bodyObject) const { return nullptr; }

    /**
     * Returns the current transform of a rigid body. This can be used to extract the new rigid body positions If the
     * transform on this body has changed since the last time it was requested then true is returned, otherwise false is
     * returned.
     */
    virtual bool getBodyTransform(BodyObject bodyObject, SimpleTransform& transform) { return false; }

    /**
     * Sets the current transform of a rigid body. Can be used to setup or reposition bodies before or during
     * simulation. Care must be taken to ensure bodies are not set to be intersecting. Returns success flag.
     */
    virtual bool setBodyTransform(BodyObject bodyObject, const SimpleTransform& transform) { return false; }

    /**
     * Returns the world space linear velocity of a rigid body, or a zero vector if the given body is not valid.
     */
    virtual Vec3 getBodyLinearVelocity(BodyObject bodyObject) const { return Vec3::Zero; }

    /**
     * Sets the world space linear velocity of a rigid body. Returns success flag.
     */
    virtual bool setBodyLinearVelocity(BodyObject bodyObject, const Vec3& velocity) const { return false; }

    /**
     * Returns the world space linear velocity of a rigid body, or a zero vector if the given body is not valid.
     */
    virtual Vec3 getBodyAngularVelocity(BodyObject bodyObject) const { return Vec3::Zero; }

    /**
     * Sets the world space angular velocity of a rigid body. Returns success flag.
     */
    virtual bool setBodyAngularVelocity(BodyObject bodyObject, const Vec3& velocity) const { return false; }

    /**
     * Applies a force to a body in the simulation.
     */
    virtual void applyForceToBody(BodyObject bodyObject, const Vec3& force, ForceMode mode) {}

    /**
     * Applies a torque to a body in the simulation.
     */
    virtual void applyTorqueToBody(BodyObject bodyObject, const Vec3& torque, ForceMode mode) {}

    /**
     * Constrains the given body so that it can only ever move in the XY plane, i.e. it's Z position will always be zero
     * and it will only ever rotate around the Z axis (X and Y axis rotations will are not allowed). This method is
     * intended to be used when doing 2D physical simulation. Returns success flag.
     */
    virtual bool constrainBodyToXYPlane(BodyObject bodyObject) { return false; }

    /**
     * Creates a hinge joint connecting two bodies. Returns the id of the new joint or zero if an error occurred.
     */
    virtual JointObject createHingeJoint(BodyObject firstBodyObject, BodyObject secondBodyObject,
                                         const Vec3& globalAnchor, const Vec3& globalAxis)
    {
        return nullptr;
    }

    /**
     * Creates a ball and socket joint connecting two bodies. Returns the id of the new joint or zero if an error
     * occurred.
     */
    virtual JointObject createBallAndSocketJoint(BodyObject firstBodyObject, BodyObject secondBodyObject,
                                                 const Vec3& globalAnchor, const Vec3& angularLimits = Vec3::Zero)
    {
        return nullptr;
    }

    /**
     * Deletes a joint. Returns success flag.
     */
    virtual bool deleteJoint(JointObject jointObject) { return false; }

    /**
     * Puts all the joints attached to the given body into the \a joints vector. Returns success flag.
     */
    virtual bool getBodyJoints(BodyObject bodyObject, Vector<JointObject>& joints) const { return false; }

    /**
     * Creates and returns a new character controller with the given dimensions. The entity that owns the new character
     * controller is stored with it in the physics manager and is used to quickly match a character controller to an
     * entity.
     */
    virtual CharacterControllerObject createCharacterController(float height, float radius, const Entity* entity)
    {
        return nullptr;
    }

    /**
     * Deletes a character controller. Returns success flag.
     */
    virtual bool deleteCharacterController(CharacterControllerObject controllerObject) { return false; }

    /**
     * Returns the current world space position of a character controller, or zero if the given controller is invalid.
     */
    virtual Vec3 getCharacterControllerPosition(CharacterControllerObject controllerObject) const { return Vec3::Zero; }

    /**
     * Sets the current world space position of a character controller. Returns success flag.
     */
    virtual bool setCharacterControllerPosition(CharacterControllerObject controllerObject, const Vec3& position)
    {
        return false;
    }

    /**
     * Moves a character controller through the world along the given movement vector. This will collide the character
     * controller with objects in the world.
     */
    virtual void moveCharacterController(CharacterControllerObject controllerObject, const Vec3& move, float dt) {}

    /**
     * For the specified character controller, this method returns whether or not it has hit a surface directly above
     * it. If it has then the return value will be true and the normal of the hit surface will be returned in \a
     * collisionNormal. This is mainly used to detect when a character controller has jumped into a ceiling or other
     * object in order to terminate the jump.
     */
    virtual bool getCharacterControllerUpAxisCollision(CharacterControllerObject characterControllerObject,
                                                       Vec3& collisionNormal) const
    {
        return false;
    }

    /**
     * For the specified character controller, this method returns whether or not it has hit a surface directly below
     * it. If it has then the return value will be true and the normal of the hit surface will be returned in \a
     * collisionNormal. This is mainly used to detect when a character controller has hit the ground after jumping or
     * falling.
     */
    virtual bool getCharacterControllerDownAxisCollision(CharacterControllerObject characterControllerObject,
                                                         Vec3& collisionNormal) const
    {
        return false;
    }

    /**
     * Returns the length in seconds of the substeps used in PhysicsInterface::update(). This is a constant value chosen
     * by the physics engine. Common values are in the range from 1/60th of a second to 1/100th of a second.
     */
    virtual TimeValue getSubstepSize() const { return {}; }

    /**
     * Advances the physics simulation by the given time amount in seconds. Fixed-length substeps are used to update the
     * simulation robustly. The size of these substeps is given by PhysicsInterface::getSubstepSize(), and partial
     * substeps are accumulated across calls to PhysicsInterface::update().
     */
    virtual void update(TimeValue time) {}

    /**
     * Returns the current gravity vector.
     */
    virtual const Vec3& getGravityVector() const { return Vec3::Zero; }

    /**
     * Sets the current gravity vector.
     */
    virtual void setGravityVector(const Vec3& gravity) {}

    /**
     * Casts a ray into the physics simulation and returns whether it hit a body, details on the intersection are
     * returned in \a result if an intersection was found.
     */
    virtual bool raycast(const Ray& ray, PhysicsIntersectResult& result) const { return false; }

    /**
     * Takes the alpha channel of the passed image and converts to to a single bit-per-pixel image that indicates
     * solid/non-solid which is then analysed to generate a set of 2D polygons that surround solid areas and can be used
     * as the basis for a collision hull. This method is used to convert 2D collision map textures to collidable
     * geometry when doing 2D physics simulation. The returned polygon geometry is normalized into the range 0-1.
     * Returns success flag.
     */
    static bool convertImageAlphaTo2DPolygons(const Image& image, Vector<Vector<Vec2>>& outPolygons,
                                              bool flipHorizontally = false, bool flipVertically = false);

    /**
     * Takes a set of 2D polygons and converts them to 3D geometry usable as collidable geometry. The size in the Z
     * dimension is controlled using \a zScale.
     */
    static void convert2DPolygonsToCollisionGeometry(const Vector<Vector<Vec2>>& polygons, Vector<Vec3>& vertices,
                                                     Vector<RawIndexedTriangle>& triangles, float zScale = 50.0f);

    /**
     * Helper method for use in 2D applications that creates a geometry body from the passed 2D line strip.
     */
    BodyObject createGeometryBodyFrom2DLineStrip(const Vector<Vec2>& points, float mass = 0.0f, bool fixed = true,
                                                 const Entity* entity = nullptr,
                                                 const SimpleTransform& initialTransform = SimpleTransform::Identity);

private:

    bool isUpdating_ = true;
};

CARBON_DECLARE_INTERFACE_REGISTRY(PhysicsInterface);

}
