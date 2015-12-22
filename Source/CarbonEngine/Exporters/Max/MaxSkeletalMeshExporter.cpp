/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAX_EXPORTER

#include "CarbonEngine/Core/BuildInfo.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Exporters/ProgressDialog.h"
#include "CarbonEngine/Exporters/ExporterStrings.h"
#include "CarbonEngine/Exporters/ExportRunners.h"
#include "CarbonEngine/Exporters/Max/MaxPlugin.h"
#include "CarbonEngine/Exporters/Max/MaxSkeletalExporterBase.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Geometry/TriangleArraySet.h"
#include "CarbonEngine/Math/Matrix3.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/SkeletalMesh.h"

namespace Carbon
{

namespace Max
{

class SkeletalMeshExportRunner : public Runnable, public SkeletalExporterBase
{
public:

    SkeletalMeshExportRunner(UnicodeString filename) : filename_(std::move(filename)) {}

    std::unordered_map<INode*, ::Matrix3> boneBindPoses;
    TriangleArraySet triangleSet;

    // Does initial export checks for a node that are common to both skins and physiques.
    bool startExport(INode* node, ObjectState& os, unsigned int skinnedVertexCount)
    {
        os = node->EvalWorldState(0);
        if (!os.obj)
            return false;

        // Check this is also a geomobject node that we can get triangles out of
        if (os.obj->SuperClassID() != GEOMOBJECT_CLASS_ID || !os.obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
        {
            LOG_ERROR_WITHOUT_CALLER << "Node '" << node->GetName() << "' is not a triobject";
            return false;
        }

        // If the number of vertices in the skin does not equal the number of vertices in the node's triobject then tell the
        // user to check that there aren't any geometry modifiers after the skinning modifier on the stack TODO: can this be
        // made to work regardless of post-skinning geometry modifiers?
        if (uint(os.obj->NumPoints()) != skinnedVertexCount)
        {
            LOG_WARNING_WITHOUT_CALLER << "Skipping '" << node->GetName() << "' because vertex count doesn't match up";
            LOG_WARNING_WITHOUT_CALLER << "Check that there are no geometry modifiers after the skinning modifier";

            return false;
        }

        return true;
    }

    // Exports the vertex and triangle data in a physique as well as any bones it references.
    bool exportPhysique(INode* node, IPhysiqueExport* phy, IPhyContextExport* mcExport, bool doExport) override
    {
        // Read the bone bind poses on this physique, bind poses are retrieved even from physiques that aren't being exported to
        // ensure that we get bind poses for all relevant bones
        for (auto i = 0U; i < uint(mcExport->GetNumberVertices()); i++)
        {
            auto exportVertex = mcExport->GetVertexInterface(i);

            auto tm = ::Matrix3();

            if (exportVertex->GetVertexType() & BLENDED_TYPE)
            {
                auto blendedVertex = static_cast<IPhyBlendedRigidVertex*>(exportVertex);
                for (auto j = 0U; j < uint(blendedVertex->GetNumberNodes()); j++)
                {
                    if (phy->GetInitNodeTM(blendedVertex->GetNode(j), tm) == MATRIX_RETURNED)
                        boneBindPoses[blendedVertex->GetNode(j)] = tm;
                }
            }
            else
            {
                auto rigidVertex = static_cast<IPhyRigidVertex*>(exportVertex);
                if (phy->GetInitNodeTM(rigidVertex->GetNode(), tm) == MATRIX_RETURNED)
                    boneBindPoses[rigidVertex->GetNode()] = tm;
            }

            mcExport->ReleaseVertexInterface(exportVertex);
        }

        // If this physique is not to be included in the export then stop now
        if (!doExport)
            return true;

        auto os = ObjectState();
        if (!startExport(node, os, mcExport->GetNumberVertices()))
            return true;

        beginTask(node->GetName(), 100);

        beginTask("reading physique vertices", 90);

        // Vertex weights
        auto skeletalVertices = Vector<Vector<SkeletalMesh::VertexWeight>>(mcExport->GetNumberVertices());

        for (auto i = 0U; i < uint(mcExport->GetNumberVertices()); i++)
        {
            auto exportVertex = mcExport->GetVertexInterface(i);

            // New skeletal vertex to fill
            auto& sv = skeletalVertices[i];

            if (exportVertex->GetVertexType() & BLENDED_TYPE)
            {
                // This vertex is a weighted blend of a number of bones
                auto blendedVertex = static_cast<IPhyBlendedRigidVertex*>(exportVertex);

                for (auto j = 0U; j < uint(blendedVertex->GetNumberNodes()); j++)
                {
                    auto bone = blendedVertex->GetNode(j);
                    auto weight = blendedVertex->GetWeight(j);

                    auto boneIndex = findOrAddBone(bone);
                    if (boneIndex == -1)
                    {
                        LOG_WARNING_WITHOUT_CALLER << "Failed getting bone index for node: " << node->GetName();
                        return false;
                    }
                    sv.emplace(uint8_t(boneIndex), weight);
                }
            }
            else
            {
                // This vertex is fixed to a single bone
                auto rigidVertex = static_cast<IPhyRigidVertex*>(exportVertex);

                auto bone = rigidVertex->GetNode();

                auto boneIndex = findOrAddBone(bone);
                if (boneIndex == -1)
                {
                    LOG_WARNING_WITHOUT_CALLER << "Failed getting bone index for node: " << node->GetName();
                    return false;
                }
                sv.emplace(uint8_t(boneIndex), 1.0f);
            }

            mcExport->ReleaseVertexInterface(exportVertex);

            if (sv.empty())
                LOG_WARNING_WITHOUT_CALLER << "Exported vertex has no weights";

            if (setTaskProgress(i + 1, mcExport->GetNumberVertices()))
                return false;
        }

        endTask();

        beginTask("reading physique triangles", 10);
        if (!readTriangles(node, os, skeletalVertices))
            return false;
        endTask();

        endTask();

        return true;
    }

    // Exports the vertex and triangle data in a skin.
    bool exportSkin(INode* node, ISkin* skin, ISkinContextData* skinContext, bool doExport) override
    {
        // Read the bone bind poses on this skin, bind poses are retrieved even from skins that aren't being exported to ensure
        // that we get bind poses for all relevant bones
        for (auto i = 0U; i < uint(skin->GetNumBones()); i++)
        {
            auto tm = ::Matrix3();
            if (skin->GetBoneInitTM(skin->GetBone(i), tm) == SKIN_OK)
                boneBindPoses[skin->GetBone(i)] = tm;
        }

        // If this skin is not to be included in the export then stop now
        if (!doExport)
            return false;

        auto os = ObjectState();
        if (!startExport(node, os, skinContext->GetNumPoints()))
            return true;

        beginTask(node->GetName(), 100);

        beginTask("reading skin vertices", 90);

        // Vertex weights
        auto skeletalVertices = Vector<Vector<SkeletalMesh::VertexWeight>>(skinContext->GetNumPoints());

        // Go through all skin vertices
        for (auto i = 0U; i < uint(skinContext->GetNumPoints()); i++)
        {
            auto boneCount = skinContext->GetNumAssignedBones(i);
            if (boneCount < 0)
                continue;

            // New skeletal vertex to fill
            auto& sv = skeletalVertices[i];

            // Go through all bones that influence this vertex
            for (auto j = 0U; j < uint(boneCount); j++)
            {
                auto assignedBone = skinContext->GetAssignedBone(i, j);
                if (assignedBone < 0)
                    continue;

                auto bone = skin->GetBone(assignedBone);
                auto weight = skinContext->GetBoneWeight(i, j);

                auto boneIndex = findOrAddBone(bone);
                if (boneIndex == -1)
                {
                    LOG_WARNING_WITHOUT_CALLER << "Failed getting bone index for node: " << node->GetName();
                    return false;
                }

                sv.emplace(uint8_t(boneIndex), weight);
            }

            if (sv.empty())
                LOG_WARNING_WITHOUT_CALLER << "Exported vertex has no weights";

            if (setTaskProgress(i + 1, skinContext->GetNumPoints()))
                return false;
        }

        endTask();

        beginTask("reading skin triangles", 10);
        if (!readTriangles(node, os, skeletalVertices))
            return false;
        endTask();

        endTask();

        return true;
    }

    bool readTriangles(INode* node, ObjectState& os, Vector<Vector<SkeletalMesh::VertexWeight>>& skeletalVertices)
    {
        // Convert to a triobject
        auto triObject = static_cast<TriObject*>(os.obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0)));
        if (!triObject)
        {
            LOG_ERROR_WITHOUT_CALLER << "Failed getting TriObject for node: " << node->GetName();
            return false;
        }

