/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_PHYSX

#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Core/SharedLibrary.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Matrix4.h"
#include "CarbonEngine/Math/SimpleTransform.h"
#include "CarbonEngine/Physics/PhysX/PhysX.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

// PhysX output stream that writes to a Vector memory buffer.
class PhysXVectorOutputStream : public PxOutputStream
{
public:

    PhysXVectorOutputStream(Vector<byte_t>& data) : data_(data) { data_.clear(); }

    PxU32 write(const void* src, PxU32 count) override
    {
        try
        {
            data_.resize(data_.size() + count);
        }
        catch (const std::bad_alloc&)
        {
            return 0;
        }

        memcpy(&data_[data_.size() - count], src, count);

        return count;
    }

private:

    Vector<byte_t>& data_;
};

// PhysX input stream that reads from a Vector.
class PhysXVectorInputStream : public PxInputStream
{
public:

    PhysXVectorInputStream(const Vector<byte_t>& data) : data_(data) {}

    PxU32 read(void* dest, PxU32 count) override
    {
        if (offset_ >= data_.size() || !count)
            return 0;

        auto byteCount = std::min(count, data_.size() - offset_);

        memcpy(dest, &data_[offset_], byteCount);
        offset_ += byteCount;

        return byteCount;
    }

private:

    const Vector<byte_t>& data_;

    PxU32 offset_ = 0;
};

// PhysX allocator, on Windows this ensures 16-byte alignment, on other platforms it maps through to MemoryInterceptor.
class PhysXAllocator : public PxAllocatorCallback
{
public:

    void* allocate(size_t size, const char* typeName, const char* filename, int line) override
    {
#ifdef WINDOWS
        return _aligned_malloc(size, 16);
#else
        MemoryInterceptor::start(filename, line);
        return MemoryInterceptor::allocate(size);
#endif
    }

    void deallocate(void* ptr) override
    {
#ifdef WINDOWS
        _aligned_free(ptr);
#else
        MemoryInterceptor::free(ptr);
#endif
    }
};
static auto pxAllocator = PhysXAllocator();

// Handles routing of PhysX output into the engine's main logfile.
static class PhysXErrorCallback : public PxErrorCallback
{
public:

    void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) override
    {
        auto outputType = Logfile::Error;
        auto type = UnicodeString();

        switch (code)
        {
            case PxErrorCode::eDEBUG_WARNING:
                outputType = Logfile::Warning;
                break;
            case PxErrorCode::eINVALID_PARAMETER:
                type = "Invalid parameter";
                break;
            case PxErrorCode::eINVALID_OPERATION:
                type = "Invalid operation";
                break;
            case PxErrorCode::eOUT_OF_MEMORY:
                type = "Out of memory";
                break;
            case PxErrorCode::eINTERNAL_ERROR:
                type = "Internal error";
                break;
            case PxErrorCode::eABORT:
                type = "Abort";
                break;
            case PxErrorCode::ePERF_WARNING:
                outputType = Logfile::Warning;
                type = "Performance warning";
                break;
            default:
                type = "Unknown error code";
                break;
        }

        Logfile::get().writeLine("", type << " - " << message << " (" << file << ":" << line << ")", outputType);
    }
} pxErrorCallback;

bool PhysX::isAvailable() const
{
#ifdef CARBON_PHYSX_DYNAMIC_LIBRARY
    auto lib = SharedLibrary();
    if (!lib.load(getDynamicLibraryName("PhysX3Common")) || !lib.load(getDynamicLibraryName("PhysX3")) ||
        !lib.load(getDynamicLibraryName("PhysX3Cooking")) || !lib.load(getDynamicLibraryName("PhysX3Extensions")) ||
        !lib.load(getDynamicLibraryName("PhysX3CharacterKinematic")))
        return false;
#endif

    return true;
}

