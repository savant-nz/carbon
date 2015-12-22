/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAYA_EXPORTER

#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Exporters/ProgressDialog.h"
#include "CarbonEngine/Exporters/ExporterStrings.h"
#include "CarbonEngine/Exporters/Maya/MayaGeometryHelper.h"
#include "CarbonEngine/Exporters/Maya/MayaHelper.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Geometry/TriangleArraySet.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/SkeletalMesh.h"

namespace Carbon
{

namespace Maya
{

class SkeletalMeshExportRunner : public Runnable
{
public:

    SkeletalMeshExportRunner(const UnicodeString& filename) : filename_(filename) {}

    Vector<MDagPath> jointPaths;
    Vector<SkeletalMesh::Bone> bones;
    TriangleArraySet triangleSet;

    bool run() override
    {
        auto skeletalMesh = SkeletalMesh();

        beginTask("Reading skeletal mesh structure", 40);
        beginTask("", 50);

        exportJointPaths();
        if (jointPaths.empty())
        {
            LOG_ERROR_WITHOUT_CALLER << "Did not find any joints";
            return false;
        }

        // Export bones
        if (!exportBones())
        {
            LOG_ERROR_WITHOUT_CALLER << "Failed exporting bones";
            return false;
        }

        endTask();
        beginTask("", 50);

        // Export skin clusters
        if (!exportSkinClusters())
        {
            LOG_ERROR_WITHOUT_CALLER << "Failed exporting skin clusters";
            return false;
        }

        endTask();
        endTask();

        // Put exported data into the skeletal mesh
        beginTask("Compiling", 59);
        if (!skeletalMesh.setup(bones, triangleSet, *this))
        {
            LOG_ERROR_WITHOUT_CALLER << "Failed setting up SkeletalMesh class";
            return false;
        }
        endTask();

        // Save skeletal mesh file
        beginTask("Saving file", 1);
        if (!skeletalMesh.saveSkeletalMesh(FileSystem::LocalFilePrefix + filename_))
        {
            LOG_ERROR_WITHOUT_CALLER << "Failed saving file";
            return false;
        }
        endTask();

        Helper::exportAllMaterials(triangleSet, FileSystem::getDirectory(filename_));

        return true;
    }

    bool exportJointPaths()
    {
        // DAG iterator to go through all joints
        auto dagIt = MItDag(MItDag::kDepthFirst, MFn::kJoint);

        // Check there is a root joint
        auto rootPath = MDagPath();
        dagIt.getPath(rootPath);
        if (!rootPath.isValid())
        {
            LOG_ERROR << "Failed finding root joint";
            return false;
        }

        // Store paths to all the joints in jointPaths
        jointPaths.clear();
        for (; !dagIt.isDone(); dagIt.next())
        {
            auto dagPath = MDagPath();
            dagIt.getPath(dagPath);
            jointPaths.append(dagPath);
        }

        return true;
    }