        // Get mesh pointer
        auto mesh = &(triObject->GetMesh());
        if (!mesh)
        {
            LOG_ERROR_WITHOUT_CALLER << "Failed getting Mesh for node: " << node->GetName();
            return false;
        }

        // Get material details
        auto material = String();
        auto mtl = node->GetMtl();
        auto isMultiMaterial = false;
        if (mtl)
        {
            isMultiMaterial = (mtl->IsMultiMtl() == TRUE);
            material = mtl->GetName().data();
        }

        // Get node transform to transform the object-space vertices by TODO: is this time right?
        auto transform = node->GetObjTMAfterWSM(ip->GetTime());

        // Create transformed vertex list
        auto vertices = Vector<Vec3>(mesh->getNumVerts());
        for (auto i = 0U; i < uint(mesh->getNumVerts()); i++)
            vertices[i] = maxPoint3ToVec3(transform * mesh->verts[i]);

        // Work out the winding order to use. A mirroring transform on the node will flip the winding order and so we need to
        // compensate
        auto winding = std::array<unsigned int, 3>{{0, 1, 2}};
        if (DotProd(CrossProd(transform.GetRow(0), transform.GetRow(1)), transform.GetRow(2)) < 0.0)
        {
            winding[1] = 2;
            winding[2] = 1;
        }