bool PhysX::setup()
{
    try
    {
        shutdown();

#ifdef CARBON_PHYSX_DYNAMIC_LIBRARY
        if (!libs_.PhysX3Common.load(getDynamicLibraryName("PhysX3Common")) ||
            !libs_.PhysX3.load(getDynamicLibraryName("PhysX3")) ||
            !libs_.PhysX3Cooking.load(getDynamicLibraryName("PhysX3Cooking")) ||
            !libs_.PhysX3Extensions.load(getDynamicLibraryName("PhysX3Extensions")) ||
            !libs_.PhysX3CharacterKinematic.load(getDynamicLibraryName("PhysX3CharacterKinematic")))
            throw Exception("Failed loading PhysX dynamic libraries");

#define MAP_PHYSX_FUNCTION(Library, Function) Function = libs_.Library.mapFunction<PFn##Function>(#Function)

        MAP_PHYSX_FUNCTION(PhysX3Common, PxCreateFoundation);
        MAP_PHYSX_FUNCTION(PhysX3Common, PxGetFoundation);
        if (!PxCreateFoundation || !PxGetFoundation)
            throw Exception() << "Failed mapping functions in " << libs_.PhysX3Common.getName();

        MAP_PHYSX_FUNCTION(PhysX3, PxCreateBasePhysics);
        MAP_PHYSX_FUNCTION(PhysX3, PxRegisterArticulations);
        MAP_PHYSX_FUNCTION(PhysX3, PxRegisterHeightFields);
        if (!PxCreateBasePhysics || !PxRegisterArticulations || !PxRegisterHeightFields)
            throw Exception() << "Failed mapping functions in " << libs_.PhysX3.getName();

        MAP_PHYSX_FUNCTION(PhysX3Cooking, PxCreateCooking);
        if (!PxCreateCooking)
            throw Exception() << "Failed mapping functions in " << libs_.PhysX3Cooking.getName();

        MAP_PHYSX_FUNCTION(PhysX3CharacterKinematic, PxCreateControllerManager);
        if (!PxCreateControllerManager)
            throw Exception() << "Failed mapping functions in " << libs_.PhysX3CharacterKinematic.getName();

        MAP_PHYSX_FUNCTION(PhysX3Extensions, PxCreateDynamic);
        MAP_PHYSX_FUNCTION(PhysX3Extensions, PxCreateStatic);
        MAP_PHYSX_FUNCTION(PhysX3Extensions, PxDefaultCpuDispatcherCreate);
        MAP_PHYSX_FUNCTION(PhysX3Extensions, PxDefaultSimulationFilterShader);
        MAP_PHYSX_FUNCTION(PhysX3Extensions, PxRevoluteJointCreate);
        MAP_PHYSX_FUNCTION(PhysX3Extensions, PxSphericalJointCreate);
        if (!PxCreateDynamic || !PxCreateStatic || !PxDefaultCpuDispatcherCreate || !PxDefaultSimulationFilterShader ||
            !PxRevoluteJointCreate || !PxSphericalJointCreate)
            throw Exception() << "Failed mapping functions in " << libs_.PhysX3Extensions.getName();
#endif

        // Create foundation instance
        foundation_ = PxCreateFoundation(PX_PHYSICS_VERSION, pxAllocator, pxErrorCallback);
        if (!foundation_)
            throw Exception("Failed creating PXFoundation object");

        // Create main physics API object
        physics_ = PxCreateBasePhysics(PX_PHYSICS_VERSION, *foundation_, PxTolerancesScale(), false, nullptr);
        if (!physics_)
            throw Exception("Failed creating PxPhysics object");

        PxRegisterArticulations(*physics_);
        PxRegisterHeightFields(*physics_);

        // Create cooking interface
        cooking_ = PxCreateCooking(PX_PHYSICS_VERSION, *foundation_, physics_->getTolerancesScale());
        if (!cooking_)
            throw Exception("Failed creating PxCooking object");

        cpuDispatcher_ = PxDefaultCpuDispatcherCreate(1, nullptr);

        // Create a scene
        PxSceneDesc description(physics_->getTolerancesScale());
        description.gravity = toPx(gravityVector_);
        description.filterShader = PxDefaultSimulationFilterShader;
        description.cpuDispatcher = cpuDispatcher_;
        pxScene_ = physics_->createScene(description);
        if (!pxScene_)
            throw Exception("Failed creating physics scene");

        // Create default material
        defaultMaterial_ = physics_->createMaterial(0.5f, 0.5f, 0.1f);

        // Create character controller manager
        controllerManager_ = PxCreateControllerManager(*pxScene_);

        LOG_INFO << "Initialized PhysX " << PX_PHYSICS_VERSION_MAJOR << "." << PX_PHYSICS_VERSION_MINOR << "."
                 << PX_PHYSICS_VERSION_BUGFIX;

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        shutdown();

        return false;
    }
}