    // Exports the relative bind pose transform for the joint at the given DAG node, and puts it onto the given bone. Two
    // diferent methods for finding the bind pose are used, if they both fail then the current joint transform is used and a
    // warning is emitted.
    void exportJointBindPose(MFnDagNode& fnJoint, SkeletalMesh::Bone& bone)
    {
        auto status = MStatus();

        // Find worldMatrix array attribute on this joint
        auto attrWorldMatrix = fnJoint.attribute("worldMatrix", &status);
        if (status == MS::kSuccess)
        {
            // Loop through all the plugs connected to the worldMatrix attribute on this joint
            auto plugWorldMatrixArray = MPlug(fnJoint.object(), attrWorldMatrix);
            for (auto j = 0U; j < plugWorldMatrixArray.numElements(); j++)
            {
                auto elementPlug = plugWorldMatrixArray[j];

                // Starting from this plug, search for a skin cluster, if one is found then this will be the skin cluster that
                // has this joint's bind pose
                auto dgIt = MItDependencyGraph(elementPlug, MFn::kInvalid, MItDependencyGraph::kDownstream,
                                               MItDependencyGraph::kDepthFirst, MItDependencyGraph::kPlugLevel, &status);
                if (status != MS::kSuccess)
                    continue;

                dgIt.disablePruningOnFilter();
                for (; !dgIt.isDone(); dgIt.next())
                {
                    auto thisNode = dgIt.thisNode();
                    if (thisNode.apiType() != MFn::kSkinClusterFilter)
                        continue;

                    MFnSkinCluster skinFn(thisNode);

                    // Now that the skin cluster for this joint has been found, get its bindPreMatrix array plug
                    auto bindPreMatrixArrayPlug = skinFn.findPlug("bindPreMatrix", &status);
                    if (status == MS::kSuccess)
                    {
                        // Look up the plug path to see how we got to the skin cluster
                        auto plugs = MPlugArray();
                        dgIt.getPlugPath(plugs);
                        plugs[0].connectedTo(plugs, false, true);

                        // Look up the bindPreMatrix array plug on the skin cluster with the logical index of the matrix that
                        // holds this joint's world space bind pose
                        auto bindPreMatrixPlug = bindPreMatrixArrayPlug.elementByLogicalIndex(plugs[0].logicalIndex(), &status);
                        if (status == MS::kSuccess)
                        {
                            // Retrieve the matrix data
                            auto dataObject = MObject();
                            bindPreMatrixPlug.getValue(dataObject);
                            auto bindPoseMatrix = MFnMatrixData(dataObject).matrix().inverse();

                            // Get the world-space bind pose transform for this joint
                            bone.currentAbsolute = MMatrixToAffineTransform(bindPoseMatrix);

                            // Convert the absolute pose to a relative pose
                            if (bone.parent == -1)
                                bone.referenceRelative = bone.currentAbsolute;
                            else
                            {
                                const auto& parentBone = bones[bone.parent];

                                bone.referenceRelative.setPosition(
                                    parentBone.currentAbsolute.getOrientation().getInverse() *
                                    (bone.currentAbsolute.getPosition() - parentBone.currentAbsolute.getPosition()));
                                bone.referenceRelative.setOrientation(bone.currentAbsolute.getOrientation() *
                                                                      parentBone.currentAbsolute.getOrientation().getInverse());
                            }

                            return;
                        }
                    }
                }
            }
        }

        // Try to retrieve the bind pose for this joint using a bindPose plug
        auto tempBindPosePlug = fnJoint.findPlug("bindPose", &status);
        if (status == MS::kSuccess)
        {
            auto mapConnections = MPlugArray();
            tempBindPosePlug.connectedTo(mapConnections, false, true, &status);
            if (status == MS::kSuccess && mapConnections.length() > 0)
            {
                auto bindPosePlug = mapConnections[0];

                // This node should be a "dagPose" node (see docs)
                MFnDependencyNode bindPoseNode(bindPosePlug.node(), &status);
                if (status == MS::kSuccess)
                {
                    // Look for the "xformMatrix" attribute.
                    auto xformMatrixAttribute = bindPoseNode.attribute("xformMatrix", &status);
                    if (status == MS::kSuccess)
                    {
                        auto localTransformPlug = MPlug(bindPosePlug.node(), xformMatrixAttribute);

                        // xformMatrix is an array. To get the local matrix we need to select the same index as our bindPosePlug
                        // (logicalIndex())
                        localTransformPlug.selectAncestorLogicalIndex(bindPosePlug.logicalIndex(), xformMatrixAttribute);

                        // Get out the matrix value as an object
                        auto localMatrixObject = MObject();
                        localTransformPlug.getValue(localMatrixObject);

                        // Read the matrix data out of the object and assign to the bone
                        bone.referenceRelative = MMatrixToAffineTransform(MFnMatrixData(localMatrixObject).matrix());
                        updateBoneCurrentAbsoluteUsingReferenceRelative(bone);

                        return;
                    }
                }
            }
        }

        LOG_WARNING_WITHOUT_CALLER << "No bind pose found for bone " << bone.name << ", using current transform instead";

        bone.referenceRelative = MMatrixToAffineTransform(fnJoint.transformationMatrix());
        updateBoneCurrentAbsoluteUsingReferenceRelative(bone);
    }

    // Exports rotation constraints for a single axis of a joint.
    void exportJointAxisRotationConstraints(MFnTransform& fnTransform, SkeletalMesh::Bone::RagdollAxisConstraint& constraint,
                                            MFnTransform::LimitType minLimit, MFnTransform::LimitType maxLimit)
    {
        constraint.enabled = fnTransform.isLimited(minLimit) && fnTransform.isLimited(maxLimit);

        if (constraint.enabled)
        {
            constraint.minimumAngle = float(fnTransform.limitValue(minLimit));
            constraint.maximumAngle = float(fnTransform.limitValue(maxLimit));
        }
    }