        // Get access to user-specified mesh normals if they are present
        auto meshNormals = mesh->GetSpecifiedNormals();

        // Construct vertex stream layout
        auto meshVertexStreams = Vector<VertexStream>();
        auto meshVertexSize = 0U;

        meshVertexStreams.emplace(VertexStream::Position, 3);
        meshVertexSize += 12;

        meshVertexStreams.emplace(VertexStream::Bones, 4, TypeUInt8, false);
        meshVertexStreams.emplace(VertexStream::Weights, 4);
        meshVertexSize += 20;

        meshVertexStreams.emplace(VertexStream::DiffuseTextureCoordinate, 2);
        meshVertexSize += 8;

        if (meshNormals)
        {
            meshVertexStreams.emplace(VertexStream::Normal, 3);
            meshVertexSize += 12;
        }

        // Get the triangle array on this set that has the vertex layout needed for this mesh
        auto triangles = triangleSet.findOrCreateArrayByVertexStreamLayout(meshVertexStreams);

        auto newTriangleVertexData = Vector<byte_t>(meshVertexSize * 3);
        auto newTriangleVertices = std::array<byte_t*, 3>{{&newTriangleVertexData[meshVertexSize * winding[0]],
                                                           &newTriangleVertexData[meshVertexSize * winding[1]],
                                                           &newTriangleVertexData[meshVertexSize * winding[2]]}};

        // Get triangles out
        for (auto i = 0U; i < uint(mesh->getNumFaces()); i++)
        {
            // Zero vertex data
            for (auto v : newTriangleVertices)
                memset(v, 0, meshVertexSize);

            auto currentOffset = 0U;

            // Get position data
            for (auto v = 0U; v < 3; v++)
                *reinterpret_cast<Vec3*>(newTriangleVertices[v]) = vertices[mesh->faces[i].v[v]];
            currentOffset += 12;

            // Copy in skeletal vertex stream data
            for (auto j = 0U; j < 3; j++)
            {
                auto& sv = skeletalVertices[mesh->faces[i].v[j]];
                for (auto v = 0U; v < sv.size(); v++)
                {
                    newTriangleVertices[j][currentOffset + v] = sv[v].getBone();
                    *reinterpret_cast<float*>(&newTriangleVertices[j][currentOffset + 4 + v * 4]) = sv[v].getWeight();
                }
            }
            currentOffset += 20;

            // Get ST data out if it's specified
            if (mesh->numTVerts)
            {
                for (auto v = 0U; v < 3; v++)
                {
                    reinterpret_cast<Vec2*>(&newTriangleVertices[v][currentOffset])
                        ->setXY(mesh->tVerts[mesh->tvFace[i].t[v]].x, mesh->tVerts[mesh->tvFace[i].t[v]].y);
                }
            }
            currentOffset += 8;

            // Get user-specified vertex normals
            if (meshNormals)
            {
                for (auto v = 0U; v < 3; v++)
                {
                    *reinterpret_cast<Vec3*>(newTriangleVertices[v][currentOffset]) =
                        maxPoint3ToVec3(meshNormals->GetNormal(i, v));
                }
                currentOffset += 8;
            }

            // If there was a multi-material assigned to this mesh then we need to get the actual submaterial being used on this
            // face
            auto materialName = String();
            if (isMultiMaterial)
            {
                auto subMaterial = mtl->GetSubMtl(mesh->faces[i].getMatID() % mtl->NumSubMtls());
                if (subMaterial)
                    materialName = material + "/" + subMaterial->GetName().data();
                else
                {
                    LOG_WARNING << "Null submaterial found, using parent material name: " << material;
                    materialName = material;
                }
            }
            else
                materialName = material;

            // Fall back if no material was found
            if (materialName.length() == 0)
                materialName = MaterialManager::ExporterNoMaterialFallback;

            if (!triangles->getMaterials().has(materialName))
                LOG_INFO << "New material: " << materialName;

            if (!triangles->addTriangle(newTriangleVertices[0], newTriangleVertices[1], newTriangleVertices[2], materialName))
                return false;

            setTaskProgress(i + 1, mesh->getNumFaces());
        }