void PhysX::shutdown()
{
    // Delete any remaining joints
    while (!joints_.empty())
        deleteJoint(joints_[0]);

    // Delete any remaining bodies
    while (!bodies_.empty())
        deleteBody(bodies_[0]);

    // Delete any remaining character controllers
    while (!characterControllers_.empty())
        deleteCharacterController(characterControllers_[0]);

    // Delete any remaining body templates
    while (!bodyTemplates_.empty())
        deleteBodyTemplate(bodyTemplates_[0]);

    safeRelease(defaultMaterial_);
    safeRelease(controllerManager_);
    safeRelease(pxScene_);
    safeRelease(cpuDispatcher_);
    safeRelease(cooking_);
    safeRelease(physics_);
    safeRelease(foundation_);

#ifdef CARBON_PHYSX_DYNAMIC_LIBRARY
    libs_.PhysX3.unload();
    libs_.PhysX3Common.unload();
    libs_.PhysX3Cooking.unload();
    libs_.PhysX3CharacterKinematic.unload();
    libs_.PhysX3Extensions.unload();

    PxCreateFoundation = nullptr;
    PxGetFoundation = nullptr;
    PxCreateBasePhysics = nullptr;
    PxRegisterArticulations = nullptr;
    PxRegisterHeightFields = nullptr;
    PxCreateCooking = nullptr;
    PxCreateControllerManager = nullptr;
    PxCreateDynamic = nullptr;
    PxCreateStatic = nullptr;
    PxDefaultCpuDispatcherCreate = nullptr;
    PxDefaultSimulationFilterShader = nullptr;
    PxRevoluteJointCreate = nullptr;
    PxSphericalJointCreate = nullptr;
#endif
}

String PhysX::getEngineName() const
{
    return String() << "PhysX " << PX_PHYSICS_VERSION_MAJOR << "." << PX_PHYSICS_VERSION_MINOR << "."
                    << PX_PHYSICS_VERSION_BUGFIX;
}

void PhysX::setWorldLimits(const AABB& aabb)
{
    // PhysX doesn't require world bounds, though they can be set at scene creation time
}

PhysicsInterface::BodyObject PhysX::createBody(PxShape& pxShape, const SimpleTransform& initialTransform, float mass,
                                               bool fixed, const Entity* entity, BodyTemplate* bodyTemplate)
{
    pxShape.setContactOffset(0.02f);
    pxShape.setRestOffset(0.0f);

    // Create actor
    auto pxRigidActor = pointer_to<PxRigidActor>::type();
    if (fixed)
        pxRigidActor = PxCreateStatic(*physics_, toPx(initialTransform), pxShape);
    else
        pxRigidActor = PxCreateDynamic(*physics_, toPx(initialTransform), pxShape, 1.0f);
    if (!pxRigidActor)
    {
        LOG_ERROR << "Failed creating rigid actor";
        return nullptr;
    }

    pxScene_->addActor(*pxRigidActor);

    auto body = new Body(pxRigidActor, entity, bodyTemplate);
    bodies_.append(body);

    body->transform = initialTransform;

    return body;
}

