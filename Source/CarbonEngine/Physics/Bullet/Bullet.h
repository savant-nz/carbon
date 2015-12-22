/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_BULLET

#include "CarbonEngine/Physics/Bullet/BulletIncludeWrapper.h"
#include "CarbonEngine/Physics/PhysicsInterface.h"
#include "CarbonEngine/Physics/PhysicsIntersectResult.h"

#ifdef _MSC_VER
    #pragma comment(lib, "Bullet" CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX)
#endif

namespace Carbon
{

class KinematicCharacterController;

/**
 * Bullet physics backend.
 */
class Bullet : public PhysicsInterface
{
public:

    ~Bullet() override { shutdown(); }

    bool isAvailable() const override { return true; }
    bool setup() override;
    void shutdown() override;
    String getEngineName() const override;
    BodyObject createBoundingBoxBody(const AABB& aabb, float mass, bool fixed, const Entity* entity,
                                     const SimpleTransform& initialTransform) override;
    BodyObject createCapsuleBody(float height, float radius, float mass, bool fixed, const Entity* entity,
                                 const SimpleTransform& initialTransform) override;
    BodyTemplateObject createBodyTemplateFromGeometry(const Vector<Vec3>& vertices, const Vector<RawIndexedTriangle>& triangles,
                                                      bool deleteOnceUnused, float customCollisionMargin = 0.5f) override;
    BodyTemplateObject createBodyTemplateFromHeightmap(unsigned int heightmapWidth, unsigned int heightmapHeight,
                                                       const Vector<float>& heightmap, bool deleteOnceUnused) override;
    bool deleteBodyTemplate(BodyTemplateObject bodyTemplateObject) override;
    BodyTemplateObject createGeometryBodyFromTemplate(BodyTemplateObject bodyTemplateObject, float mass, bool fixed,
                                                      const Entity* entity, const SimpleTransform& initialTransform) override;
    BodyObject createHeightmapBodyFromTemplate(BodyTemplateObject bodyTemplateObject, float heightScale, float terrainScale,
                                               float mass, bool fixed, const Entity* entity,
                                               const SimpleTransform& initialTransform) override;
    bool deleteBody(BodyObject bodyObject) override;
    const Entity* getBodyEntity(BodyObject bodyObject) const override;
    bool getBodyTransform(BodyObject bodyObject, SimpleTransform& transform) override;
    bool setBodyTransform(BodyObject bodyObject, const SimpleTransform& transform) override;
    Vec3 getBodyLinearVelocity(BodyObject bodyObject) const override;
    bool setBodyLinearVelocity(BodyObject bodyObject, const Vec3& velocity) const override;
    Vec3 getBodyAngularVelocity(BodyObject bodyObject) const override;
    bool setBodyAngularVelocity(BodyObject bodyObject, const Vec3& velocity) const override;
    void applyForceToBody(BodyObject bodyObject, const Vec3& force, ForceMode mode) override;
    void applyTorqueToBody(BodyObject bodyObject, const Vec3& torque, ForceMode mode) override;
    bool constrainBodyToXYPlane(BodyObject bodyObject) override;
    JointObject createHingeJoint(BodyObject firstBodyObject, BodyObject secondBodyObject, const Vec3& globalAnchor,
                                 const Vec3& globalAxis) override;
    JointObject createBallAndSocketJoint(BodyObject firstBodyObject, BodyObject secondBodyObject, const Vec3& globalAnchor,
                                         const Vec3& angularLimits) override;
    bool deleteJoint(JointObject jointObject) override;
    bool getBodyJoints(BodyObject bodyObject, Vector<JointObject>& joints) const override;
    CharacterControllerObject createCharacterController(float height, float radius, const Entity* entity) override;
    bool deleteCharacterController(CharacterControllerObject characterControllerObject) override;
    Vec3 getCharacterControllerPosition(CharacterControllerObject characterControllerObject) const override;
    bool setCharacterControllerPosition(CharacterControllerObject characterControllerObject, const Vec3& position) override;
    void moveCharacterController(CharacterControllerObject characterControllerObject, const Vec3& move, float time) override;
    bool getCharacterControllerUpAxisCollision(CharacterControllerObject controllerObject,
                                               Vec3& collisionNormal) const override;
    bool getCharacterControllerDownAxisCollision(CharacterControllerObject controllerObject,
                                                 Vec3& collisionNormal) const override;
    TimeValue getSubstepSize() const override;
    void update(TimeValue time) override;
    const Vec3& getGravityVector() const override;
    void setGravityVector(const Vec3& gravity) override;
    bool raycast(const Ray& ray, PhysicsIntersectResult& result) const override;

