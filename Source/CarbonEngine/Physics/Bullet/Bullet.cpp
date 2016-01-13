/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_BULLET

#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Math/SimpleTransform.h"
#include "CarbonEngine/Physics/Bullet/Bullet.h"
#include "CarbonEngine/Physics/Bullet/KinematicCharacterController.h"

namespace Carbon
{

const auto DefaultLinearSleepingThreshold = 0.4f;
const auto DefaultAngularSleepingThreshold = 0.5f;
const auto DefaultLinearDamping = 0.15f;
const auto DefaultAngularDamping = 0.15f;

bool Bullet::setup()
{
    broadphase_ = new btDbvtBroadphase;

    ghostPairCallback_ = new btGhostPairCallback;
    broadphase_->getOverlappingPairCache()->setInternalGhostPairCallback(ghostPairCallback_);

    collisionConfiguration_ = new btDefaultCollisionConfiguration;
    dispatcher_ = new btCollisionDispatcher(collisionConfiguration_);

    solver_ = new btSequentialImpulseConstraintSolver;

    dynamicsWorld_ = new btDiscreteDynamicsWorld(dispatcher_, broadphase_, solver_, collisionConfiguration_);
    dynamicsWorld_->setGravity(toBullet(getGravityVector()));

    LOG_INFO << "Initialized Bullet " << (btGetVersion() / 100) << "." << (btGetVersion() % 100);

    return true;
}

void Bullet::shutdown()
{
    // Delete any remaining joints
    while (!joints_.empty())
        deleteJoint(joints_[0]);

    // Delete all bodies
    while (!bodies_.empty())
        deleteBody(bodies_[0]);

    // Delete all body templates
    while (!bodyTemplates_.empty())
        deleteBodyTemplate(bodyTemplates_[0]);

    delete dynamicsWorld_;
    dynamicsWorld_ = nullptr;

    delete solver_;
    solver_ = nullptr;

    delete dispatcher_;
    dispatcher_ = nullptr;

    delete collisionConfiguration_;
    collisionConfiguration_ = nullptr;

    delete broadphase_;
    broadphase_ = nullptr;

    delete ghostPairCallback_;
    ghostPairCallback_ = nullptr;
}

String Bullet::getEngineName() const
{
    return String() + "Bullet " + (btGetVersion() / 100) + "." + (btGetVersion() % 100);
}

PhysicsInterface::BodyObject Bullet::createBoundingBoxBody(const AABB& aabb, float mass, bool fixed, const Entity* entity,
                                                           const SimpleTransform& initialTransform)
{
    auto collisionShape = new btBoxShape(toBullet((aabb.getMaximum() - aabb.getMinimum()) * 0.5f));

    // Calculate inertia
    auto localInertia = btVector3(0.0f, 0.0f, 0.0f);
    if (!fixed)
        collisionShape->calculateLocalInertia(mass, localInertia);

    // Motion state for this body
    auto motionState = new btDefaultMotionState(toBullet(initialTransform),
                                                btTransform(btQuaternion::getIdentity(), toBullet(aabb.getCenter())));

    // Create Bullet rigid body
    auto bulletBody = new btRigidBody(
        btRigidBody::btRigidBodyConstructionInfo(fixed ? 0.0f : mass, motionState, collisionShape, localInertia));

    bulletBody->setDamping(DefaultLinearDamping, DefaultAngularDamping);
    bulletBody->setSleepingThresholds(DefaultLinearSleepingThreshold, DefaultAngularSleepingThreshold);
    bulletBody->setRestitution(0.0f);

    // Add body to the simulation
    dynamicsWorld_->addRigidBody(bulletBody);

    // Create local body
    auto body = new Body(bulletBody, entity, fixed);
    bodies_.append(body);

    body->ownedCollisionShape = collisionShape;

    bulletBody->setUserPointer(body);

    return body;
}

PhysicsInterface::BodyObject Bullet::createCapsuleBody(float height, float radius, float mass, bool fixed, const Entity* entity,
                                                       const SimpleTransform& initialTransform)
{
    auto collisionShape = new btCapsuleShape(radius, height);

    // Calculate inertia
    auto localInertia = btVector3(0.0f, 0.0f, 0.0f);
    if (!fixed)
        collisionShape->calculateLocalInertia(mass, localInertia);

    // Motion state for this body
    auto motionState = new btDefaultMotionState(toBullet(initialTransform));

    // Create Bullet rigid body
    auto bulletBody = new btRigidBody(
        btRigidBody::btRigidBodyConstructionInfo(fixed ? 0.0f : mass, motionState, collisionShape, localInertia));

    bulletBody->setDamping(DefaultLinearDamping, DefaultAngularDamping);
    bulletBody->setSleepingThresholds(DefaultLinearSleepingThreshold, DefaultAngularSleepingThreshold);
    bulletBody->setRestitution(0.0f);

    // Add body to the simulation
    dynamicsWorld_->addRigidBody(bulletBody);

    // Create local body
    auto body = new Body(bulletBody, entity, fixed);
    bodies_.append(body);

    body->ownedCollisionShape = collisionShape;

    bulletBody->setUserPointer(body);

    return body;
}

bool Bullet::constrainBodyToXYPlane(BodyObject bodyObject)
{
    if (!bodyObject)
        return false;

    auto body = reinterpret_cast<Body*>(bodyObject);

    body->bulletBody->setLinearFactor({1.0f, 1.0f, 0.0f});
    body->bulletBody->setAngularFactor({0.0f, 0.0f, 1.0f});

    return true;
}

PhysicsInterface::BodyTemplateObject Bullet::createBodyTemplateFromGeometry(const Vector<Vec3>& vertices,
                                                                            const Vector<RawIndexedTriangle>& triangles,
                                                                            bool deleteOnceUnused, float customCollisionMargin)
{
    if (vertices.empty() || triangles.empty())
        return nullptr;

    auto bodyTemplate = new BodyTemplate(deleteOnceUnused);
    bodyTemplates_.append(bodyTemplate);

    // Copy the geometry data
    bodyTemplate->vertices = vertices;
    bodyTemplate->triangles = triangles;

    // Create interface to the geometry data for Bullet to use
    auto mesh = btIndexedMesh();
    mesh.m_numTriangles = bodyTemplate->triangles.size();
    mesh.m_triangleIndexBase = reinterpret_cast<byte_t*>(bodyTemplate->triangles.getData());
    mesh.m_triangleIndexStride = 3 * sizeof(unsigned int);
    mesh.m_numVertices = bodyTemplate->vertices.size();
    mesh.m_vertexBase = bodyTemplate->vertices.as<byte_t>();
    mesh.m_vertexStride = 3 * sizeof(float);

    auto meshInterface = new btTriangleIndexVertexArray();
    meshInterface->addIndexedMesh(mesh);

    // Calculate an AABB around the geometry
    auto aabb = AABB(bodyTemplate->vertices);

    // Create the collision shape
    bodyTemplate->collisionShape =
        new btBvhTriangleMeshShape(meshInterface, true, toBullet(aabb.getMinimum()), toBullet(aabb.getMaximum()));

    if (customCollisionMargin > 0.0f)
        bodyTemplate->collisionShape->setMargin(customCollisionMargin);

    return bodyTemplate;
}

PhysicsInterface::BodyTemplateObject Bullet::createBodyTemplateFromHeightmap(unsigned int heightmapWidth,
                                                                             unsigned int heightmapHeight,
                                                                             const Vector<float>& heightmap,
                                                                             bool deleteOnceUnused)
{
    auto bodyTemplate = new BodyTemplate(deleteOnceUnused);

    try
    {
        bodyTemplate->heightmapData.resize(heightmapWidth * heightmapHeight);
    }
    catch (const std::bad_alloc&)
    {
        LOG_ERROR << "Failed allocating memory for the heightmap data";

        delete bodyTemplate;
        bodyTemplate = nullptr;

        return nullptr;
    }

    // Store the heightmap size and data on the template
    bodyTemplate->heightmapWidth = heightmapWidth;
    bodyTemplate->heightmapHeight = heightmapHeight;
    bodyTemplate->heightmapData = heightmap;

    bodyTemplates_.append(bodyTemplate);

    return bodyTemplate;
}

bool Bullet::deleteBodyTemplate(BodyTemplateObject bodyTemplateObject)
{
    if (!bodyTemplateObject)
        return false;

    auto bodyTemplate = reinterpret_cast<BodyTemplate*>(bodyTemplateObject);

    if (getBodyTemplateBodyCount(bodyTemplate))
    {
        LOG_ERROR << "Can't delete body templates that are in use by a body";
        return false;
    }

    delete bodyTemplate->collisionShape;
    bodyTemplate->collisionShape = nullptr;

    bodyTemplates_.unorderedEraseValue(bodyTemplate);

    delete bodyTemplate;
    bodyTemplate = nullptr;

    return true;
}

unsigned int Bullet::getBodyTemplateBodyCount(BodyTemplate* bodyTemplate) const
{
    if (!bodyTemplate)
        return 0;

    return bodies_.count([&](const Body* body) { return body->bodyTemplate == bodyTemplate; });
}

PhysicsInterface::BodyObject Bullet::createGeometryBodyFromTemplate(BodyTemplateObject bodyTemplateObject, float mass,
                                                                    bool fixed, const Entity* entity,
                                                                    const SimpleTransform& initialTransform)
{
    auto bodyTemplate = reinterpret_cast<BodyTemplate*>(bodyTemplateObject);

    if (!bodyTemplateObject || !bodyTemplate->collisionShape)
    {
        LOG_ERROR << "Invalid body template";
        return nullptr;
    }

    // Calculate inertia
    auto localInertia = btVector3(0.0f, 0.0f, 0.0f);
    if (!fixed)
        bodyTemplate->collisionShape->calculateLocalInertia(mass, localInertia);

    // Motion state for this body
    auto motionState =
        new btDefaultMotionState(toBullet(initialTransform), btTransform(btQuaternion::getIdentity(), toBullet(Vec3::Zero)));

    // Create Bullet rigid body
    auto bulletBody = new btRigidBody(
        btRigidBody::btRigidBodyConstructionInfo((fixed ? 0.0f : mass), motionState, bodyTemplate->collisionShape,
                                                 (fixed ? btVector3(0.0f, 0.0f, 0.0f) : localInertia)));

    bulletBody->setDamping(DefaultLinearDamping, DefaultAngularDamping);
    bulletBody->setSleepingThresholds(DefaultLinearSleepingThreshold, DefaultAngularSleepingThreshold);
    bulletBody->setRestitution(0.0f);

    // Add body to the simulation
    dynamicsWorld_->addRigidBody(bulletBody);

    // Create local body
    auto body = new Body(bulletBody, entity, fixed);
    bodies_.append(body);

    bulletBody->setUserPointer(body);

    return body;
}

PhysicsInterface::BodyObject Bullet::createHeightmapBodyFromTemplate(BodyTemplateObject bodyTemplateObject, float heightScale,
                                                                     float terrainScale, float mass, bool fixed,
                                                                     const Entity* entity,
                                                                     const SimpleTransform& initialTransform)
{
    auto bodyTemplate = reinterpret_cast<BodyTemplate*>(bodyTemplateObject);

    if (!bodyTemplateObject || bodyTemplate->heightmapData.empty())
    {
        LOG_ERROR << "Invalid body template";
        return nullptr;
    }

    // Construct a mesh out of the heightmap data, native Bullet heightmaps aren't yet supported by this backend

    auto heightmapWidth = bodyTemplate->heightmapWidth;
    auto heightmapHeight = bodyTemplate->heightmapHeight;
    auto heightmap = bodyTemplate->heightmapData.getData();

    // Create vertex array
    auto vertices = Vector<Vec3>(heightmapWidth * heightmapHeight);

    for (auto y = 0U; y < heightmapHeight; y++)
    {
        for (auto x = 0U; x < heightmapWidth; x++)
        {
            vertices[y * heightmapWidth + x].setXYZ(float(x) * terrainScale, heightmap[y * heightmapWidth + x] * heightScale,
                                                    float(y) * terrainScale);
        }
    }

    // Create triangle array
    auto triangles = Vector<RawIndexedTriangle>((heightmapWidth - 1) * (heightmapHeight - 1) * 2);
    auto currentTriangle = 0U;

    for (auto y = 0U; y < heightmapHeight - 1; y++)
    {
        for (auto x = 0U; x < heightmapWidth - 1; x++)
        {
            triangles[currentTriangle++].setIndex(0, y * heightmapWidth + x);
            triangles[currentTriangle++].setIndex(1, (y + 1) * heightmapWidth + x);
            triangles[currentTriangle++].setIndex(2, y * heightmapWidth + x + 1);

            triangles[currentTriangle++].setIndex(0, (y + 1) * heightmapWidth + x);
            triangles[currentTriangle++].setIndex(1, (y + 1) * heightmapWidth + x + 1);
            triangles[currentTriangle++].setIndex(2, y * heightmapWidth + x + 1);
        }
    }

    // Create a temporary geometry body for the heightmap geometry and then create the final heightmap body from that template
    auto temporaryBodyTemplate = createBodyTemplateFromGeometry(vertices, triangles, true);
    if (!temporaryBodyTemplate)
    {
        LOG_ERROR << "Failed creating temporary body template for heightmap";
        return nullptr;
    }

    return createGeometryBodyFromTemplate(temporaryBodyTemplate, mass, fixed, entity, initialTransform);
}

bool Bullet::deleteBody(BodyObject bodyObject)
{
    if (!bodyObject)
        return false;

    auto body = reinterpret_cast<Body*>(bodyObject);

    // Any joints using this body are automatically removed from the simulation and any pointers to the body to be deleted are
    // nulled, but the Bullet::Joint instance lives on until an actual call to PhysX::deleteJoint()
    for (auto joint : joints_)
    {
        if (joint->firstBody == body || joint->secondBody == body)
        {
            joint->destroyBulletConstraint(dynamicsWorld_);

            if (joint->firstBody == body)
                joint->firstBody = nullptr;
            if (joint->secondBody == body)
                joint->secondBody = nullptr;
        }
    }

    // Clean up rigid body
    dynamicsWorld_->removeRigidBody(body->bulletBody);
    delete body->bulletBody->getMotionState();
    delete body->bulletBody;
    body->bulletBody = nullptr;

    // Destroy this body's template if it is flagged to be destroyed when the last user of that template is destroyed and this
    // body is that last user
    auto bodyTemplate = body->bodyTemplate;
    if (bodyTemplate && bodyTemplate->deleteOnceUnused && getBodyTemplateBodyCount(bodyTemplate) == 1)
    {
        body->bodyTemplate = nullptr;
        deleteBodyTemplate(bodyTemplate);
    }

    delete body->ownedCollisionShape;
    body->ownedCollisionShape = nullptr;

    bodies_.unorderedEraseValue(body);

    delete body;
    body = nullptr;

    return true;
}

const Entity* Bullet::getBodyEntity(BodyObject bodyObject) const
{
    if (!bodyObject)
        return nullptr;

    return reinterpret_cast<Body*>(bodyObject)->entity;
}

bool Bullet::getBodyTransform(BodyObject bodyObject, SimpleTransform& transform)
{
    if (!bodyObject || !isUpdating())
        return false;

    auto body = reinterpret_cast<Body*>(bodyObject);

    auto bulletTransform = btTransform();
    body->bulletBody->getMotionState()->getWorldTransform(bulletTransform);

    // Return the body transform
    transform = toCarbon(bulletTransform);

    // TODO: track sleeping/waking objects to know what's alive/dead and what has changed since the last call

    return true;
}

bool Bullet::setBodyTransform(BodyObject bodyObject, const SimpleTransform& transform)
{
    if (!bodyObject)
        return false;

    auto body = reinterpret_cast<Body*>(bodyObject);

    if (body->isFixed)
    {
        LOG_ERROR << "Can't alter the physics transform of a fixed body";
        return false;
    }

    // Set new transform on the body
    body->bulletBody->getWorldTransform() = toBullet(transform);

    // Put this body back in the active list since it's been manually reconfigured
    body->bulletBody->activate(true);

    return true;
}

Vec3 Bullet::getBodyLinearVelocity(BodyObject bodyObject) const
{
    if (!bodyObject)
        return Vec3::Zero;

    auto body = reinterpret_cast<Body*>(bodyObject);

    return toCarbon(body->bulletBody->getLinearVelocity());
}

bool Bullet::setBodyLinearVelocity(BodyObject bodyObject, const Vec3& velocity) const
{
    if (!bodyObject)
        return false;

    auto body = reinterpret_cast<Body*>(bodyObject);

    body->bulletBody->setLinearVelocity(toBullet(velocity));
    body->bulletBody->activate(true);

    return true;
}

Vec3 Bullet::getBodyAngularVelocity(BodyObject bodyObject) const
{
    if (!bodyObject)
        return Vec3::Zero;

    auto body = reinterpret_cast<Body*>(bodyObject);

    return toCarbon(body->bulletBody->getAngularVelocity());
}

bool Bullet::setBodyAngularVelocity(BodyObject bodyObject, const Vec3& velocity) const
{
    if (!bodyObject)
        return false;

    auto body = reinterpret_cast<Body*>(bodyObject);

    body->bulletBody->setAngularVelocity(toBullet(velocity));
    body->bulletBody->activate(true);

    return true;
}

void Bullet::applyForceToBody(BodyObject bodyObject, const Vec3& force, ForceMode mode)
{
    if (!bodyObject)
        return;

    auto body = reinterpret_cast<Body*>(bodyObject);

    body->bulletBody->activate(true);

    if (mode == ForceStandard)
        body->bulletBody->applyCentralForce(toBullet(force * 100.0f));
    else if (mode == ForceImpulse)
        body->bulletBody->applyCentralImpulse(toBullet(force));
}

void Bullet::applyTorqueToBody(BodyObject bodyObject, const Vec3& torque, ForceMode mode)
{
    if (!bodyObject)
        return;

    auto body = reinterpret_cast<Body*>(bodyObject);

    body->bulletBody->activate(true);

    if (mode == ForceStandard)
        body->bulletBody->applyTorque(toBullet(torque));
    else if (mode == ForceImpulse)
        body->bulletBody->applyTorqueImpulse(toBullet(torque));
}

PhysicsInterface::JointObject Bullet::createHingeJoint(BodyObject firstBodyObject, BodyObject secondBodyObject,
                                                       const Vec3& globalAnchor, const Vec3& globalAxis)
{
    if (!firstBodyObject || !secondBodyObject || firstBodyObject == secondBodyObject)
    {
        LOG_ERROR << "Invalid bodies";
        return nullptr;
    }

    auto firstBody = reinterpret_cast<Body*>(firstBodyObject);
    auto secondBody = reinterpret_cast<Body*>(secondBodyObject);

    auto firstBodyTransform = btTransform();
    auto secondBodyTransform = btTransform();
    firstBody->bulletBody->getMotionState()->getWorldTransform(firstBodyTransform);
    secondBody->bulletBody->getMotionState()->getWorldTransform(secondBodyTransform);

    auto btConstraint = new btHingeConstraint(
        *firstBody->bulletBody, *secondBody->bulletBody, firstBodyTransform.inverse() * toBullet(globalAnchor),
        secondBodyTransform.inverse() * toBullet(globalAnchor), firstBodyTransform.getBasis().inverse() * toBullet(globalAxis),
        secondBodyTransform.getBasis().inverse() * toBullet(globalAxis));

    joints_.append(new Joint(firstBody, secondBody, btConstraint));

    dynamicsWorld_->addConstraint(joints_.back()->bulletConstraint, true);

    return joints_.back();
}

PhysicsInterface::JointObject Bullet::createBallAndSocketJoint(BodyObject firstBodyObject, BodyObject secondBodyObject,
                                                               const Vec3& globalAnchor, const Vec3& angularLimits)
{
    if (!firstBodyObject || !secondBodyObject || firstBodyObject == secondBodyObject)
    {
        LOG_ERROR << "Invalid bodies";
        return nullptr;
    }

    auto firstBody = reinterpret_cast<Body*>(firstBodyObject);
    auto secondBody = reinterpret_cast<Body*>(secondBodyObject);

    auto firstBodyTransform = btTransform();
    auto secondBodyTransform = btTransform();
    firstBody->bulletBody->getMotionState()->getWorldTransform(firstBodyTransform);
    secondBody->bulletBody->getMotionState()->getWorldTransform(secondBodyTransform);

    auto firstLocalAnchor = btTransform(btQuaternion::getIdentity(), firstBodyTransform.inverse() * toBullet(globalAnchor));
    auto secondLocalAnchor = btTransform(btQuaternion::getIdentity(), secondBodyTransform.inverse() * toBullet(globalAnchor));

    auto btConstraint =
        new btGeneric6DofConstraint(*firstBody->bulletBody, *secondBody->bulletBody, firstLocalAnchor, secondLocalAnchor, true);

    if (angularLimits != Vec3::Zero)
    {
        btConstraint->setAngularLowerLimit(toBullet(-angularLimits));
        btConstraint->setAngularUpperLimit(toBullet(angularLimits));
    }

    joints_.append(new Joint(firstBody, secondBody, btConstraint));

    dynamicsWorld_->addConstraint(joints_.back()->bulletConstraint, true);

    return joints_.back();
}

bool Bullet::deleteJoint(JointObject jointObject)
{
    if (!jointObject)
        return false;

    auto joint = reinterpret_cast<Joint*>(jointObject);

    joint->destroyBulletConstraint(dynamicsWorld_);

    joints_.unorderedEraseValue(joint);

    delete joint;
    joint = nullptr;

    return true;
}

void Bullet::Joint::destroyBulletConstraint(btDynamicsWorld* world)
{
    if (bulletConstraint)
    {
        world->removeConstraint(bulletConstraint);

        delete bulletConstraint;
        bulletConstraint = nullptr;
    }
}

bool Bullet::getBodyJoints(BodyObject bodyObject, Vector<JointObject>& joints) const
{
    if (!bodyObject)
        return false;

    joints.clear();

    for (auto joint : joints_)
    {
        if (joint->firstBody == reinterpret_cast<Body*>(bodyObject) || joint->secondBody == reinterpret_cast<Body*>(bodyObject))
            joints.append(joint);
    }

    return true;
}

PhysicsInterface::CharacterControllerObject Bullet::createCharacterController(float height, float radius, const Entity* entity)
{
    auto ghostObject = new btPairCachingGhostObject;

    auto cylinder = new btCylinderShape({radius, height * 0.5f, radius});
    ghostObject->setCollisionShape(cylinder);
    ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
    ghostObject->setRestitution(0.0f);

    auto stepHeight = btScalar(5.0);
    auto controller = new KinematicCharacterController(ghostObject, cylinder, stepHeight);

    dynamicsWorld_->addCollisionObject(ghostObject, btBroadphaseProxy::CharacterFilter,
                                       btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter);

    dynamicsWorld_->addAction(controller);

    return new CharacterController(controller, ghostObject, entity);
}

bool Bullet::deleteCharacterController(CharacterControllerObject characterControllerObject)
{
    if (!characterControllerObject)
        return false;

    auto characterController = reinterpret_cast<CharacterController*>(characterControllerObject);

    dynamicsWorld_->removeAction(characterController->bulletController);
    dynamicsWorld_->removeCollisionObject(characterController->ghostObject);

    delete characterController->bulletController;
    delete characterController;

    return true;
}

Vec3 Bullet::getCharacterControllerPosition(CharacterControllerObject characterControllerObject) const
{
    if (!characterControllerObject)
        return Vec3::Zero;

    auto characterController = reinterpret_cast<CharacterController*>(characterControllerObject);

    return toCarbon(characterController->ghostObject->getWorldTransform().getOrigin());
}

bool Bullet::setCharacterControllerPosition(CharacterControllerObject characterControllerObject, const Vec3& position)
{
    if (!characterControllerObject)
        return false;

    auto characterController = reinterpret_cast<CharacterController*>(characterControllerObject);

    characterController->bulletController->setWorldPosition(toBullet(position));

    return true;
}

void Bullet::moveCharacterController(CharacterControllerObject characterControllerObject, const Vec3& move, float time)
{
    if (!characterControllerObject)
        return;

    auto characterController = reinterpret_cast<CharacterController*>(characterControllerObject);

    characterController->bulletController->setVelocityForTimeInterval(toBullet(move / time), time);
}

bool Bullet::getCharacterControllerUpAxisCollision(CharacterControllerObject controllerObject, Vec3& collisionNormal) const
{
    if (!controllerObject)
        return false;

    auto characterController = reinterpret_cast<CharacterController*>(controllerObject);

    auto normal = btVector3();
    if (!characterController->bulletController->getUpAxisCollision(normal))
        return false;

    collisionNormal = toCarbon(normal);

    return true;
}

bool Bullet::getCharacterControllerDownAxisCollision(CharacterControllerObject controllerObject, Vec3& collisionNormal) const
{
    if (!controllerObject)
        return false;

    auto characterController = reinterpret_cast<CharacterController*>(controllerObject);

    auto normal = btVector3();
    if (!characterController->bulletController->getDownAxisCollision(normal))
        return false;

    collisionNormal = toCarbon(normal);

    return true;
}

TimeValue Bullet::getSubstepSize() const
{
    return TimeValue(1.0f / 60.0f);
}

void Bullet::update(TimeValue time)
{
    if (isUpdating())
        dynamicsWorld_->stepSimulation(time.toSeconds(), 8, getSubstepSize().toSeconds());
}

const Vec3& Bullet::getGravityVector() const
{
    return gravityVector_;
}

void Bullet::setGravityVector(const Vec3& gravity)
{
    gravityVector_ = gravity;

    dynamicsWorld_->setGravity(toBullet(gravityVector_));
}

// How far the ray is shot
const auto MaxRayDistance = 10000.0f;

// Subclass of btCollisionWorld::RayResultCallback that collects ray intersection results and puts them into a vector.
class RayResultCollector : public btCollisionWorld::RayResultCallback
{
public:

    RayResultCollector(const Ray& ray, Vector<PhysicsIntersectResult>& results)
        : btCollisionWorld::RayResultCallback(), ray_(ray), results_(results)
    {
    }

    btScalar addSingleResult(btCollisionWorld::LocalRayResult& result, bool normalInWorldSpace) override
    {
        auto bodyObject = result.m_collisionObject->getUserPointer();

        auto normal = normalInWorldSpace ? result.m_hitNormalLocal :
                                           result.m_collisionObject->getWorldTransform().getBasis() * result.m_hitNormalLocal;

        results_.emplace(result.m_hitFraction * MaxRayDistance, ray_.getPoint(result.m_hitFraction * MaxRayDistance),
                         Bullet::toCarbon(normal), bodyObject);

        return 1.0f;
    }

private:

    const Ray& ray_;
    Vector<PhysicsIntersectResult>& results_;
};

bool Bullet::raycast(const Ray& ray, PhysicsIntersectResult& result) const
{
    auto endPoint = ray.getPoint(MaxRayDistance);

    auto intersections = Vector<PhysicsIntersectResult>();
    auto callback = RayResultCollector(ray, intersections);
    dynamicsWorld_->rayTest(toBullet(ray.getOrigin()), toBullet(endPoint), callback);

    if (intersections.empty())
        return false;

    intersections.sort();
    result = intersections[0];

    return true;
}

}

#endif