PhysicsInterface::BodyObject PhysX::createBoundingBoxBody(const AABB& aabb, float mass, bool fixed, const Entity* entity,
                                                          const SimpleTransform& initialTransform)
{
    auto pxShape =
        physics_->createShape(PxBoxGeometry(toPx((aabb.getMaximum() - aabb.getMinimum()) * 0.5f)), *defaultMaterial_, true);

    pxShape->setLocalPose(toPx(SimpleTransform(aabb.getCenter())));

    return createBody(*pxShape, initialTransform, mass, fixed, entity, nullptr);
}

PhysicsInterface::BodyObject PhysX::createCapsuleBody(float height, float radius, float mass, bool fixed, const Entity* entity,
                                                      const SimpleTransform& initialTransform)
{
    auto pxShape = physics_->createShape(PxCapsuleGeometry(radius, height * 0.5f), *defaultMaterial_, true);

    pxShape->setLocalPose(toPx(SimpleTransform(Vec3(0.0f, radius + 0.5f * height, 0.0f))));

    return createBody(*pxShape, initialTransform, mass, fixed, entity, nullptr);
}

PhysicsInterface::BodyTemplateObject PhysX::createBodyTemplateFromGeometry(const Vector<Vec3>& vertices,
                                                                           const Vector<RawIndexedTriangle>& triangles,
                                                                           bool deleteOnceUnused, float customCollisionMargin)
{
    auto data = Vector<byte_t>();
    if (!preProcessGeometry(vertices, triangles, data))
        return nullptr;

    return createBodyTemplateFromPreProcessedGeometry(data, deleteOnceUnused);
}

bool PhysX::preProcessGeometry(const Vector<Vec3>& vertices, const Vector<RawIndexedTriangle>& triangles,
                               Vector<byte_t>& output)
{
    try
    {
        if (vertices.empty() || triangles.empty())
            return false;

        auto pxTriangleMeshDesc = PxTriangleMeshDesc();

        pxTriangleMeshDesc.points.count = vertices.size();
        pxTriangleMeshDesc.points.stride = sizeof(PxVec3);
        pxTriangleMeshDesc.points.data = vertices.as<PxVec3>();

        pxTriangleMeshDesc.triangles.count = triangles.size();
        pxTriangleMeshDesc.triangles.stride = 3 * sizeof(PxU32);
        pxTriangleMeshDesc.triangles.data = triangles.as<PxU32>();

        auto outputStream = PhysXVectorOutputStream(output);
        if (!cooking_->cookTriangleMesh(pxTriangleMeshDesc, outputStream))
            throw Exception("Failed cooking triangle mesh");

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        return false;
    }
}

PhysicsInterface::BodyTemplateObject PhysX::createBodyTemplateFromPreProcessedGeometry(const Vector<byte_t>& data,
                                                                                       bool deleteOnceUnused)
{
    try
    {
        auto inputStream = PhysXVectorInputStream(data);
        auto pxTriangleMesh = physics_->createTriangleMesh(inputStream);
        if (!pxTriangleMesh)
            throw Exception("Failed creating triangle mesh");

        auto bodyTemplate = new BodyTemplate;
        bodyTemplates_.append(bodyTemplate);

        bodyTemplate->pxTriangleMesh = pxTriangleMesh;
        bodyTemplate->deleteOnceUnused = deleteOnceUnused;

        return bodyTemplate;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        return nullptr;
    }
}

