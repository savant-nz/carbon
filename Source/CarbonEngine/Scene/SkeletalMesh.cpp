/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Geometry/TriangleArraySet.h"
#include "CarbonEngine/Math/Interpolate.h"
#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Platform/Console.h"
#include "CarbonEngine/Platform/ConsoleCommand.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Scene/Camera.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/Mesh/Mesh.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Scene/SkeletalAnimation.h"
#include "CarbonEngine/Scene/SkeletalMesh.h"

namespace Carbon
{

const auto SkeletalMeshVersionInfo = VersionInfo(2, 3);
const auto SkeletalMeshHeaderID = FileSystem::makeFourCC("cskl");

const UnicodeString SkeletalMesh::SkeletalMeshExtension = ".skeletalmesh";
const unsigned int SkeletalMesh::MaximumBoneCount;

void SkeletalMesh::Bone::save(FileWriter& file) const
{
    file.write(name, parent, referenceRelative, inverseReferenceAbsolute);
    file.write(ragdollXConstraint, ragdollYConstraint, ragdollZConstraint);
}

void SkeletalMesh::Bone::load(FileReader& file)
{
    // Get the skeletal mesh version info that was read from the file
    auto readVersion = file.findVersionedSection(SkeletalMeshVersionInfo);

    file.read(name, parent, referenceRelative);

    // v2.0, bone absolute inverse transform when in the reference pose
    file.read(inverseReferenceAbsolute);

    // v2.2, ragdoll constraints
    if (readVersion.getMinor() >= 2)
        file.read(ragdollXConstraint, ragdollYConstraint, ragdollZConstraint);

    calculateLength();
}

void SkeletalMesh::Bone::calculateLength()
{
    length = referenceRelative.getPosition().length();
}

void SkeletalMesh::VertexWeight::normalizeWeights(Vector<VertexWeight>& weights)
{
    auto sum = 0.0f;
    for (auto& weight : weights)
        sum += weight.weight_;

    for (auto& weight : weights)
        weight.weight_ /= sum;
}

void SkeletalMesh::VertexWeight::limitWeightCount(Vector<VertexWeight>& weights, unsigned int maximumWeightCount)
{
    if (weights.size() > maximumWeightCount)
    {
        weights.sortBy([](const VertexWeight& a, const VertexWeight& b) { return a.getWeight() > b.getWeight(); });
        weights.resize(maximumWeightCount);
    }

    // Normalize so the weights sum to one
    normalizeWeights(weights);
}

typedef std::array<float, 12> Matrix4x3;

class SkeletalMesh::Members
{
public:

    const SkeletalMesh* const parent = nullptr;

    Members(const SkeletalMesh* parent_) : parent(parent_) {}

    // Bones
    Vector<Bone> bones;
    Vector<Matrix4x3> gpuBoneTransforms;    // The 4x3 row major matrix for each bone used in GPU skinning
    void updateBoneAbsoluteTransforms();
    void setReferencePose();
    void calculateBoneInverseReferenceAbsolutes();
    void calculateGPUBoneTransforms();

    // This class stores information on a skeletal animation that is currently playing on this skeletal mesh
    class ActiveAnimation
    {
    public:

        const SkeletalAnimation* animation = nullptr;
        bool loop = false;
        bool blendFromInitialBoneTransforms = false;
        bool isPaused = false;
        float currentFrame = 0.0f;

        // When blendFromInitialBoneTransforms is true the first frame of this animation becomes an interpolation from
        // the bone transforms at the time the animation was applied to the first frame of the animation itself. The
        // initial bone transforms are stored in this vector so that the animation code knows what initial transform to
        // interpolate from.
        Vector<SimpleTransform> initialBoneTransforms;

        // The SkeletalAnimation class stores bone names explicitly as strings, but converting each name to a bone index
        // for this skeletal mesh every frame would be wasteful so the bone indices for this mesh are computed once and
        // stored in the boneIndices[] array. An index of -1 means there is no bone with that name in this mesh and so
        // the animation frames for that bone are ignored when animating.
        Vector<int> boneIndices;
        void calculateBoneIndices(const Vector<Bone>& skeletalMeshBones)
        {
            // Map the bone names in the Animation class to bone indices for this skeletal mesh and store it in the
            // boneIndices vector on the ActiveAnimation instance, this saves having to work out these indices every
            // frame
            boneIndices = Vector<int>(animation->getBoneAnimations().size(), -1);
            for (auto i = 0U; i < animation->getBoneAnimations().size(); i++)
            {
                auto& ba = animation->getBoneAnimations()[i];

                for (auto j = 0U; j < skeletalMeshBones.size(); j++)
                {
                    if (skeletalMeshBones[j].name == ba.boneName)
                    {
                        boneIndices[i] = j;
                        break;
                    }
                }
            }
        }

        ActiveAnimation() {}
        ActiveAnimation(const SkeletalAnimation* animation_, bool loop_, bool blendFromInitialBoneTransforms_)
            : animation(animation_), loop(loop_), blendFromInitialBoneTransforms(blendFromInitialBoneTransforms_)
        {
        }

        // Restricts the currentFrame value to the range 0-frameCount
        void normalizeCurrentFrame()
        {
            auto frameCount = float(animation->getFrameCount());

            while (currentFrame < 0.0f)
                currentFrame += frameCount;

            while (uint(currentFrame) >= frameCount)
                currentFrame -= frameCount;
        }
    };
    Vector<ActiveAnimation> activeAnimations;    // All animations currently active on this skeletal mesh

    float animationSpeedScale = 1.0f;

    // The geometry is divided up into submeshes, each with a different material
    class SubMesh
    {
    public:

        String material;
        GeometryChunk geometryChunk;    // The reference pose data

        bool isShadowCaster = false;

        // This chunk is where CPU skinning results are put. CPU skinning is done on-demand in order to get accurate
        // intersection testing against skeletal meshes. If GPU skinning is disabled then full skinning is done by the
        // CPU directly into this chunk and it is then used for rendering.
        GeometryChunk animationGeometryChunk;

        // The current maximum weight count used by a vertex in this submesh. The maximum allowed value is 4 weights per
        // vertex, but some animation data will use fewer weights per vertex than this, and this information can be used
        // by the renderer to improve efficiency by reducing the amount of per-vertex math.
        unsigned int weightsPerVertex = 4;

        // Works out the correct value for SubMesh::weightsPerVertex based on the contents of this submesh's geometry
        // chunk
        void calculateWeightsPerVertex();

        // The bone indices in this submesh index into this vector, which maps to an absolute bone index in the main
        // bones array
        Vector<unsigned int> localBoneIndexToAbsoluteBoneIndex;

        Matrix4x3 gpuBoneTransforms[MaximumMaterialBoneCount];

        ParameterArray internalParams;

        void save(FileWriter& file) const
        {
            file.write(material, geometryChunk, localBoneIndexToAbsoluteBoneIndex, weightsPerVertex);
        }

        void load(FileReader& file)
        {
            auto readVersion = file.findVersionedSection(SkeletalMeshVersionInfo);

            file.read(material, geometryChunk);

            // v2.1
            if (readVersion.getMinor() >= 1)
                file.read(localBoneIndexToAbsoluteBoneIndex);

            // v2.3, precalculate weights-per-vertex
            if (readVersion.getMinor() >= 3)
                file.read(weightsPerVertex);
            else
                calculateWeightsPerVertex();
        }

        // The name of the original skeletal mesh component which this submesh was loaded from. This is set in
        // addSkeletalMesh() and is used in removeSkeletalMesh() to be able to remove the submeshes associated with a
        // specific skeletal mesh component
        String skeletalMeshComponent;

        // Creates the localBoneIndexToAbsoluteBoneIndex array and updates all the bone indices in the reference pose
        // data appropriately so they index this array rather than the master bones array
        bool createLocalBoneIndexMap();

        // When bones are removed the indices stored in the submesh geometry chunks need to be updated to reflect any
        // change to bone indices
        void updateBoneIndices(const Vector<unsigned int>& boneIndexMap);
    };
    Vector<SubMesh*> submeshes;

    unsigned int maximumAllowedWeightsPerVertex = 4;

    // Adds a set of submeshes and bones that are part of a new skeletal component
    bool addSkeletalMeshComponent(const Vector<Bone>& newBones, Vector<SubMesh*>& newSubmeshes,
                                  const String& component);

    // Removes all submeshes that are part of the specified skeletal mesh component
    bool removeSkeletalMeshComponent(const String& component);

    // Removes bones not in use by any submesh
    void removeUnreferencedBones();

    bool isGPUSkinningEnabled = true;
    bool isCPUAnimationCurrent = false;
    bool areCPUAnimatedVertexPositionsCurrent = false;
    void skinVertices();           // Full CPU skinning of vertex positions, normals and tangents
    void skinVertexPositions();    // CPU skinning of vertex positions only