    // Updates a bone's currentAbsolute transform based on its referenceRelative. This assumes that the parent bone already has
    // a correct currentAbsolute transform set.
    void updateBoneCurrentAbsoluteUsingReferenceRelative(SkeletalMesh::Bone& bone)
    {
        // Update currentAbsolute on this bone as well as it may be needed to calculate the referenceRelative of upcoming bones
        // (i.e. they have a proper bind pose retrievable by exportJointBindPose())
        if (bone.parent == -1)
            bone.currentAbsolute = bone.referenceRelative;
        else
        {
            const auto& parentBone = bones[bone.parent];

            bone.currentAbsolute.setOrientation(bone.referenceRelative.getOrientation() *
                                                parentBone.currentAbsolute.getOrientation());
            bone.currentAbsolute.setPosition(parentBone.currentAbsolute.getPosition() +
                                             parentBone.currentAbsolute.getOrientation() *
                                                 bone.referenceRelative.getPosition());
        }
    }

    bool exportBones()
    {
        try
        {
            auto status = MStatus();

            bones.resize(jointPaths.size());
            for (auto i = 0U; i < jointPaths.size(); i++)
            {
                auto& bone = bones[i];

                auto jointNode = jointPaths[i].node();
                MFnDagNode fnJoint(jointNode, &status);
                if (status != MS::kSuccess)
                    throw Exception("Failed getting joint node");

                // Get bone name, TODO: using partialPathName() isn't always right
                bone.name = fnJoint.partialPathName().asChar();

                // Check joint has exactly one parent
                if (fnJoint.parentCount() != 1)
                    throw Exception() << "Joint " << bone.name << " does not have exactly one parent";

                // Get parent joint
                auto parentObj = fnJoint.parent(0);
                if (parentObj.hasFn(MFn::kJoint))
                {
                    MFnIkJoint fnParentJoint(parentObj);

                    // Get parent joint index from its name
                    auto parentName = String(fnParentJoint.partialPathName().asChar());
                    for (auto j = 0U; j < i; j++)
                    {
                        if (bones[j].name == parentName)
                        {
                            bone.parent = j;
                            break;
                        }
                    }
                }
                else
                {
                    // No parent
                    bone.parent = -1;
                }

                exportJointBindPose(fnJoint, bone);

                // Export ragdoll rotation constraints
                MFnTransform fnTransform(jointNode, &status);
                exportJointAxisRotationConstraints(fnTransform, bone.ragdollXConstraint, MFnTransform::kRotateMinX,
                                                   MFnTransform::kRotateMaxX);
                exportJointAxisRotationConstraints(fnTransform, bone.ragdollYConstraint, MFnTransform::kRotateMinY,
                                                   MFnTransform::kRotateMaxY);
                exportJointAxisRotationConstraints(fnTransform, bone.ragdollZConstraint, MFnTransform::kRotateMinZ,
                                                   MFnTransform::kRotateMaxZ);

                LOG_INFO << "Exported bone: " << bone.name;

                if (setTaskProgress(i + 1, jointPaths.size()))
                    return false;
            }

            return true;
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;

            return false;
        }
    }