PhysicsInterface::BodyTemplateObject PhysX::createBodyTemplateFromHeightmap(unsigned int heightmapWidth,
                                                                            unsigned int heightmapHeight,
                                                                            const Vector<float>& heightmap,
                                                                            bool deleteOnceUnused)
{
    try
    {
        auto sampleCount = heightmapHeight * heightmapWidth;

        auto heightFieldDesc = PxHeightFieldDesc();

        heightFieldDesc.nbRows = heightmapWidth;
        heightFieldDesc.nbColumns = heightmapHeight;

        // The height values need to be scaled into the -32767 - 32767 range in order to be packed into the 16-bit signed
        // integer format used by PhysX
        auto lowest = 0.0f;
        auto highest = 0.0f;
        Math::calculateBounds(heightmap.getData(), sampleCount, lowest, highest);

        // Calculate the height scale factor
        auto heightScaleFactor = 32767.0f / std::max(fabsf(lowest), fabsf(highest));

        // Calculate thickness, adding a little extra just to be certain
        heightFieldDesc.thickness = -(highest - lowest + 1.0f);

        // Copy height samples
        auto sampleData = Vector<PxHeightFieldSample>(sampleCount);
        for (auto y = 0U; y < heightmapHeight; y++)
        {
            for (auto x = 0U; x < heightmapWidth; x++)
                sampleData[x * heightmapHeight + y].height = short(heightmap[y * heightmapWidth + x] * heightScaleFactor);
        }
        heightFieldDesc.samples.data = sampleData.getData();
        heightFieldDesc.samples.stride = sizeof(PxHeightFieldSample);

        // Create heightfield
        auto pxHeightField = physics_->createHeightField(heightFieldDesc);
        if (!pxHeightField)
            throw Exception("Failed creating height field");

        auto bodyTemplate = new BodyTemplate;
        bodyTemplate->pxHeightField = pxHeightField;
        bodyTemplate->heightScaleFactor = heightScaleFactor;
        bodyTemplate->deleteOnceUnused = deleteOnceUnused;

        return bodyTemplate;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        return nullptr;
    }
}

unsigned int PhysX::getBodyTemplateBodyCount(BodyTemplate* bodyTemplate) const
{
    auto count = 0U;

    for (auto body : bodies_)
    {
        if (body->bodyTemplate == bodyTemplate)
            count++;
    }

    return count;
}

bool PhysX::deleteBodyTemplate(BodyTemplateObject bodyTemplateObject)
{
    if (!bodyTemplateObject)
        return false;

    auto bodyTemplate = reinterpret_cast<BodyTemplate*>(bodyTemplateObject);

    if (getBodyTemplateBodyCount(bodyTemplate))
    {
        LOG_ERROR << "Can't delete body templates that are in use by a body";
        return false;
    }

    safeRelease(bodyTemplate->pxTriangleMesh);
    safeRelease(bodyTemplate->pxHeightField);

    bodyTemplates_.unorderedEraseValue(bodyTemplate);

    delete bodyTemplate;
    bodyTemplate = nullptr;

    return true;
}

PhysicsInterface::BodyObject PhysX::createGeometryBodyFromTemplate(BodyTemplateObject bodyTemplateObject, float mass,
                                                                   bool fixed, const Entity* entity,
                                                                   const SimpleTransform& initialTransform)
{
    auto bodyTemplate = reinterpret_cast<BodyTemplate*>(bodyTemplateObject);

    if (!bodyTemplate || !bodyTemplate->pxTriangleMesh)
    {
        LOG_ERROR << "Invalid body template";
        return nullptr;
    }

    // Create shape from triangle mesh template
    auto pxShape = physics_->createShape(PxTriangleMeshGeometry(bodyTemplate->pxTriangleMesh), *defaultMaterial_, true);

    auto bodyObject = createBody(*pxShape, initialTransform, mass, fixed, entity, bodyTemplate);
    if (!bodyObject)
    {
        LOG_ERROR << "Failed creating body";
        return nullptr;
    }

    return bodyObject;
}

PhysicsInterface::BodyObject PhysX::createHeightmapBodyFromTemplate(BodyTemplateObject bodyTemplateObject, float heightScale,
                                                                    float terrainScale, float mass, bool fixed,
                                                                    const Entity* entity,
                                                                    const SimpleTransform& initialTransform)
{
    auto bodyTemplate = reinterpret_cast<BodyTemplate*>(bodyTemplateObject);

    if (!bodyTemplate || !bodyTemplate->pxHeightField)
    {
        LOG_ERROR << "Invalid body template";
        return nullptr;
    }

    auto pxShape =
        physics_->createShape(PxHeightFieldGeometry(bodyTemplate->pxHeightField, PxMeshGeometryFlags(0),
                                                    heightScale / bodyTemplate->heightScaleFactor, terrainScale, terrainScale),
                              *defaultMaterial_, true);

    auto bodyObject = createBody(*pxShape, initialTransform, mass, fixed, entity, bodyTemplate);
    if (!bodyObject)
    {
        LOG_ERROR << "Failed creating body";
        return nullptr;
    }

    return bodyObject;
}

