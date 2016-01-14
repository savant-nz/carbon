/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifdef CARBON_INCLUDE_PHYSX

#include "CarbonEngine/Core/SharedLibrary.h"
#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Physics/PhysicsInterface.h"
#include "CarbonEngine/Physics/PhysicsIntersectResult.h"
#include "CarbonEngine/Physics/PhysX/PhysXIncludeWrapper.h"

#if defined(WINDOWS) || defined(LINUX)
    #define CARBON_PHYSX_DYNAMIC_LIBRARY
#endif

namespace Carbon
{

using namespace physx;

class PhysXOutputStream;

/**
 * PhysX physics backend.
 */
class PhysX : public PhysicsInterface
{
public:

    ~PhysX() override { shutdown(); }

    bool isAvailable() const override;
    bool setup() override;
    void shutdown() override;
    String getEngineName() const override;
    void setWorldLimits(const AABB& aabb) override;
    BodyObject createBoundingBoxBody(const AABB& aabb, float mass, bool fixed, const Entity* entity,
                                     const SimpleTransform& initialTransform) override;
    BodyObject createCapsuleBody(float height, float radius, float mass, bool fixed, const Entity* entity,
                                 const SimpleTransform& initialTransform) override;
    BodyTemplateObject createBodyTemplateFromGeometry(const Vector<Vec3>& vertices, const Vector<RawIndexedTriangle>& triangles,
                                                      bool deleteOnceUnused, float customCollisionMargin = 0.0f) override;
    bool preProcessGeometry(const Vector<Vec3>& vertices, const Vector<RawIndexedTriangle>& triangles,
                            Vector<byte_t>& output) override;
    BodyTemplateObject createBodyTemplateFromPreProcessedGeometry(const Vector<byte_t>& data, bool deleteOnceUnused) override;
    BodyTemplateObject createBodyTemplateFromHeightmap(unsigned int heightmapWidth, unsigned int heightmapHeight,
                                                       const Vector<float>& heightmap, bool deleteOnceUnused) override;
    bool deleteBodyTemplate(BodyTemplateObject bodyTemplateObject) override;
    BodyObject createGeometryBodyFromTemplate(BodyTemplateObject bodyTemplateObject, float mass, bool fixed,
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
    TimeValue getSubstepSize() const override;
    void update(TimeValue time) override;
    const Vec3& getGravityVector() const override;
    void setGravityVector(const Vec3& gravity) override;
    bool raycast(const Ray& ray, PhysicsIntersectResult& result) const override;

private:

    friend class PhysXRaycastReporter;

    static PxVec3 toPx(const Vec3& v) { return {v.x, v.y, v.z}; }
    static PxQuat toPx(const Quaternion& q) { return {q.x, q.y, q.z, q.w}; }
    static PxTransform toPx(const SimpleTransform& t) { return {toPx(t.getPosition()), toPx(t.getOrientation())}; }
    static PxForceMode::Enum toPx(ForceMode mode)
    {
        if (mode == ForceImpulse || mode == ForceSmoothImpulse)
            return PxForceMode::eIMPULSE;

        return PxForceMode::eFORCE;
    }
    static PxExtendedVec3 toPxEx(const Vec3& v) { return {v.x, v.y, v.z}; }

    static Vec3 toCarbon(const PxVec3& v) { return {v.x, v.y, v.z}; }
    static Vec3 toCarbon(const PxExtendedVec3& v) { return {float(v.x), float(v.y), float(v.z)}; }
    static Quaternion toCarbon(const PxQuat& q) { return {q.x, q.y, q.z, q.w}; }
    static SimpleTransform toCarbon(const PxTransform& t) { return {toCarbon(t.p), toCarbon(t.q).getInverse()}; }
    static PhysicsIntersectResult toCarbon(const PxLocationHit& h)
    {
        return {h.distance, toCarbon(h.position), toCarbon(h.normal), reinterpret_cast<BodyObject*>(h.actor->userData)};
    }

    // Stores a PhysX template for a mesh or heightfield.
    class BodyTemplate
    {
    public:

        PxTriangleMesh* pxTriangleMesh = nullptr;

        PxHeightField* pxHeightField = nullptr;
        float heightScaleFactor = 0.0f;

        bool deleteOnceUnused = false;
    };

    // Internal class that maps to a PhysX actor.
    class Body
    {
    public:

        PxRigidActor* pxActor = nullptr;

        const Entity* entity = nullptr;

        SimpleTransform transform;

        BodyTemplate* bodyTemplate = nullptr;    // The body template used to create this body, if one was used

        Body(PxRigidActor* pxActor_, const Entity* entity_, BodyTemplate* bodyTemplate_)
            : pxActor(pxActor_), entity(entity_), bodyTemplate(bodyTemplate_)
        {
            if (pxActor)
                pxActor->userData = this;
        }
    };

    PhysicsInterface::BodyObject createBody(PxShape& pxShape, const SimpleTransform& initialTransform, float mass, bool fixed,
                                            const Entity* entity, BodyTemplate* bodyTemplate);

    // Internal class that maps to a PhysX joint.
    class Joint
    {
    public:

        Body* firstBody = nullptr;
        Body* secondBody = nullptr;

        PxJoint* pxJoint = nullptr;

        Joint(Body* firstBody_, Body* secondBody_, PxJoint* pxJoint_)
            : firstBody(firstBody_), secondBody(secondBody_), pxJoint(pxJoint_)
        {
        }
    };

    // Internal class that maps to a PhysX controller.
    class CharacterController
    {
    public:

        PxController* pxController = nullptr;

        TimeValue lastUpdateTime;

        const Entity* entity = nullptr;
    };

    unsigned int getBodyTemplateBodyCount(BodyTemplate* bodyTemplate) const;

    TimeValue timeSinceLastUpdate_;

    PxFoundation* foundation_ = nullptr;
    PxPhysics* physics_ = nullptr;
    PxCooking* cooking_ = nullptr;
    PxDefaultCpuDispatcher* cpuDispatcher_ = nullptr;
    PxScene* pxScene_ = nullptr;
    PxMaterial* defaultMaterial_ = nullptr;

    PxControllerManager* controllerManager_ = nullptr;

    Vec3 gravityVector_ = DefaultGravityVector;

    Vector<Body*> bodies_;
    Vector<Joint*> joints_;
    Vector<CharacterController*> characterControllers_;
    Vector<BodyTemplate*> bodyTemplates_;

    template <typename T> static void safeRelease(T*& p)
    {
        if (p)
        {
            p->release();
            p = nullptr;
        }
    }

#ifdef CARBON_PHYSX_DYNAMIC_LIBRARY

    // When used as a dynamic library PhysX is loaded dynamically at runtime in order to avoid a fixed dependency. If the
    // relevant shared libraries aren't available at runtime then the PhysX backend won't be available for use. Below are the
    // functions implemented in the dynamic PhysX libraries that are mapped at runtime.

    struct
    {
        SharedLibrary PhysX3, PhysX3Common, PhysX3Cooking, PhysX3CharacterKinematic, PhysX3Extensions;
    } libs_;

    typedef PxFoundation*(PX_CALL_CONV* PFnPxCreateFoundation)(PxU32 version, PxAllocatorCallback& allocator,
                                                               PxErrorCallback& errorCallback);
    typedef PxFoundation&(PX_CALL_CONV* PFnPxGetFoundation)();
    typedef PxPhysics*(PX_CALL_CONV* PFnPxCreateBasePhysics)(PxU32 version, PxFoundation& foundation,
                                                             const PxTolerancesScale& scale, bool trackOutstandingAllocations,
                                                             PxProfileZoneManager* profileZoneManager);
    typedef void(PX_CALL_CONV* PFnPxRegisterArticulations)(PxPhysics& physics);
    typedef void(PX_CALL_CONV* PFnPxRegisterHeightFields)(PxPhysics& physics);
    typedef PxCooking*(PX_CALL_CONV* PFnPxCreateCooking)(PxU32 version, PxFoundation& foundation,
                                                         const PxCookingParams& params);
    typedef PxControllerManager*(PX_CALL_CONV* PFnPxCreateControllerManager)(PxScene& scene);
    typedef PxRigidDynamic*(PX_CALL_CONV* PFnPxCreateDynamic)(PxPhysics& sdk, const PxTransform& transform, PxShape& shape,
                                                              PxReal density);
    typedef PxRigidStatic*(PX_CALL_CONV* PFnPxCreateStatic)(PxPhysics& sdk, const PxTransform& transform, PxShape& shape);
    typedef PxDefaultCpuDispatcher*(PX_CALL_CONV* PFnPxDefaultCpuDispatcherCreate)(PxU32 numThreads, PxU32* affinityMasks);
    typedef PxFilterFlags(PX_CALL_CONV* PFnPxDefaultSimulationFilterShader)(PxFilterObjectAttributes attributes0,
                                                                            PxFilterData filterData0,
                                                                            PxFilterObjectAttributes attributes1,
                                                                            PxFilterData filterData1, PxPairFlags& pairFlags,
                                                                            const void* constantBlock, PxU32 constantBlockSize);
    typedef PxRevoluteJoint*(PX_CALL_CONV* PFnPxRevoluteJointCreate)(PxPhysics& physics, PxRigidActor* actor0,
                                                                     const PxTransform& localFrame0, PxRigidActor* actor1,
                                                                     const PxTransform& localFrame1);
    typedef PxSphericalJoint*(PX_CALL_CONV* PFnPxSphericalJointCreate)(PxPhysics& physics, PxRigidActor* actor0,
                                                                       const PxTransform& localFrame0, PxRigidActor* actor1,
                                                                       const PxTransform& localFrame1);

    PFnPxCreateFoundation PxCreateFoundation = nullptr;
    PFnPxGetFoundation PxGetFoundation = nullptr;
    PFnPxCreateBasePhysics PxCreateBasePhysics = nullptr;
    PFnPxRegisterArticulations PxRegisterArticulations = nullptr;
    PFnPxRegisterHeightFields PxRegisterHeightFields = nullptr;
    PFnPxCreateCooking PxCreateCooking = nullptr;
    PFnPxCreateControllerManager PxCreateControllerManager = nullptr;
    PFnPxCreateDynamic PxCreateDynamic = nullptr;
    PFnPxCreateStatic PxCreateStatic = nullptr;
    PFnPxDefaultCpuDispatcherCreate PxDefaultCpuDispatcherCreate = nullptr;
    PFnPxDefaultSimulationFilterShader PxDefaultSimulationFilterShader = nullptr;
    PFnPxRevoluteJointCreate PxRevoluteJointCreate = nullptr;
    PFnPxSphericalJointCreate PxSphericalJointCreate = nullptr;

    UnicodeString getDynamicLibraryName(const UnicodeString& baseName) const
    {
        auto name = UnicodeString();

#ifdef LINUX
        name = "lib";
#endif

        name += baseName;

#ifdef DEBUG
        name += "DEBUG";
#endif

#ifdef CARBON_64BIT
        name += "_x64";
#else
        name += "_x86";
#endif

#ifdef WINDOWS
        name += ".dll";
#elif defined(LINUX)
        name += ".so";
#endif

        return name;
    }
#endif
};

}

#endif