    // Geometry chunk used to draw the skeleton for debugging purposes
    bool isDrawSkeletonEnabled = false;
    GeometryChunk skeletonGeometryChunk;
    void updateSkeletonGeometryChunk();

    // Local bounding volumes around each bone in the reference pose
    void calculateBoneAABBs();

    void clear();

    // The root bone being used to define entity transforms
    int rootBoneToFollow = -1;
    SimpleTransform lastRootBoneToFollowTransform;

    // Ragdoll bodies and joints
    Vector<PhysicsInterface::BodyObject> ragdollBodies;
    Vector<PhysicsInterface::JointObject> ragdollJoints;
    void calculateBoneRagdollReferenceOrientationAbsoluteInverses();
    bool createRagdoll(float boneMass, bool fixed);
    void destroyRagdoll();
    void alignBonesToRagdollBodies();
};

void SkeletalMesh::Members::updateBoneAbsoluteTransforms()
{
    for (auto& bone : bones)
    {
        if (bone.parent == -1)
            bone.currentAbsolute = bone.currentRelative;
        else
            bone.currentAbsolute = bones[bone.parent].currentAbsolute * bone.currentRelative;

        bone.currentCombinedTransform = bone.currentAbsolute * bone.inverseReferenceAbsolute;
    }
}

void SkeletalMesh::Members::setReferencePose()
{
    for (auto& bone : bones)
        bone.currentRelative = bone.referenceRelative;

    updateBoneAbsoluteTransforms();
    isCPUAnimationCurrent = false;
    areCPUAnimatedVertexPositionsCurrent = false;
}

void SkeletalMesh::Members::calculateBoneInverseReferenceAbsolutes()
{
    setReferencePose();

    for (auto& bone : bones)
        bone.inverseReferenceAbsolute = bone.currentAbsolute.getInverse();
}

void SkeletalMesh::Members::calculateBoneRagdollReferenceOrientationAbsoluteInverses()
{
    // Store the current relative transform for all the bones so it can be restored later
    auto previousCurrentRelativeBoneTransforms = Vector<SimpleTransform>();
    previousCurrentRelativeBoneTransforms.reserve(bones.size());

    for (const auto& bone : bones)
        previousCurrentRelativeBoneTransforms.append(bone.currentRelative);

    // All following calculations are done in the reference pose
    setReferencePose();

    for (auto& bone : bones)
    {
        // Construct a basis for this bone in parent space that is used to orient ragdoll bodies to the skeleton. The Y
        // axis is made to point along the bone because the capsules created in the physical layer use Y as their major
        // axis.

        auto& parentAbsolutePosition =
            (bone.parent == -1) ? Vec3::Zero : bones[bone.parent].currentAbsolute.getPosition();
        auto target = bone.currentAbsolute.getPosition() - parentAbsolutePosition;

        auto referenceOrientationAbsolute = Quaternion::createFromVectorToVector(Vec3::UnitY, target);

        bone.ragdollReferenceOrientationAbsoluteInverse = referenceOrientationAbsolute.getInverse();
    }

    // Restore the original relative bone transforms
    for (auto i = 0U; i < bones.size(); i++)
        bones[i].currentRelative = previousCurrentRelativeBoneTransforms[i];
    updateBoneAbsoluteTransforms();
}

void SkeletalMesh::Members::calculateGPUBoneTransforms()
{
    gpuBoneTransforms.resize(bones.size());

    // Calculate all the bone transforms that the shader will need to do animation
    for (auto i = 0U; i < bones.size(); i++)
    {
        // Get the 4x4 for this bone's combined animation-ready transform
        auto boneMatrix = bones[i].currentCombinedTransform.getMatrix();

        // Copy bone matrix into m->gpuBoneTransforms (4x3) ready for shaders to use
        gpuBoneTransforms[i][0] = boneMatrix[0];
        gpuBoneTransforms[i][1] = boneMatrix[4];
        gpuBoneTransforms[i][2] = boneMatrix[8];
        gpuBoneTransforms[i][3] = boneMatrix[12];
        gpuBoneTransforms[i][4] = boneMatrix[1];
        gpuBoneTransforms[i][5] = boneMatrix[5];
        gpuBoneTransforms[i][6] = boneMatrix[9];
        gpuBoneTransforms[i][7] = boneMatrix[13];
        gpuBoneTransforms[i][8] = boneMatrix[2];
        gpuBoneTransforms[i][9] = boneMatrix[6];
        gpuBoneTransforms[i][10] = boneMatrix[10];
        gpuBoneTransforms[i][11] = boneMatrix[14];
    }

    for (auto submesh : submeshes)
    {
        for (auto j = 0U; j < submesh->localBoneIndexToAbsoluteBoneIndex.size(); j++)
            submesh->gpuBoneTransforms[j] = gpuBoneTransforms[submesh->localBoneIndexToAbsoluteBoneIndex[j]];
    }
}

bool SkeletalMesh::Members::addSkeletalMeshComponent(const Vector<Bone>& newBones, Vector<SubMesh*>& newSubmeshes,
                                                     const String& component)
{
    if (bones.size())
    {
        // Append new bones onto the existing bones list and create a mapping from the newBones[] index to the new index
        // in bones[]. Bones in newBones[] that match an already existing bone in bones[] will be mapped to that bone.
        auto boneIndexMap = Vector<unsigned int>();
        for (const auto& newBone : newBones)
        {
            auto index = -1;
            for (auto j = 0U; j < bones.size(); j++)
            {
                if (bones[j].name != newBone.name)
                    continue;

                // Check that the bones have the same reference relative transform
                if (bones[j].referenceRelative.getPosition().distance(newBone.referenceRelative.getPosition()) >
                    Math::Epsilon)
                {
                    LOG_WARNING << "Bone '" << bones[j].name
                                << "': the reference poses do not match, this may result in "
                                   "incorrect rendering of one or more skeletal mesh components";
                }

                index = j;
                break;
            }

            if (index != -1)
                boneIndexMap.append(index);
            else
            {
                // New bone
                bones.append(newBone);
                bones.back().parent = newBone.parent == -1 ? -1 : int(boneIndexMap[newBone.parent]);
                bones.back().currentRelative = bones.back().referenceRelative;    // Start in the reference pose
                boneIndexMap.append(bones.size() - 1);
            }
        }

        for (auto i = 0U; i < boneIndexMap.size(); i++)
        {
            if (i != boneIndexMap[i])
            {
                // A bone index has changed, so update the indices in the submeshes
                for (auto newSubmesh : newSubmeshes)
                    newSubmesh->updateBoneIndices(boneIndexMap);

                break;
            }
        }

        updateBoneAbsoluteTransforms();
    }
    else
    {
        bones = newBones;
        setReferencePose();
    }

    // Check that the submeshes have a local bone index mapping set up
    for (auto newSubmesh : newSubmeshes)
    {
        if (newSubmesh->localBoneIndexToAbsoluteBoneIndex.empty())
        {
            if (!newSubmesh->createLocalBoneIndexMap())
            {
                LOG_ERROR << "Submesh with material '" << newSubmesh->material
                          << "' exceeds the maximum number of supported bones";

                removeUnreferencedBones();
                return false;
            }
        }
    }

    calculateBoneRagdollReferenceOrientationAbsoluteInverses();

    // Take ownership of the submeshes
    for (auto newSubmesh : newSubmeshes)
    {
        newSubmesh->skeletalMeshComponent = component;
        submeshes.append(newSubmesh);
    }
    newSubmeshes.clear();

    isCPUAnimationCurrent = false;
    areCPUAnimatedVertexPositionsCurrent = false;

    // Update bone indices for any active animations
    for (auto& activeAnimation : activeAnimations)
        activeAnimation.calculateBoneIndices(bones);

    return true;
}

bool SkeletalMesh::Members::removeSkeletalMeshComponent(const String& component)
{
    auto foundSubmesh = false;

    // Free the submeshes that use this component
    for (auto i = 0U; i < submeshes.size(); i++)
    {
        if (submeshes[i]->skeletalMeshComponent == component)
        {
            delete submeshes[i];
            submeshes.erase(i--);
            foundSubmesh = true;
        }
    }

    // Exit if no submeshes were found for the specified component
    if (!foundSubmesh)
        return false;

    // Condense the bone list following the removal of submeshes
    removeUnreferencedBones();

    return true;
}

void SkeletalMesh::Members::removeUnreferencedBones()
{
    // For each bone determine whether it is directly referenced by any submesh, that is, there is a submesh containing
    // a vertex that is weighted at least partially to the bone
    auto isBoneReferenced = Vector<bool>(bones.size());
    for (auto submesh : submeshes)
    {
        const auto& geometryChunk = submesh->geometryChunk;
        auto itReferenceBones = geometryChunk.getVertexStreamConstIterator<uint8_t>(VertexStream::Bones);
        for (auto j = 0U; j < geometryChunk.getVertexCount(); j++)
        {
            for (auto k = 0U; k < 4; k++)
                isBoneReferenced[submesh->localBoneIndexToAbsoluteBoneIndex[itReferenceBones[k]]] = true;

            itReferenceBones++;
        }
    }

    // Make sure all parent bones of referenced bones are also flagged as referenced. Going backwards through the array
    // ensures no bones get missed out because parent bones always precede their child bones in the list
    for (auto i = int(isBoneReferenced.size()) - 1; i >= 0; i--)
    {
        if (isBoneReferenced[i] && bones[i].parent != -1)
        {
            assert(bones[i].parent < i && "Parent bones must always precede any child bones in the bone list");
            isBoneReferenced[bones[i].parent] = true;
        }
    }

    // If all bones are referenced then there is nothing to update
    if (!isBoneReferenced.has(false))
        return;

    // Construct a new bone list that excludes the now unreferenced bones, as well as a mapping from old bone indices to
    // the corresponding index in the new bones list, this mapping is then used to update the bone indices in the
    // submeshes
    auto newBones = Vector<Bone>();
    auto boneIndexMap = Vector<unsigned int>(bones.size());
    for (auto i = 0U; i < bones.size(); i++)
    {
        if (isBoneReferenced[i])
        {
            newBones.append(bones[i]);
            boneIndexMap[i] = newBones.size() - 1;

            // Update the parent index as well
            if (bones[i].parent != -1)
                newBones.back().parent = boneIndexMap[bones[i].parent];
        }
    }

    bones = newBones;

    // Update all bone indices in the submeshes to correctly reference the new bone list
    for (auto submesh : submeshes)
        submesh->updateBoneIndices(boneIndexMap);

    // Update bone indices for any active animations
    for (auto& activeAnimation : activeAnimations)
        activeAnimation.calculateBoneIndices(bones);
}

bool SkeletalMesh::Members::SubMesh::createLocalBoneIndexMap()
{
    localBoneIndexToAbsoluteBoneIndex.clear();

    auto absoluteBoneIndexToSubmeshBoneIndex = Vector<int>();

    geometryChunk.lockVertexData();
    auto itBones = geometryChunk.getVertexStreamIterator<uint8_t>(VertexStream::Bones);

    // Iterate over every bone index in this submesh's geometry chunk
    for (auto i = 0U; i < geometryChunk.getVertexCount(); i++, itBones++)
    {
        for (auto j = 0U; j < 4; j++)
        {
            auto boneIndex = itBones[j];

            if (boneIndex >= absoluteBoneIndexToSubmeshBoneIndex.size())
                absoluteBoneIndexToSubmeshBoneIndex.resize(boneIndex + 1, -1);

            if (absoluteBoneIndexToSubmeshBoneIndex[boneIndex] == -1)
            {
                localBoneIndexToAbsoluteBoneIndex.append(boneIndex);
                absoluteBoneIndexToSubmeshBoneIndex[boneIndex] = localBoneIndexToAbsoluteBoneIndex.size() - 1;
            }

            itBones[j] = uint8_t(absoluteBoneIndexToSubmeshBoneIndex[boneIndex]);
        }
    }

    geometryChunk.unlockVertexData();

    return localBoneIndexToAbsoluteBoneIndex.size() < MaximumMaterialBoneCount;
}

void SkeletalMesh::Members::SubMesh::updateBoneIndices(const Vector<unsigned int>& boneIndexMap)
{
    for (auto& index : localBoneIndexToAbsoluteBoneIndex)
        index = boneIndexMap[index];
}

void SkeletalMesh::Members::SubMesh::calculateWeightsPerVertex()
{
    auto itWeights = geometryChunk.getVertexStreamConstIterator<float>(VertexStream::Weights);

    // Reset weight count
    weightsPerVertex = 1;

    // Loop over vertices
    for (auto i = 0U; i < geometryChunk.getVertexCount(); i++, itWeights++)
    {
        // Loop over weights
        for (auto j = 0U; j < 4; j++)
        {
            if (itWeights[j] != 0.0f)
                weightsPerVertex = std::max(j + 1, weightsPerVertex);
        }

        if (weightsPerVertex == 4)
            break;
    }
}

void SkeletalMesh::Members::skinVertices()
{
    if (isCPUAnimationCurrent)
        return;

    for (auto submesh : submeshes)
    {
        // Vertex streams in the reference pose geometry data needed to do animation
        const auto& referenceGeometryChunk = submesh->geometryChunk;
        auto itReferencePosition = referenceGeometryChunk.getVertexStreamConstIterator<Vec3>(VertexStream::Position);
        auto itReferenceBones = referenceGeometryChunk.getVertexStreamConstIterator<uint8_t>(VertexStream::Bones);
        auto itReferenceWeights = referenceGeometryChunk.getVertexStreamConstIterator<float>(VertexStream::Weights);
        auto itReferenceTangent = referenceGeometryChunk.getVertexStreamConstIterator<Vec3>(VertexStream::Tangent);
        auto itReferenceBitangent = referenceGeometryChunk.getVertexStreamConstIterator<Vec3>(VertexStream::Bitangent);
        auto itReferenceNormal = referenceGeometryChunk.getVertexStreamConstIterator<Vec3>(VertexStream::Normal);

        // Vertex streams in the animated geometry data needed to do animation
        submesh->animationGeometryChunk.lockVertexData();
        auto itAnimatedPosition = submesh->animationGeometryChunk.getVertexStreamIterator<Vec3>(VertexStream::Position);
        auto itAnimatedTangent = submesh->animationGeometryChunk.getVertexStreamIterator<Vec3>(VertexStream::Tangent);
        auto itAnimatedBitangent =
            submesh->animationGeometryChunk.getVertexStreamIterator<Vec3>(VertexStream::Bitangent);
        auto itAnimatedNormal = submesh->animationGeometryChunk.getVertexStreamIterator<Vec3>(VertexStream::Normal);

        // Animate each vertex
        for (auto i = 0U; i < referenceGeometryChunk.getVertexCount(); i++)
        {
            // Clear the values that will be filled with animated data
            *itAnimatedPosition = Vec3::Zero;
            *itAnimatedTangent = Vec3::Zero;
            *itAnimatedBitangent = Vec3::Zero;
            *itAnimatedNormal = Vec3::Zero;

            for (auto j = 0U; j < submesh->weightsPerVertex; j++)
            {
                auto weight = itReferenceWeights[j];

                auto boneIndex = submesh->localBoneIndexToAbsoluteBoneIndex[itReferenceBones[j]];
                auto& boneTransform = bones[boneIndex].currentCombinedTransform;

                *itAnimatedPosition += (boneTransform * *itReferencePosition) * weight;
                *itAnimatedTangent += (boneTransform.getOrientation() * *itReferenceTangent) * weight;
                *itAnimatedBitangent += (boneTransform.getOrientation() * *itReferenceBitangent) * weight;
                *itAnimatedNormal += (boneTransform.getOrientation() * *itReferenceNormal) * weight;
            }

            // Normalize animated tangent basis vectors
            (*itAnimatedTangent).normalize();
            (*itAnimatedBitangent).normalize();
            (*itAnimatedNormal).normalize();

            // Move to next animated vertex
            *itAnimatedPosition++;
            *itAnimatedTangent++;
            *itAnimatedBitangent++;
            *itAnimatedNormal++;

            // Move to next reference pose vertex
            *itReferencePosition++;
            *itReferenceBones++;
            *itReferenceWeights++;
            *itReferenceTangent++;
            *itReferenceBitangent++;
            *itReferenceNormal++;
        }

        submesh->animationGeometryChunk.unlockVertexData();
    }

    isCPUAnimationCurrent = true;
    areCPUAnimatedVertexPositionsCurrent = true;
}

void SkeletalMesh::Members::skinVertexPositions()
{
    if (areCPUAnimatedVertexPositionsCurrent)
        return;

    // This method is identical to the one above except it only skins the vertex positions. Normals and tangents are not
    // touched. This is used to skin the model for CPU calculation of intersections when GPU skinning is being used for
    // rendering, so only the vertex positions need to be known. When doing all skinning on the CPU this method will
    // never be called.

    for (auto submesh : submeshes)
    {
        // Vertex streams in the reference pose geometry data needed to do animation
        const auto& referenceGeometryChunk = submesh->geometryChunk;
        auto itReferencePosition = referenceGeometryChunk.getVertexStreamConstIterator<Vec3>(VertexStream::Position);
        auto itReferenceBones = referenceGeometryChunk.getVertexStreamConstIterator<uint8_t>(VertexStream::Bones);
        auto itReferenceWeights = referenceGeometryChunk.getVertexStreamConstIterator<float>(VertexStream::Weights);

        // Vertex streams in the animated geometry data needed to do animation
        submesh->animationGeometryChunk.lockVertexData();
        auto itAnimatedPosition = submesh->animationGeometryChunk.getVertexStreamIterator<Vec3>(VertexStream::Position);

        // Animate each vertex
        for (auto i = 0U; i < referenceGeometryChunk.getVertexCount(); i++)
        {
            // Clear the values that will be filled with animated data
            *itAnimatedPosition = Vec3::Zero;

            for (auto j = 0U; j < submesh->weightsPerVertex; j++)
            {
                auto weight = itReferenceWeights[j];

                auto boneIndex = submesh->localBoneIndexToAbsoluteBoneIndex[itReferenceBones[j]];
                auto& boneTransform = bones[boneIndex].currentCombinedTransform;

                *itAnimatedPosition += (boneTransform * *itReferencePosition) * weight;
            }

            // Move to next animated vertex
            itAnimatedPosition++;

            // Move to next reference pose vertex
            itReferencePosition++;
            itReferenceBones++;
            itReferenceWeights++;
        }

        submesh->animationGeometryChunk.unlockVertexData();
    }

    areCPUAnimatedVertexPositionsCurrent = true;
}

void SkeletalMesh::Members::updateSkeletonGeometryChunk()
{
    // Setup the geometry chunk if not already
    if (skeletonGeometryChunk.getVertexStreams().empty())
    {
        // Add position, color and texture coordinate vertex streams
        skeletonGeometryChunk.addVertexStream({VertexStream::Position, 3});
        skeletonGeometryChunk.addVertexStream({VertexStream::Color, 4, TypeUInt8});
        skeletonGeometryChunk.addVertexStream({VertexStream::DiffuseTextureCoordinate, 2});
        skeletonGeometryChunk.setVertexCount(bones.size() * 2);

        // Single draw item of individual lines
        auto drawItems = Vector<DrawItem>{{GraphicsInterface::LineList, bones.size() * 2, 0}};

        // Simple index buffer
        auto indices = Vector<unsigned int>(bones.size() * 2);
        for (auto i = 0U; i < indices.size(); i++)
            indices[i] = i;

        skeletonGeometryChunk.setupIndexData(drawItems, indices);
        skeletonGeometryChunk.setDynamic(true);
        skeletonGeometryChunk.registerWithRenderer();
    }

    if (bones.empty())
        return;

    // Create iterators for the bone vertices
    skeletonGeometryChunk.lockVertexData();
    auto itAnimatedPosition = skeletonGeometryChunk.getVertexStreamIterator<Vec3>(VertexStream::Position);
    auto animatedColor = skeletonGeometryChunk.getVertexStreamIterator<uint8_t>(VertexStream::Color);

    for (const auto& bone : bones)
    {
        // First vertex for this bone
        animatedColor[0] = 255;
        animatedColor[3] = 255;
        if (bone.parent == -1)
            *itAnimatedPosition = Vec3::Zero;
        else
            *itAnimatedPosition = bones[bone.parent].currentAbsolute.getPosition();

        itAnimatedPosition++;
        animatedColor++;

        // Second vertex for this bone
        animatedColor[1] = 255;
        animatedColor[2] = 255;
        animatedColor[3] = 255;
        *itAnimatedPosition = bone.currentAbsolute.getPosition();

        itAnimatedPosition++;
        animatedColor++;
    }

    skeletonGeometryChunk.unlockVertexData();
}

void SkeletalMesh::Members::calculateBoneAABBs()
{
    // Calculate boundings for each of the bones in this skeletal mesh
    for (auto& bone : bones)
        bone.aabb = AABB();

    for (auto submesh : submeshes)
    {
        auto itReferencePosition = submesh->geometryChunk.getVertexStreamConstIterator<Vec3>(VertexStream::Position);
        auto itReferenceBones = submesh->geometryChunk.getVertexStreamConstIterator<uint8_t>(VertexStream::Bones);
        auto itReferenceWeights = submesh->geometryChunk.getVertexStreamConstIterator<float>(VertexStream::Weights);

        for (auto j = 0U; j < submesh->geometryChunk.getVertexCount(); j++)
        {
            for (auto k = 0U; k < 4; k++)
            {
                if (itReferenceWeights[k] > 0.05f)
                {
                    bones[submesh->localBoneIndexToAbsoluteBoneIndex[itReferenceBones[k]]].aabb.addPoint(
                        *itReferencePosition);
                }
            }

            itReferencePosition++;
            itReferenceBones++;
            itReferenceWeights++;
        }
    }
}

void SkeletalMesh::Members::clear()
{
    bones.clear();
    gpuBoneTransforms.clear();
    activeAnimations.clear();
    animationSpeedScale = 1.0f;

    for (auto submesh : submeshes)
        delete submesh;
    submeshes.clear();

    maximumAllowedWeightsPerVertex = 4;

    isCPUAnimationCurrent = false;
    areCPUAnimatedVertexPositionsCurrent = false;
    skeletonGeometryChunk.clear();
    rootBoneToFollow = -1;
    lastRootBoneToFollowTransform = SimpleTransform::Identity;
}

bool SkeletalMesh::Members::createRagdoll(float boneMass, bool fixed)
{
    destroyRagdoll();

    updateBoneAbsoluteTransforms();

    ragdollBodies.resize(bones.size());
    ragdollJoints.resize(bones.size());

    const auto size = 0.1f;

    for (auto i = 0U; i < bones.size(); i++)
    {
        auto& bone = bones[i];

        if (!bone.isRagdollBone)
            continue;

        if (bone.parent == -1)
        {
            // Create and position a ragdoll body for this root bone
            ragdollBodies[i] = physics().createBoundingBoxBody(
                AABB(Vec3(-size), Vec3(size)), boneMass, fixed, nullptr,
                parent->localToWorld(
                    {bone.currentAbsolute.getPosition(), bone.ragdollReferenceOrientationAbsoluteInverse.getInverse() *
                         bone.currentAbsolute.getOrientation()}));
        }
        else
        {
            if (!ragdollBodies[bone.parent])
                continue;

            auto& parentBone = bones[bone.parent];

            auto physicalLength = std::max(bone.length - size * 3.0f, 0.2f);

            // Create a ragdoll body for this bone
            ragdollBodies[i] = physics().createCapsuleBody(
                physicalLength, size, boneMass, false, nullptr,
                parent->localToWorld(
                    {bone.currentAbsolute.getPosition(), bone.ragdollReferenceOrientationAbsoluteInverse.getInverse() *
                         bone.currentAbsolute.getOrientation()}));

            if (ragdollBodies[i])
            {
                // Create a ball and socket joint connecting the two ragdoll bones
                ragdollJoints[i] =
                    physics().createBallAndSocketJoint(ragdollBodies[bone.parent], ragdollBodies[i],
                                                       parent->localToWorld(parentBone.currentAbsolute.getPosition()));
            }
        }

        if (!ragdollBodies[i])
        {
            LOG_ERROR << "Failed creating ragdoll body for bone " << bone.name << " (" << i << ")";
            destroyRagdoll();
            return false;
        }
    }

    return true;
}

void SkeletalMesh::Members::destroyRagdoll()
{
    // Delete all ragdoll joints and bodies

    for (auto ragdollJoint : ragdollJoints)
        physics().deleteJoint(ragdollJoint);

    for (auto ragdollBody : ragdollBodies)
        physics().deleteBody(ragdollBody);

    ragdollJoints.clear();
    ragdollBodies.clear();
}

void SkeletalMesh::Members::alignBonesToRagdollBodies()
{
    for (auto i = 0U; i < bones.size(); i++)
    {
        auto& bone = bones[i];

        // Get ragdoll body transform
        auto ragdollTransform = SimpleTransform();
        if (!physics().getBodyTransform(ragdollBodies[i], ragdollTransform))
            continue;

        bone.currentAbsolute.setPosition(parent->worldToLocal(ragdollTransform.getPosition()));
        bone.currentAbsolute.setOrientation(bone.ragdollReferenceOrientationAbsoluteInverse *
                                            parent->worldToLocal(ragdollTransform.getOrientation()));

        if (bone.parent == -1)
            bone.currentRelative = bone.currentAbsolute;
        else
            bone.currentRelative = bones[bone.parent].currentAbsolute.getInverse() * bone.currentAbsolute;
    }

    isCPUAnimationCurrent = false;
    areCPUAnimatedVertexPositionsCurrent = false;
}

SkeletalMesh::SkeletalMesh()
{
    m = new Members(this);

    clear();
}

SkeletalMesh::~SkeletalMesh()
{
    onDestruct();
    clear();

    delete m;
    m = nullptr;
}

void SkeletalMesh::clear()
{
    setDrawSkeletonEnabled(false);

    m->clear();
    setGPUSkinningEnabled(graphics().isShaderLanguageSupported(ShaderProgram::GLSL110));

    ComplexEntity::clear();
}

void SkeletalMesh::save(FileWriter& file) const
{
    ComplexEntity::save(file);

    // TODO: save skeletal mesh state

    LOG_WARNING << "Not implemented";
}

void SkeletalMesh::load(FileReader& file)
{
    try
    {
        clear();

        ComplexEntity::load(file);

        // TODO: load skeletal mesh state

        LOG_WARNING << "Not implemented";
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

void SkeletalMesh::setReferencePose()
{
    m->setReferencePose();

    // Invalidate the world transforms of child entities that are attached to bones in this skeletal mesh
    if (getScene())
    {
        for (auto j = 0U; j < getChildCount(); j++)
            getChild(j)->invalidateWorldTransform();
    }

    onLocalAABBChanged();
}

bool SkeletalMesh::addAnimation(const String& name, bool loop, bool blendFromInitialBoneTransforms)
{
    auto animation = SkeletalAnimation::get(name);
    if (!animation->isLoaded())
        return false;

    // Check this animation isn't already active on this skeletal mesh
    for (auto& activeAnimation : m->activeAnimations)
    {
        if (activeAnimation.animation == animation)
            return false;
    }

    m->activeAnimations.emplace(animation, loop, blendFromInitialBoneTransforms);
    m->activeAnimations.back().calculateBoneIndices(m->bones);

    // Store the initial bone transforms if they will be needed when animating the first frame of this animation
    if (blendFromInitialBoneTransforms)
    {
        m->activeAnimations.back().initialBoneTransforms.resize(getBoneCount());
        for (auto i = 0U; i < m->bones.size(); i++)
            m->activeAnimations.back().initialBoneTransforms[i] = m->bones[i].currentRelative;
    }

    // Check bone lengths are the same in the animation compared to the current skeleton, if they aren't this will cause
    // animation errors
    for (auto i = 0U; i < animation->getBoneAnimations().size(); i++)
    {
        auto boneIndex = m->activeAnimations.back().boneIndices[i];
        if (boneIndex == -1)
            continue;

        if (m->bones[boneIndex].parent == -1)
            continue;

        auto skeletonBoneLength = m->bones[boneIndex].length;
        auto animationBoneLength = animation->getBoneAnimations()[i].frames[0].getPosition().length();

        if (fabsf(skeletonBoneLength - animationBoneLength) > 0.05f)
        {
            LOG_WARNING
                << "Length of bone '" << m->bones[boneIndex].name << "' in animation '" << name << "' doesn't match "
                << "the length in the skeleton, got " << animationBoneLength << " but expected " << skeletonBoneLength;
        }
    }

    return true;
}

void SkeletalMesh::removeAnimation(const String& name)
{
    m->activeAnimations.eraseIf([&](const Members::ActiveAnimation& a) { return a.animation->getName() == name; });
}

void SkeletalMesh::removeAllAnimations()
{
    m->activeAnimations.clear();
}

bool SkeletalMesh::setAnimation(const String& name, bool loop, bool blendFromInitialBoneTransforms)
{
    removeAllAnimations();
    return addAnimation(name, loop, blendFromInitialBoneTransforms);
}

Vector<String> SkeletalMesh::getAnimations() const
{
    return m->activeAnimations.map<String>([](const Members::ActiveAnimation& a) { return a.animation->getName(); });
}

float SkeletalMesh::getAnimationCurrentFrame(const String& name) const
{
    for (auto& a : m->activeAnimations)
    {
        if (a.animation->getName() == name)
            return a.currentFrame;
    }

    return 0.0f;
}

bool SkeletalMesh::setAnimationCurrentFrame(const String& name, float frame)
{
    for (auto& a : m->activeAnimations)
    {
        if (a.animation->getName() == name)
        {
            a.currentFrame = frame;
            a.normalizeCurrentFrame();

            m->isCPUAnimationCurrent = false;
            m->areCPUAnimatedVertexPositionsCurrent = false;

            return true;
        }
    }

    return false;
}

bool SkeletalMesh::isAnimationPaused(const String& name) const
{
    for (auto& a : m->activeAnimations)
    {
        if (a.animation->getName() == name)
            return a.isPaused;
    }

    return false;
}

bool SkeletalMesh::setAnimationPaused(const String& name, bool paused)
{
    for (auto& a : m->activeAnimations)
    {
        if (a.animation->getName() == name)
        {
            a.isPaused = paused;
            return true;
        }
    }

    return false;
}

float SkeletalMesh::getAnimationSpeedScale() const
{
    return m->animationSpeedScale;
}

void SkeletalMesh::setAnimationSpeedScale(float scale)
{
    m->animationSpeedScale = scale;
}

bool SkeletalMesh::getAttachmentPointLocalTransform(const String& name, SimpleTransform& transform) const
{
    if (name.length())
    {
        for (auto& bone : m->bones)
        {
            if (bone.name == name)
            {
                transform.setPosition(bone.currentAbsolute.getPosition() * getMeshScale());
                transform.setOrientation(bone.currentAbsolute.getOrientation());

                return true;
            }
        }
    }

    return ComplexEntity::getAttachmentPointLocalTransform(name, transform);
}

void SkeletalMesh::getAttachmentPointNames(Vector<String>& names, const String& requiredPrefix) const
{
    ComplexEntity::getAttachmentPointNames(names, requiredPrefix);

    for (auto& bone : m->bones)
    {
        if (bone.name.startsWith(requiredPrefix))
            names.append(bone.name);
    }
}

bool SkeletalMesh::saveSkeletalMesh(const UnicodeString& name) const
{
    try
    {
        auto filename = name;
        if (!filename.startsWith(FileSystem::LocalFilePrefix))
            filename = Mesh::MeshDirectory + filename + SkeletalMeshExtension;

        auto file = FileWriter();
        fileSystem().open(filename, file);

        // Write header
        file.write(SkeletalMeshHeaderID);
        file.beginVersionedSection(SkeletalMeshVersionInfo);

        // Write bones
        file.write(m->bones);

        // Write submeshes
        file.writePointerVector(m->submeshes);

        // Write export info
        file.write(ExportInfo::get());

        file.writeBytes(nullptr, 40);

        file.endVersionedSection();
        file.close();

        LOG_INFO << "Saved skeletal mesh - '" << name << "'";

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name << "' - " << e;

        return false;
    }
}

bool SkeletalMesh::addSkeletalMesh(const String& name)
{
    try
    {
        // Open the file
        auto file = FileReader();
        fileSystem().open(Mesh::MeshDirectory + name + SkeletalMeshExtension, file);

        // Check header
        if (file.readFourCC() != SkeletalMeshHeaderID)
            throw Exception("Not a skeletal mesh file");

        auto readVersion = file.beginVersionedSection(SkeletalMeshVersionInfo);

        auto newBones = Vector<Bone>();
        auto newSubmeshes = Vector<Members::SubMesh*>();
        auto exportInfo = ExportInfo();

        if (readVersion.getMajor() == 1)
            throw Exception("Deprecated skeletal mesh file format detected, please re-export");
        else
        {
            // Read bones
            file.read(newBones);

            // Read submeshes
            file.readPointerVector(newSubmeshes);

            // Read export info
            file.read(exportInfo);

            file.skip(40);
        }

        file.endVersionedSection();

        for (auto submesh : newSubmeshes)
        {
            submesh->isShadowCaster = getDefaultGeometryShadowCasterValue();

            // Take a copy of the reference pose geometry chunks needed for doing CPU skeletal animation and for
            // per-triangle hit detection on the skeletal mesh
            submesh->animationGeometryChunk = submesh->geometryChunk;
            submesh->animationGeometryChunk.deleteVertexStream(VertexStream::Bones);
            submesh->animationGeometryChunk.deleteVertexStream(VertexStream::Weights);
            submesh->animationGeometryChunk.setDynamic(true);

            // Register the appropriate chunk with the renderer, chunks are subsequently registered and deregistered by
            // setGPUSkinningEnabled()
            if (m->isGPUSkinningEnabled)
                submesh->geometryChunk.registerWithRenderer();
            else
                submesh->animationGeometryChunk.registerWithRenderer();
        }

        auto submeshCount = newSubmeshes.size();

        if (!m->addSkeletalMeshComponent(newBones, newSubmeshes, name))
        {
            for (auto newSubmesh : newSubmeshes)
                delete newSubmesh;

            throw Exception("Failed incorporating new skeletal mesh component");
        }

        // Enforce maximum weight count per vertex
        if (m->maximumAllowedWeightsPerVertex < 4)
            setMaximumAllowedWeightsPerVertex(m->maximumAllowedWeightsPerVertex);

        m->calculateBoneAABBs();
        onLocalAABBChanged();

        LOG_INFO << "Added skeletal mesh '" << name << "' - bones: " << newBones.size()
                 << ", submeshes: " << submeshCount << ", export info: " << exportInfo;

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name << "' - " << e;

        return false;
    }
}

bool SkeletalMesh::removeSkeletalMesh(const String& name)
{
    if (!m->removeSkeletalMeshComponent(name))
        return false;

    m->calculateBoneAABBs();
    onLocalAABBChanged();

    return true;
}

void SkeletalMesh::clearSkeletalMesh()
{
    m->clear();
}

struct TriangleGroup
{
    String material;
    Vector<const Triangle*> triangles;

    TriangleGroup() {}

    TriangleGroup(String material_, Vector<const Triangle*>&& triangles_)
        : material(std::move(material_)), triangles(triangles_)
    {
    }
};

bool SkeletalMesh::setup(const Vector<Bone>& bones, TriangleArraySet& triangleSet, Runnable& r)
{
    try
    {
        m->clear();

        LOG_INFO << "Creating skeletal mesh, bones: " << bones.size()
                 << ", triangles: " << triangleSet.getTriangleCount();

        r.beginTask("Checking data integrity", 5);

        // Check bone count doesn't exceed the maximum
        if (bones.size() > MaximumBoneCount)
            throw Exception() << "Too many bones, maximum is " << MaximumBoneCount;

        // Remove any degenerate triangles
        r.beginTask("removing degenerate triangles", 20);
        for (auto triangles : triangleSet)
        {
            if (!triangles->removeDegenerateTriangles(r))
                return false;
        }
        r.endTask();

        // Validate the bones
        if (bones.empty())
            throw Exception("No bones found");
        for (auto i = 0U; i < bones.size(); i++)
        {
            const auto& bone = bones[i];

            // Check bone has a name
            if (bone.name.length() == 0)
                throw Exception("Found invalid bone name");

            // Check bone name is unique
            for (auto j = i + 1; j < bones.size(); j++)
            {
                if (bone.name == bones[j].name)
                    throw Exception() << "Found duplicated bone name: " << bone.name;
            }

            // Check bone parent index is valid
            if (bone.parent >= int(i) || bone.parent < -1)
                throw Exception() << "Found invalid bone parent index: " << bone.parent;
        }

        // Validate the triangles
        for (auto triangles : triangleSet)
        {
            if (!triangles->hasVertexStream(VertexStream::Bones) || !triangles->hasVertexStream(VertexStream::Weights))
                throw Exception() << "Invalid triangle set, missing a required skeletal vertex stream";

            // Fill the geometry chunk with vertex data
            auto itBones =
                triangles->getVertexDataGeometryChunk().getVertexStreamConstIterator<uint8_t>(VertexStream::Bones);
            auto itWeights =
                triangles->getVertexDataGeometryChunk().getVertexStreamConstIterator<float>(VertexStream::Weights);

            for (auto i = 0U; i < triangles->getVertexDataGeometryChunk().getVertexCount(); i++)
            {
                auto sum = 0.0f;

                for (auto j = 0U; j < 4; j++)
                {
                    // Check bone index
                    if (itBones[j] >= bones.size())
                        throw Exception() << "Found invalid vertex weight bone index: " << itBones[j];

                    // Check weight is in the valid range
                    if (itWeights[j] < 0.0f || itWeights[j] > 1.0f)
                        throw Exception() << "Found invalid vertex weight: " << itWeights[j];

                    sum += itWeights[j];
                }

                // Check weights sum to one
                if (fabsf(sum - 1.0f) > 0.01f)
                    throw Exception() << "Found vertex with bad weight sum: " << sum;
            }
        }

        // Store the bones
        m->bones = bones;
        for (auto& bone : m->bones)
            bone.calculateLength();

        // Calculate the inverseReferenceAbsolute value for each bone
        m->calculateBoneInverseReferenceAbsolutes();
        m->calculateBoneRagdollReferenceOrientationAbsoluteInverses();

        r.endTask();

        // Group triangles in each set by their material
        r.beginTask("Gathering materials", 5);
        auto groupedTriangles = Vector<std::pair<String, Vector<const Triangle*>>>();
        for (auto i = 0U; i < triangleSet.size(); i++)
        {
            auto& triangleArray = triangleSet[i];

            auto materialTriangles = std::unordered_map<String, Vector<const Triangle*>>();
            for (auto& triangle : triangleArray)
                materialTriangles[triangle.getMaterial()].append(&triangle);

            for (auto& tris : materialTriangles)
                groupedTriangles.emplace(tris.first, std::move(tris.second));

            if (!r.setTaskProgress(i + 1, triangleSet.size()))
                return false;
        }
        r.endTask();

        auto triangleCount = triangleSet.getTriangleCount();

        // Compile each submesh
        for (auto i = 0U; i < groupedTriangles.size(); i++)
        {
            auto submesh = new Members::SubMesh;
            m->submeshes.append(submesh);

            submesh->material = groupedTriangles[i].first;
            auto& submeshTriangles = groupedTriangles[i].second;

            r.beginTask(String() + "submesh " + (i + 1) + " of " + groupedTriangles.size() + " with " +
                            submeshTriangles.size() + " triangles",
                        90.0f * float(submeshTriangles.size()) / float(triangleCount));

            auto triangleArray = submeshTriangles[0]->getParentTriangleArray();

            // Setup the geometry chunk
            submesh->geometryChunk.setVertexStreams(triangleArray->getVertexStreams());
            submesh->geometryChunk.setVertexCount(submeshTriangles.size() * 3);

            auto vertexSize = submesh->geometryChunk.getVertexSize();

            // Fill with raw vertex data
            auto lockedVertexData = submesh->geometryChunk.lockVertexData();
            for (auto triangle : submeshTriangles)
            {
                for (auto index : triangle->getIndices())
                {
                    memcpy(lockedVertexData, triangleArray->getVertexData(index), vertexSize);
                    lockedVertexData += vertexSize;
                }
            }
            submesh->geometryChunk.unlockVertexData();

            submesh->geometryChunk.setIndexDataStraight();

            // Validate the vertex position data
            if (!submesh->geometryChunk.validateVertexPositionData())
                throw Exception("Mesh vertex positions are not valid");

            // Compile the vertex data
            r.beginTask("optimizing vertex array", 5);
            if (!submesh->geometryChunk.optimizeVertexData(r))
                throw Exception("Vertex array construction interrupted");
            r.endTask();

            r.beginTask("calculating tangent bases", 10);
            if (!submesh->geometryChunk.calculateTangentBases())
                throw Exception("Tangent bases calculation interrupted");
            r.endTask();

            r.beginTask("optimizing vertex array", 5);
            if (!submesh->geometryChunk.optimizeVertexData(r))
                throw Exception("Vertex array construction interrupted");
            r.endTask();

            r.beginTask("calculating triangle strips", 75);
            if (!submesh->geometryChunk.generateTriangleStrips(r))
                throw Exception("Triangle stripping interrupted");
            r.endTask();

            r.beginTask("optimizing vertex array", 5);
            if (!submesh->geometryChunk.optimizeVertexData(r))
                throw Exception("Vertex array construction interrupted");
            r.endTask();

            if (!submesh->createLocalBoneIndexMap())
                throw Exception() << "Material '" << submesh->material
                                  << "' uses too many bones, maximum: " << MaximumBoneCount;

            submesh->calculateWeightsPerVertex();

            r.endTask();
        }

        m->calculateBoneAABBs();
        onLocalAABBChanged();

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        clear();

        return false;
    }
}

void SkeletalMesh::intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry)
{
    ComplexEntity::intersectRay(ray, intersections, onlyWorldGeometry);

    if (!isVisible() || m->submeshes.empty())
        return;

    // Check if the intersection is only targeted for world geometry
    if (onlyWorldGeometry && !isWorldGeometry())
        return;

    // Transform the ray into local space
    auto localRay = getWorldTransform().getInverse() * ray;

    // Check ray against all the submeshes. Note that this checks against the 'animationGeometryChunk' member of each
    // submesh which contains the CPU skinned version of the mesh needed for this type of intersection test. Because of
    // this it is necessary to first ensure that the local CPU skinned geometry is current and then update it if
    // required. Only vertex positions need to be current for intersection tests.
    m->skinVertexPositions();
    for (auto submesh : m->submeshes)
    {
        auto results = Vector<GeometryChunk::IntersectionResult>();
        submesh->animationGeometryChunk.intersect(localRay, results);

        for (auto& result : results)
        {
            auto p = localToWorld(localRay.getPoint(result.getDistance()) * getMeshScale());

            auto material = getMaterialRoot() + submesh->material;

            if (!onlyWorldGeometry || (getScene() && getScene()->isWorldGeometryMaterial(material)))
                intersections.emplace(ray.getOrigin().distance(p), p, result.getNormal(), this, material);
        }
    }
}

bool SkeletalMesh::isShadowCaster() const
{
    if (m->submeshes.has([](const Members::SubMesh* submesh) { return submesh->isShadowCaster; }))
        return true;

    return ComplexEntity::isShadowCaster();
}

void SkeletalMesh::setShadowCaster(bool isShadowCaster)
{
    for (auto submesh : m->submeshes)
        submesh->isShadowCaster = true;

    ComplexEntity::setShadowCaster(isShadowCaster);
}

void SkeletalMesh::setShadowCaster(const String& meshName, bool isShadowCaster)
{
    for (auto submesh : m->submeshes)
    {
        if (submesh->skeletalMeshComponent == meshName)
            submesh->isShadowCaster = isShadowCaster;
    }

    return ComplexEntity::setShadowCaster(meshName, isShadowCaster);
}

bool SkeletalMesh::gatherGeometry(GeometryGather& gather)
{
    if (!ComplexEntity::gatherGeometry(gather))
        return false;

    if (shouldProcessGather(gather))
    {
        // Test whether this skeletal mesh is visible
        auto isSkeletalMeshCulled = !gather.getFrustum().intersect(getWorldAABB()) && !m->isDrawSkeletonEnabled;

        if (!isSkeletalMeshCulled)
        {
            if (m->isDrawSkeletonEnabled)
                m->updateSkeletonGeometryChunk();
            else
            {
                // If doing CPU skinning then ensure the animation result is current
                if (!m->isGPUSkinningEnabled)
                    m->skinVertices();
            }

            // When gathering shadow extents tell the renderer the extents of the current animated geometry, the extents
            // of the static geometry stored in the geometry chunks is insufficient
            if (gather.isShadowGeometryGather())
                gather.enlargeExtraWorldSpaceShadowCasterExtents(getWorldAABB());

            gather.changePriority(getRenderPriority());
            gather.changeTransformation(getWorldTransform(), getMeshScale());

            if (m->isDrawSkeletonEnabled)
            {
                if (!isSkeletalMeshCulled)
                {
                    gather.changeMaterial("ImmediateGeometry");
                    gather.addGeometryChunk(m->skeletonGeometryChunk);
                }
            }
            else
            {
                // If we're doing GPU skinning then make sure the GPU bone transforms are up to date
                if (m->isGPUSkinningEnabled)
                    m->calculateGPUBoneTransforms();

                for (auto submesh : m->submeshes)
                {
                    if (gather.isShadowGeometryGather() && !submesh->isShadowCaster)
                        continue;

                    auto material = String();
                    if (getOverrideMaterial().length())
                        material = getOverrideMaterial();
                    else
                        material = getMaterialRoot() + submesh->material;

                    auto overrideParameters = getMaterialOverrideParameters(material);

                    if (m->isGPUSkinningEnabled)
                    {
                        // Shaders that support GPU skinning look for "boneCount", "boneTransforms" and
                        // "weightsPerVertex" parameters that contain the data needed for doing GPU skinning

                        submesh->internalParams[Parameter::boneCount].setInteger(
                            submesh->localBoneIndexToAbsoluteBoneIndex.size());
                        submesh->internalParams[Parameter::boneTransforms].setPointer(submesh->gpuBoneTransforms);
                        submesh->internalParams[Parameter::weightsPerVertex].setInteger(submesh->weightsPerVertex);

                        gather.newMaterial(&materials().getMaterial(material), overrideParameters,
                                           submesh->internalParams);
                        gather.addGeometryChunk(submesh->geometryChunk);
                    }
                    else
                    {
                        gather.changeMaterial(material, overrideParameters);
                        gather.addGeometryChunk(submesh->animationGeometryChunk);
                    }
                }
            }
        }
    }

    return true;
}

void SkeletalMesh::precache()
{
    for (auto submesh : m->submeshes)
    {
        materials().getMaterial(getMaterialRoot() + submesh->material).precache();
        submesh->geometryChunk.registerWithRenderer();
    }

    ComplexEntity::precache();
}

bool SkeletalMesh::isPerFrameUpdateRequired() const
{
    return true;
}

void SkeletalMesh::update()
{
    ComplexEntity::update();

    auto completedAnimations = Vector<const SkeletalAnimation*>();

    // Update the current frame for all the animations
    for (auto i = 0U; i < m->activeAnimations.size(); i++)
    {
        auto& activeAnimation = m->activeAnimations[i];

        if (activeAnimation.isPaused)
            continue;

        m->isCPUAnimationCurrent = false;
        m->areCPUAnimatedVertexPositionsCurrent = false;

        // Increase frame step
        activeAnimation.currentFrame +=
            platform().getSecondsPassed() * activeAnimation.animation->getFrameRate() * m->animationSpeedScale;

        // If the animation has reached the end then queue it to have the handler called
        if (uint(activeAnimation.currentFrame) >= activeAnimation.animation->getFrameCount())
        {
            completedAnimations.append(activeAnimation.animation);

            // Remove the animation if it is not set to loop
            if (!activeAnimation.loop)
                m->activeAnimations.erase(i--);
        }
    }

    // Notify about animation completions
    for (auto& completedAnimation : completedAnimations)
        onAnimationFinished(completedAnimation);

    auto hasSkeletonChanged = false;

    if (isPhysical())
    {
        // Apply the results from the ragdoll simulation to the bones of this skeletal mesh
        m->alignBonesToRagdollBodies();
        hasSkeletonChanged = true;
    }
    else
    {
        // Apply animations to the skeleton
        for (auto& activeAnimation : m->activeAnimations)
        {
            // Normalize current frame value
            activeAnimation.normalizeCurrentFrame();

            // Get frame numbers to use
            auto frame = uint(activeAnimation.currentFrame);
            auto nextFrame = (frame + 1) % activeAnimation.animation->getFrameCount();

            // Detect when blending from the initial bone transforms to the first frame of the animation has been
            // completed, and revert back to standard animation.
            if (activeAnimation.blendFromInitialBoneTransforms && frame >= 1)
            {
                activeAnimation.blendFromInitialBoneTransforms = false;
                activeAnimation.currentFrame -= 1.0f;
                frame--;
                nextFrame = (frame + 1) % activeAnimation.animation->getFrameCount();
            }

            // The final frame of a non-looping animation should not be blended with the first frame
            if (!activeAnimation.loop && nextFrame < frame)
                nextFrame = frame;

            // Work out interpolation between the frames
            auto t = activeAnimation.currentFrame - float(frame);

            // Set interpolated relative bone transforms from this animation
            for (auto j = 0U; j < activeAnimation.animation->getBoneAnimations().size(); j++)
            {
                auto boneIndex = activeAnimation.boneIndices[j];
                if (boneIndex != -1)
                {
                    auto& frames = activeAnimation.animation->getBoneAnimations()[j].frames;

                    if (activeAnimation.blendFromInitialBoneTransforms)
                        m->bones[boneIndex].currentRelative =
                            activeAnimation.initialBoneTransforms[boneIndex].interpolate(frames[0], t);
                    else
                        m->bones[boneIndex].currentRelative = frames[frame].interpolate(frames[nextFrame], t);
                }
            }
        }

        hasSkeletonChanged = !m->activeAnimations.empty();
    }

    // Update this entity's position and orientation if using a root bone on the skeleton to define entity movement
    if (m->rootBoneToFollow != -1)
    {
        auto& rootBoneToFollow = m->bones[m->rootBoneToFollow];

        // Transform this entity based on the changes to the root bone being followed
        transform(m->lastRootBoneToFollowTransform.getInverse() * rootBoneToFollow.currentRelative);
        m->lastRootBoneToFollowTransform = rootBoneToFollow.currentRelative;

        // Zero the root bone transform as it has now been put onto the entity
        rootBoneToFollow.currentRelative = SimpleTransform::Identity;

        hasSkeletonChanged = true;
    }

    // Update affected things when the skeleton has changed
    if (hasSkeletonChanged)
    {
        m->updateBoneAbsoluteTransforms();
        onLocalAABBChanged();
    }

    // Invalidate the world transforms of child entities that are attached to bones in this skeletal mesh
    if (getChildCount() && m->activeAnimations.size())
    {
        for (auto& bone : m->bones)
        {
            for (auto j = 0U; j < getChildCount(); j++)
                getChild(j)->invalidateWorldTransform(bone.name);
        }
    }
}

bool SkeletalMesh::isGPUSkinningEnabled()
{
    return m->isGPUSkinningEnabled;
}

void SkeletalMesh::setGPUSkinningEnabled(bool enabled)
{
    if (m->isGPUSkinningEnabled == enabled)
        return;

    m->isGPUSkinningEnabled = enabled;

    // Register and deregister the appropriate chunks with the renderer. When doing GPU skinning the reference pose
    // chunk needs to be registered, and when doing CPU skinning the animated geometry chunk needs to be registered.
    if (!m->isGPUSkinningEnabled)
    {
        for (auto submesh : m->submeshes)
        {
            submesh->geometryChunk.unregisterWithRenderer();
            submesh->animationGeometryChunk.registerWithRenderer();
        }
    }
    else
    {
        for (auto submesh : m->submeshes)
        {
            submesh->animationGeometryChunk.unregisterWithRenderer();
            submesh->geometryChunk.registerWithRenderer();
        }
    }
}

Vector<String> SkeletalMesh::getMaterials() const
{
    return m->submeshes.map<String>([](const Members::SubMesh* mesh) { return mesh->material; });
}

bool SkeletalMesh::setMaterials(const Vector<String>& materials)
{
    if (materials.size() != m->submeshes.size())
    {
        LOG_ERROR << "Incorrect number of materials, expected " << m->submeshes.size() << " but received "
                  << materials.size();
        return false;
    }

    for (auto i = 0U; i < m->submeshes.size(); i++)
        m->submeshes[i]->material = materials[i];

    return true;
}

int SkeletalMesh::getRootBoneToFollow() const
{
    return m->rootBoneToFollow;
}

bool SkeletalMesh::setRootBoneToFollow(int index)
{
    if (index < -1 || index >= int(m->bones.size()) || m->bones[index].parent != -1)
    {
        LOG_ERROR << "Invalid root bone: " << index;
        return false;
    }

    m->rootBoneToFollow = index;
    m->lastRootBoneToFollowTransform = m->bones[index].currentRelative;

    return true;
}

unsigned int SkeletalMesh::getBoneCount() const
{
    return m->bones.size();
}

const SkeletalMesh::Bone& SkeletalMesh::getBone(unsigned int index) const
{
    return m->bones[index];
}

Vector<String> SkeletalMesh::getBoneNames() const
{
    return m->bones.map<String>([](const Bone& bone) { return bone.name; });
}

int SkeletalMesh::getBoneIndex(const String& boneName) const
{
    return m->bones.findBy([&](const Bone& bone) { return bone.name == boneName; });
}

bool SkeletalMesh::getBoneReferencePoseRelativeTransform(unsigned int index, SimpleTransform& transform) const
{
    if (index >= m->bones.size())
        return false;

    transform = m->bones[index].referenceRelative;

    return true;
}

bool SkeletalMesh::getBoneRelativeTransform(unsigned int index, SimpleTransform& transform) const
{
    if (index >= m->bones.size())
        return false;

    transform = m->bones[index].currentRelative;

    return true;
}

bool SkeletalMesh::setBoneRelativeTransform(unsigned int index, const SimpleTransform& transform)
{
    if (index >= m->bones.size())
        return false;

    m->bones[index].currentRelative = transform;

    m->isCPUAnimationCurrent = false;
    m->areCPUAnimatedVertexPositionsCurrent = false;
    onLocalAABBChanged();

    return true;
}

bool SkeletalMesh::isRagdollBone(const String& boneName) const
{
    for (auto& bone : m->bones)
    {
        if (bone.name == boneName)
            return bone.isRagdollBone;
    }

    return false;
}

bool SkeletalMesh::setRagdollBone(const String& boneName, bool ragdollBone)
{
    if (isPhysical())
    {
        LOG_ERROR << "Can't change ragdoll bones when the skeleton has been made physical";
        return false;
    }

    for (auto& bone : m->bones)
    {
        if (bone.name == boneName)
        {
            bone.isRagdollBone = ragdollBone;
            return true;
        }
    }

    return false;
}

bool SkeletalMesh::makePhysical(float mass, bool fixed)
{
    if (!getScene())
    {
        LOG_ERROR << "This skeletal mesh can't be made physical because it is not in a scene";
        return false;
    }

    if (m->bones.empty())
    {
        LOG_ERROR << "This skeletal mesh can't be made physical because it has no bones";
        return false;
    }

    return m->createRagdoll(mass / float(m->bones.size()), fixed);
}

bool SkeletalMesh::isPhysical() const
{
    return m->ragdollBodies.size() != 0;
}

void SkeletalMesh::makeNotPhysical()
{
    m->destroyRagdoll();
}

void SkeletalMesh::calculateLocalAABB() const
{
    ComplexEntity::calculateLocalAABB();

    // Extend the local space bounding box to include the current boundings of the geometry attached to each bone
    auto localSkeletalMeshBoundingAABB = AABB();
    for (auto& bone : m->bones)
    {
        if (bone.aabb == AABB())
            continue;

        localSkeletalMeshBoundingAABB.merge(bone.aabb, bone.currentCombinedTransform);
    }

    localAABB_.merge(localSkeletalMeshBoundingAABB, SimpleTransform::Identity, getMeshScale());
}

void SkeletalMesh::debugSkeleton() const
{
    LOG_DEBUG << "Skeleton for '" << getName() << "', bone count: " << m->bones.size();

    for (auto i = 0U; i < m->bones.size(); i++)
        LOG_DEBUG << i << ". - " << m->bones[i].name << ", parent: " << m->bones[i].parent;
}

SkeletalMesh::operator UnicodeString() const
{
    auto info = Vector<UnicodeString>();

    info.append("");
    info.append("bones: [" + UnicodeString(getBoneNames(), " ") + "]");

    auto components = Vector<String>();
    for (auto submesh : m->submeshes)
    {
        if (!components.has(submesh->skeletalMeshComponent))
            components.append(submesh->skeletalMeshComponent);
    }
    if (components.size())
        info.append("skeletal mesh components: [" + UnicodeString(components) + "]");

    if (m->activeAnimations.size())
    {
        info.append("animations: [");
        info.append(UnicodeString(
            m->activeAnimations.map<String>([](const Members::ActiveAnimation& a) { return a.animation->getName(); })));
        info.back() << "]";
    }

    return ComplexEntity::operator UnicodeString() << info;
}

bool SkeletalMesh::setMaximumAllowedWeightsPerVertex(unsigned int maximumAllowedWeightsPerVertex)
{
    if (maximumAllowedWeightsPerVertex < 1 || maximumAllowedWeightsPerVertex > 4)
    {
        LOG_ERROR << "The maximum weight count must be 1, 2, 3 or 4";
        return false;
    }

    // Create list of geometry chunks that need to have their weights adjusted
    auto chunks = Vector<GeometryChunk*>();
    for (auto submesh : m->submeshes)
    {
        if (submesh->weightsPerVertex <= maximumAllowedWeightsPerVertex)
            continue;

        chunks.append(&submesh->geometryChunk);
        chunks.append(&submesh->animationGeometryChunk);

        submesh->weightsPerVertex = maximumAllowedWeightsPerVertex;
    }

    // Loop over the chunks
    for (auto chunk : chunks)
    {
        chunk->lockVertexData();

        auto itBones = chunk->getVertexStreamIterator<uint8_t>(VertexStream::Bones);
        auto itWeights = chunk->getVertexStreamIterator<float>(VertexStream::Weights);

        // Loop over the vertices in this chunk
        for (auto j = 0U; j < chunk->getVertexCount(); j++)
        {
            // Collect this vertex's weights
            auto weights = Vector<VertexWeight>();
            for (auto k = 0U; k < 4; k++)
            {
                if (itWeights[k] != 0.0f)
                    weights.emplace(itBones[k], itWeights[k]);
            }

            // Reduce weight count as requested
            VertexWeight::limitWeightCount(weights, maximumAllowedWeightsPerVertex);

            // Put new weights back onto this vertex
            for (auto k = 0U; k < 4; k++)
            {
                itBones[k] = k < weights.size() ? weights[k].getBone() : 1;
                itWeights[k] = k < weights.size() ? weights[k].getWeight() : 0.0f;
            }

            // Next vertex
            itBones++;
            itWeights++;
        }

        chunk->unlockVertexData();
    }

    if (chunks.size())
    {
        LOG_INFO << "Reduced weights per vertex to " << maximumAllowedWeightsPerVertex
                 << " on skeletal mesh: " << getName();
    }

    m->maximumAllowedWeightsPerVertex = maximumAllowedWeightsPerVertex;

    return true;
}

void SkeletalMesh::setDrawSkeletonEnabled(bool enabled)
{
    m->isDrawSkeletonEnabled = enabled;
}

bool SkeletalMesh::isDrawSkeletonEnabled() const
{
    return m->isDrawSkeletonEnabled;
}

}