bool PhysX::deleteBody(BodyObject bodyObject)
{
    if (!bodyObject)
        return false;

    auto body = reinterpret_cast<Body*>(bodyObject);

    // Any joints using this body are automatically removed from the simulation and any pointers to the body to be deleted are
    // nulled but the PhysX::Joint instance lives on until an actual call to PhysX::deleteJoint()
    for (auto joint : joints_)
    {
        if (joint->firstBody == body || joint->secondBody == body)
        {
            safeRelease(joint->pxJoint);

            if (joint->firstBody == body)
                joint->firstBody = nullptr;
            else if (joint->secondBody == body)
                joint->secondBody = nullptr;
        }
    }

    safeRelease(body->pxActor);

    auto bodyTemplate = body->bodyTemplate;
    if (bodyTemplate && bodyTemplate->deleteOnceUnused)
    {
        if (getBodyTemplateBodyCount(bodyTemplate) == 1)
        {
            body->bodyTemplate = nullptr;
            deleteBodyTemplate(bodyTemplate);
        }
    }

    bodies_.unorderedEraseValue(body);

    delete body;
    body = nullptr;

    return true;
}

const Entity* PhysX::getBodyEntity(BodyObject bodyObject) const
{
    return bodyObject ? reinterpret_cast<Body*>(bodyObject)->entity : nullptr;
}

bool PhysX::getBodyTransform(BodyObject bodyObject, SimpleTransform& transform)
{
    if (!bodyObject || !isUpdating())
        return false;

    auto body = reinterpret_cast<Body*>(bodyObject);

    body->transform = toCarbon(body->pxActor->getGlobalPose());

    transform = body->transform;

    // TODO: track sleeping/waking objects to know what's alive/dead and what has changed since the last call

    return true;
}

bool PhysX::setBodyTransform(BodyObject bodyObject, const SimpleTransform& transform)
{
    if (!bodyObject)
        return false;

    auto body = reinterpret_cast<Body*>(bodyObject);

    body->transform = transform;

    body->pxActor->setGlobalPose(toPx(transform));

    return true;
}

Vec3 PhysX::getBodyLinearVelocity(BodyObject bodyObject) const
{
    if (!bodyObject)
        return Vec3::Zero;

    auto pxRigidBody = reinterpret_cast<Body*>(bodyObject)->pxActor->is<PxRigidBody>();

    return pxRigidBody ? toCarbon(pxRigidBody->getLinearVelocity()) : Vec3::Zero;
}

bool PhysX::setBodyLinearVelocity(BodyObject bodyObject, const Vec3& velocity) const
{
    if (!bodyObject)
        return false;

    auto pxRigidBody = reinterpret_cast<Body*>(bodyObject)->pxActor->is<PxRigidBody>();

    if (pxRigidBody)
        pxRigidBody->setLinearVelocity(toPx(velocity));

    return pxRigidBody != nullptr;
}

Vec3 PhysX::getBodyAngularVelocity(BodyObject bodyObject) const
{
    if (!bodyObject)
        return Vec3::Zero;

    auto pxRigidBody = reinterpret_cast<Body*>(bodyObject)->pxActor->is<PxRigidBody>();

    return pxRigidBody ? toCarbon(pxRigidBody->getAngularVelocity()) : Vec3::Zero;
}