    static btVector3 toBullet(const Vec3& v) { return {v.x, v.y, v.z}; }
    static Vec3 toCarbon(const btVector3& v) { return {v.x(), v.y(), v.z()}; }

    static btQuaternion toBullet(const Quaternion& q) { return btQuaternion(q.x, q.y, q.z, q.w).inverse(); }
    static Quaternion toCarbon(const btQuaternion& q) { return Quaternion(q.x(), q.y(), q.z(), q.w()).getInverse(); }

    static btTransform toBullet(const SimpleTransform& t)
    {
        return btTransform(toBullet(t.getOrientation()), toBullet(t.getPosition()));
    }
    static SimpleTransform toCarbon(const btTransform& t) { return {toCarbon(t.getOrigin()), toCarbon(t.getRotation())}; }

private:

    btDefaultCollisionConfiguration* collisionConfiguration_ = nullptr;
    btCollisionDispatcher* dispatcher_ = nullptr;
    btBroadphaseInterface* broadphase_ = nullptr;
    btGhostPairCallback* ghostPairCallback_ = nullptr;
    btConstraintSolver* solver_ = nullptr;
    btDynamicsWorld* dynamicsWorld_ = nullptr;

    Vec3 gravityVector_ = DefaultGravityVector;

    class BodyTemplate
    {
    public:

        btCollisionShape* collisionShape = nullptr;

        unsigned int heightmapWidth = 0;
        unsigned int heightmapHeight = 0;
        Vector<float> heightmapData;

        bool deleteOnceUnused = false;

        Vector<Vec3> vertices;
        Vector<RawIndexedTriangle> triangles;
        btTriangleIndexVertexArray* meshInterface = nullptr;

        BodyTemplate(bool deleteOnceUnused_) : deleteOnceUnused(deleteOnceUnused_) {}
    };

    // Internal class that maps to a Bullet rigid body.
    class Body
    {
    public:

        btRigidBody* bulletBody = nullptr;

        bool isFixed = false;

        const Entity* entity = nullptr;

        BodyTemplate* bodyTemplate = nullptr;    // The body template used to create this body

        btCollisionShape* ownedCollisionShape = nullptr;

        Body(btRigidBody* bulletBody_, const Entity* entity_, bool isFixed_)
            : bulletBody(bulletBody_), isFixed(isFixed_), entity(entity_)
        {
        }
    };

    // Internal class that maps to a Bullet joint (which Bullet calls a constraint)
    class Joint
    {
    public:

        Body* firstBody = nullptr;
        Body* secondBody = nullptr;

        btTypedConstraint* bulletConstraint = nullptr;
        void destroyBulletConstraint(btDynamicsWorld* world);

        Joint(Body* firstBody_, Body* secondBody_, btTypedConstraint* bulletConstraint_)
            : firstBody(firstBody_), secondBody(secondBody_), bulletConstraint(bulletConstraint_)
        {
        }
    };

    class CharacterController
    {
    public:

        KinematicCharacterController* bulletController = nullptr;
        btPairCachingGhostObject* ghostObject = nullptr;

        const Entity* entity = nullptr;

        CharacterController(KinematicCharacterController* bulletController_, btPairCachingGhostObject* ghostObject_,
                            const Entity* entity_)
            : bulletController(bulletController_), ghostObject(ghostObject_), entity(entity_)
        {
        }
    };

    unsigned int getBodyTemplateBodyCount(BodyTemplate* bodyTemplate) const;

    Vector<Body*> bodies_;
    Vector<BodyTemplate*> bodyTemplates_;
    Vector<Joint*> joints_;
};

}

#endif