        LOG_INFO << "Exported skinned mesh: '" << node->GetName() << "' with " << os.obj->NumPoints() << " vertices and "
                 << mesh->getNumFaces() << " triangles";

        // Delete triobject if it was allocated specifically for us
        if (os.obj != triObject)
            delete triObject;

        return true;
    }

    bool run() override
    {
        beginTask("Reading skeletal mesh structure", 40);
        if (!exportData(*this))
            return false;
        endTask();

        // Copy the bone bind poses across
        for (auto i = 0U; i < bones_.size(); i++)
        {
            if (boneBindPoses.find(boneNodes_[i]) == boneBindPoses.end())
            {
                LOG_WARNING_WITHOUT_CALLER << "No bind pose for bone: " << bones_[i].name;
                continue;
            }

            // Get parent transform if we have one
            auto parentTM = ::Matrix3();
            parentTM.IdentityMatrix();
            if (bones_[i].parent != -1)
            {
                // Use the bind pose for the parent bone if there is one, if not then the frame 0 transform is used instead
                auto parent = boneNodes_[bones_[i].parent];
                if (boneBindPoses.find(parent) != boneBindPoses.end())
                    parentTM = boneBindPoses[parent];
                else
                    parentTM = parent->GetNodeTM(0);
            }

            // Get the local transform for the bind pose on this bone
            auto localTM = boneBindPoses[boneNodes_[i]] * Inverse(parentTM);

            bones_[i].referenceRelative = maxMatrix3ToSimpleTransform(localTM);
        }

        // Put exported data into the skeletal mesh
        beginTask("Compiling", 59);
        auto skeletalMesh = SkeletalMesh();
        if (!skeletalMesh.setup(bones_, triangleSet, *this))
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

        return true;
    }

private:

    UnicodeString filename_;
};

class SkeletalMeshExporter : public SceneExport
{
public:

    unsigned int Version() override { return BuildInfo::getVersion().asInteger(); }
    const char* ShortDesc() override { return SkeletalMeshExporterFileType.cStr(); }
    const char* LongDesc() override { return ShortDesc(); }
    const char* AuthorName() override { return Globals::getDeveloperName().cStr(); }
    const char* CopyrightMessage() override { return ""; }
    const char* OtherMessage1() override { return ""; }
    const char* OtherMessage2() override { return ""; }
    void ShowAbout(HWND hWnd) override {}
    int ExtCount() override { return 1; }

    const char* Ext(int n) override
    {
        static auto const extension = A(SkeletalMesh::SkeletalMeshExtension.substr(1));

        return extension.cStr();
    }

    BOOL SupportsOptions(int ext, DWORD options) override { return options == SCENE_EXPORT_SELECTED; }

    int DoExport(const char* name, ExpInterface* ei, Interface* pIp, BOOL suppressPrompts, DWORD options) override
    {
        ip = pIp;
        onlyExportSelected = (options & SCENE_EXPORT_SELECTED);

        Globals::initializeEngine(getMaxClientName());

        auto runner = SkeletalMeshExportRunner(fixMaxFilename(name, Ext(0)));
        ProgressDialog(SkeletalMeshExporterTitle).show(runner, ip->GetMAXHWnd());

        Globals::uninitializeEngine();

        return 1;
    }
};

class SkeletalMeshExporterClassDesc : public ClassDesc2
{
public:

    int IsPublic() override { return TRUE; }
    void* Create(BOOL loading) override { return new SkeletalMeshExporter; }
    const char* ClassName() override { return "SkeletalMeshExporterClassDesc"; }
    SClass_ID SuperClassID() override { return SCENE_EXPORT_CLASS_ID; }
    Class_ID ClassID() override { return Class_ID(0x69941294, 0x19902818); }
    const char* Category() override { return ""; }
    const char* InternalName() override { return "CarbonSkeletalMeshExporter"; }
    HINSTANCE HInstance() override { return Globals::getHInstance(); }
};

ClassDesc* getSkeletalMeshExporterClassDesc()
{
    static auto desc = SkeletalMeshExporterClassDesc();
    return &desc;
}

}

}

#endif