bool PhysX::setBodyAngularVelocity(BodyObject bodyObject, const Vec3& velocity) const
{
    if (!bodyObject)
        return false;

    auto pxRigidBody = reinterpret_cast<Body*>(bodyObject)->pxActor->is<PxRigidBody>();

    if (pxRigidBody)
        pxRigidBody->setAngularVelocity(toPx(velocity));

    return pxRigidBody != nullptr;
}

void PhysX::applyForceToBody(BodyObject bodyObject, const Vec3& force, ForceMode mode)
{
    if (!bodyObject)
        return;

    auto pxRigidBody = reinterpret_cast<Body*>(bodyObject)->pxActor->is<PxRigidBody>();

    if (pxRigidBody)
        pxRigidBody->addForce(toPx(force), toPx(mode));
}

void PhysX::applyTorqueToBody(BodyObject bodyObject, const Vec3& torque, ForceMode mode)
{
    if (!bodyObject)
        return;

    auto pxRigidBody = reinterpret_cast<Body*>(bodyObject)->pxActor->is<PxRigidBody>();

    if (pxRigidBody)
        pxRigidBody->addTorque(toPx(torque), toPx(mode));
}

PhysicsInterface::JointObject PhysX::createHingeJoint(BodyObject firstBodyObject, BodyObject secondBodyObject,
                                                      const Vec3& globalAnchor, const Vec3& globalAxis)
{
    if (!firstBodyObject || !secondBodyObject || firstBodyObject == secondBodyObject)
    {
        LOG_ERROR << "Invalid bodies";
        return nullptr;
    }

    auto firstBody = reinterpret_cast<Body*>(firstBodyObject);
    auto secondBody = reinterpret_cast<Body*>(secondBodyObject);

    auto jointTransform = PxTransform(toPx(globalAnchor), toPx(Quaternion::createFromVectorToVector(Vec3::UnitY, globalAxis)));

    auto pxJoint =
        PxRevoluteJointCreate(*physics_, firstBody->pxActor, jointTransform * firstBody->pxActor->getGlobalPose().getInverse(),
                              secondBody->pxActor, jointTransform * secondBody->pxActor->getGlobalPose().getInverse());
    if (!pxJoint)
    {
        LOG_ERROR << "Failed creating joint";
        return nullptr;
    }

    joints_.append(new Joint(firstBody, secondBody, pxJoint));

    return joints_.back();
}

PhysicsInterface::JointObject PhysX::createBallAndSocketJoint(BodyObject firstBodyObject, BodyObject secondBodyObject,
                                                              const Vec3& globalAnchor, const Vec3& angularLimits)
{
    if (!firstBodyObject || !secondBodyObject || firstBodyObject == secondBodyObject)
    {
        LOG_ERROR << "Invalid bodies";
        return nullptr;
    }

    auto firstBody = reinterpret_cast<Body*>(firstBodyObject);
    auto secondBody = reinterpret_cast<Body*>(secondBodyObject);

    auto jointTransform = PxTransform(toPx(globalAnchor));

    auto pxJoint =
        PxSphericalJointCreate(*physics_, firstBody->pxActor, jointTransform * firstBody->pxActor->getGlobalPose().getInverse(),
                               secondBody->pxActor, jointTransform * secondBody->pxActor->getGlobalPose().getInverse());
    if (!pxJoint)
    {
        LOG_ERROR << "Failed creating joint";
        return nullptr;
    }

    joints_.append(new Joint(firstBody, secondBody, pxJoint));

    return joints_.back();
}

bool PhysX::deleteJoint(PhysicsInterface::JointObject jointObject)
{
    if (!jointObject)
        return false;

    auto joint = reinterpret_cast<Joint*>(jointObject);

    safeRelease(joint->pxJoint);

    joints_.unorderedEraseValue(joint);

    delete joint;
    joint = nullptr;

    return true;
}

bool PhysX::getBodyJoints(BodyObject bodyObject, Vector<JointObject>& joints) const
{
    if (!bodyObject)
        return false;

    auto body = reinterpret_cast<Body*>(bodyObject);

    joints.clear();

    for (auto joint : joints_)
    {
        if (joint->firstBody == body || joint->secondBody == body)
            joints.append(joint);
    }

    return true;
}