    bool exportSkinClusters()
    {
        auto status = MStatus();
        for (auto depIt = MItDependencyNodes(MFn::kSkinClusterFilter); !depIt.isDone(); depIt.next())
        {
            MFnSkinCluster fnSkinCluster(depIt.thisNode());

            LOG_INFO << "Exporting skin cluster: " << MStringToString(fnSkinCluster.name());

            // Get the list of influence objects for this skin cluster. These should all be bones, however the indices used on
            // the skin cluster reference this array and not our bones array, so we need to create a mapping between the two.
            auto influenceObjectPaths = MDagPathArray();
            fnSkinCluster.influenceObjects(influenceObjectPaths, &status);

            // Maps an influence object index on this skin cluster to a bone index in the exported skeleton
            auto influenceObjectBoneIndices = Vector<unsigned int>();
            for (auto i = 0U; i < influenceObjectPaths.length(); i++)
            {
                for (auto j = 0U; j < jointPaths.size(); j++)
                {
                    if (influenceObjectPaths[i] == jointPaths[j])
                    {
                        influenceObjectBoneIndices.append(j);
                        break;
                    }
                }

                if (influenceObjectBoneIndices.size() != i + 1)
                {
                    LOG_ERROR_WITHOUT_CALLER << "Failed matching skin cluster influence " << i << " to a joint path";
                    return false;
                }
            }

            auto numGeoms = fnSkinCluster.numOutputConnections();

            for (auto i = 0U; i < numGeoms; i++)
            {
                auto skeletalVertices = Vector<Vector<SkeletalMesh::VertexWeight>>();

                auto index = fnSkinCluster.indexForOutputConnection(i);

                // Get the DAG path of this geometry
                auto geomPath = MDagPath();
                fnSkinCluster.getPathAtIndex(index, geomPath);

                auto node = geomPath.node();
                if (onlyExportSelected && !Helper::isObjectSelected(node))
                    continue;

                // Iterate through all the components (vertices) of this geometry
                for (auto geometryIt = MItGeometry(geomPath); !geometryIt.isDone(); geometryIt.next())
                {
                    auto obj = geometryIt.component();
                    if (obj.apiType() != MFn::kMeshVertComponent)
                        continue;

                    // Get vertex weights
                    auto influenceCount = 0U;
                    auto weights = MFloatArray();
                    fnSkinCluster.getWeights(geomPath, obj, weights, influenceCount);

                    // Read vertex weight data
                    skeletalVertices.append(Vector<SkeletalMesh::VertexWeight>());
                    auto& vertexWeights = skeletalVertices.back();
                    for (auto j = 0U; j < weights.length(); j++)
                    {
                        if (weights[j] > Math::Epsilon)
                        {
                            // Convert the influence object index into a bone index
                            auto boneIndex = influenceObjectBoneIndices[j];

                            // Check the bone index doesn't exceed the maximum number of bones allowed
                            if (boneIndex >= SkeletalMesh::MaximumBoneCount)
                            {
                                LOG_WARNING_WITHOUT_CALLER << "Skipping vertex weight, bone index is too large: " << j;
                                continue;
                            }

                            vertexWeights.emplace(uint8_t(boneIndex), weights[j]);
                        }
                    }

                    SkeletalMesh::VertexWeight::limitWeightCount(vertexWeights, 4);

                    if (isCancelled())
                        return false;
                }

                // Get input plug on this skin cluster
                auto inputPlug = fnSkinCluster.findPlug("input", &status);
                if (status != MS::kSuccess)
                {
                    LOG_WARNING_WITHOUT_CALLER << "Failed finding input plug, geom number " << i;
                    continue;
                }

                // Retrieve the mesh right as it comes into the skin cluster to be deformed, this will be the bind pose mesh
                auto childPlug = inputPlug.elementByLogicalIndex(0);
                auto geomPlug = childPlug.child(0);
                auto dataObj1 = MObject();
                geomPlug.getValue(dataObj1);
                MFnMesh fnMesh(dataObj1, &status);
                if (status != MS::kSuccess)
                {
                    LOG_WARNING_WITHOUT_CALLER << "Failed getting mesh data input to skin cluster";
                    continue;
                }

                MFnMesh fnMeshWithShaders(geomPath, &status);
                if (status != MS::kSuccess)
                {
                    LOG_WARNING_WITHOUT_CALLER << "Failed getting mesh input for shader extraction";
                    continue;
                }

                if (!GeometryHelper::exportMFnMesh(geomPath, fnMesh, fnMeshWithShaders.object(), triangleSet, &skeletalVertices,
                                                   *this))
                    return false;
            }
        }

        return true;
    }

private:

    UnicodeString filename_;
};

class SkeletalMeshExporter : public MPxFileTranslator
{
public:

    bool canBeOpened() const override { return true; }
    bool haveReadMethod() const override { return false; }
    bool haveWriteMethod() const override { return true; }

    MString defaultExtension() const override { return toMString(SkeletalMesh::SkeletalMeshExtension.substr(1)); }

    MString filter() const override { return toMString("*" + SkeletalMesh::SkeletalMeshExtension); }

    MPxFileTranslator::MFileKind identifyFile(const MFileObject& fileName, const char* buffer, short size) const override
    {
        if (MStringToString(fileName.name()).asLower().endsWith(SkeletalMesh::SkeletalMeshExtension))
            return kIsMyFileType;

        return kNotMyFileType;
    }

    MStatus writer(const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode) override
    {
        onlyExportSelected = (mode == kExportActiveAccessMode);

        Globals::initializeEngine(getMayaClientName());

        auto runner = SkeletalMeshExportRunner(MStringToString(file.fullName()));
        ProgressDialog(SkeletalMeshExporterTitle).show(runner, M3dView::applicationShell());

        Globals::uninitializeEngine();

        return MS::kSuccess;
    }
};

void* createSkeletalMeshExporter()
{
    return new SkeletalMeshExporter;
}

}

}

#endif