PhysicsInterface::CharacterControllerObject PhysX::createCharacterController(float height, float radius, const Entity* entity)
{
    // Create new character controller
    auto characterController = new CharacterController;
    characterController->entity = entity;
    characterController->lastUpdateTime = platform().getTime();

    // Controller description
    auto description = PxCapsuleControllerDesc();

    description.radius = radius;
    description.height = height;
    description.climbingMode = PxCapsuleClimbingMode::eEASY;
    description.upDirection = PxVec3(0.0f, 1.0f, 0.0f);
    description.slopeLimit = cosf(Math::QuarterPi);    // Maximum slope the character can walk up
    description.contactOffset = 0.04f;
    description.density = 1.0f;
    description.stepOffset = radius;    // Maximum obstacle height the character can climb
    description.userData = characterController;

    characterController->pxController = static_cast<PxCapsuleController*>(controllerManager_->createController(description));
    if (!characterController->pxController)
    {
        LOG_ERROR << "Failed creating controller";
        deleteCharacterController(characterController);
        return nullptr;
    }

    return characterController;
}

bool PhysX::deleteCharacterController(PhysicsInterface::CharacterControllerObject characterControllerObject)
{
    if (!characterControllerObject)
        return false;

    auto characterController = reinterpret_cast<CharacterController*>(characterControllerObject);

    safeRelease(characterController->pxController);

    characterControllers_.unorderedEraseValue(characterController);

    delete characterController;
    characterController = nullptr;

    return true;
}

Vec3 PhysX::getCharacterControllerPosition(PhysicsInterface::CharacterControllerObject characterControllerObject) const
{
    if (!characterControllerObject)
        return Vec3::Zero;

    return toCarbon(reinterpret_cast<CharacterController*>(characterControllerObject)->pxController->getPosition());
}

bool PhysX::setCharacterControllerPosition(PhysicsInterface::CharacterControllerObject characterControllerObject,
                                           const Vec3& position)
{
    if (!characterControllerObject)
        return false;

    reinterpret_cast<CharacterController*>(characterControllerObject)->pxController->setPosition(toPxEx(position));

    return true;
}

void PhysX::moveCharacterController(PhysicsInterface::CharacterControllerObject characterControllerObject, const Vec3& move,
                                    float time)
{
    if (!characterControllerObject)
        return;

    auto characterController = reinterpret_cast<CharacterController*>(characterControllerObject);

    if (move.lengthSquared() > Math::Epsilon)
    {
        auto currentTime = platform().getTime();

        characterController->pxController->move(
            toPx(move), Math::Epsilon, (currentTime - characterController->lastUpdateTime).toSeconds(), PxControllerFilters());

        characterController->lastUpdateTime = currentTime;
    }
}

TimeValue PhysX::getSubstepSize() const
{
    return TimeValue(1.0f / 60.0f);
}

void PhysX::update(TimeValue time)
{
    if (isUpdating())
    {
        timeSinceLastUpdate_ += time;

        if (timeSinceLastUpdate_ > getSubstepSize())
        {
            // Run a simulation step
            pxScene_->simulate(getSubstepSize().toSeconds());
            pxScene_->fetchResults(true);

            timeSinceLastUpdate_ -= getSubstepSize();
        }
    }
}

const Vec3& PhysX::getGravityVector() const
{
    return gravityVector_;
}

void PhysX::setGravityVector(const Vec3& gravity)
{
    gravityVector_ = gravity;
    pxScene_->setGravity(toPx(gravity));
}

bool PhysX::raycast(const Ray& ray, PhysicsIntersectResult& result) const
{
    auto hit = PxRaycastBuffer();

    pxScene_->raycast(toPx(ray.getOrigin()), toPx(ray.getDirection()), 10000.0f, hit);

    if (!hit.hasBlock)
        return false;

    result = toCarbon(hit.block);

    return true;
}

}

#endif
