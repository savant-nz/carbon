/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CARBON_INCLUDE_OPENASSETIMPORT

#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Geometry/TriangleArraySet.h"

#include "CarbonEngine/Scene/Mesh/OpenAssetImportIncludeWrapper.h"

#ifndef CARBON_INCLUDE_ZLIB
    #error Using Open Asset Import requires that ZLib is included in the build
#endif

#ifdef _MSC_VER
    #pragma comment(lib, "OpenAssetImport" CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX)
    #pragma comment(lib, "ZLib" CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX)
#endif

namespace Carbon
{

using namespace Assimp;

/**
 * Adds support for importing a number of different mesh formats through the Open Asset Import Library.
 */
class OpenAssetImport
{
public:

    static const auto ProcessingFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_SortByPType;

    static bool load(FileReader& file, Mesh& mesh)
    {
        try
        {
            auto importer = Assimp::Importer();

            auto fileDataStorage = Vector<byte_t>();
            auto fileData = file.getData(fileDataStorage);

            auto scene = importer.ReadFileFromMemory(fileData, file.getSize(), ProcessingFlags);

            if (!scene || !scene->HasMeshes())
                throw Exception("No mesh data found in this file");

            // Read material names
            auto materialNames = Vector<String>(scene->mNumMaterials);
            for (auto i = 0U; i < materialNames.size(); i++)
            {
                auto name = aiString();
                scene->mMaterials[i]->Get(AI_MATKEY_NAME, name);
                materialNames[i] = name.C_Str();
            }

            // Vertex stream layout, only the basics are imported at present
            auto vertexStreams = Vector<VertexStream>{
                {VertexStream::Position, 3}, {VertexStream::DiffuseTextureCoordinate, 2}, {VertexStream::Normal, 3}};

            // Temporary storage for vertex data during import
            auto triangleSet = TriangleArraySet();
            auto triangles = triangleSet.findOrCreateArrayByVertexStreamLayout(vertexStreams);
            auto vertices = std::array<Vertex, 3>();

            // Loop over meshes
            for (auto i = 0U; i < scene->mNumMeshes; i++)
            {
                auto m = scene->mMeshes[i];

                if (m->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
                {
                    LOG_WARNING << "Skipping non-triangle mesh";
                    continue;
                }

                // Check whether this mesh has texture coordinates
                auto hasDiffuseTextureCoordinates = (m->mNumUVComponents[0] == 2);

                // Reserve space for this mesh's triangles
                triangles->reserve(triangles->size() + m->mNumFaces);

                // Copy triangle data from the mesh into triangles
                for (auto j = 0U; j < m->mNumFaces; j++)
                {
                    auto& face = m->mFaces[j];

                    for (auto k = 0U; k < 3; k++)
                    {
                        auto index = face.mIndices[k];

                        vertices[k].position = *reinterpret_cast<const Vec3*>(&m->mVertices[index]);
                        vertices[k].normal = *reinterpret_cast<const Vec3*>(&m->mNormals[index]);

                        if (hasDiffuseTextureCoordinates)
                            vertices[k].diffuseTextureCoordinate = *reinterpret_cast<const Vec2*>(&m->mTextureCoords[0][index]);
                    }

                    triangles->addTriangle(&vertices[0], &vertices[1], &vertices[2], materialNames[m->mMaterialIndex]);
                }
            }

            // Create a native mesh from the triangles
            return mesh.setupFromTriangles(triangleSet);
        }
        catch (const Exception& e)
        {
            LOG_ERROR << file.getName() << " - " << e;

            return false;
        }
    }

private:

    struct Vertex
    {
        Vec3 position;
        Vec2 diffuseTextureCoordinate;
        Vec3 normal;
    };
};

CARBON_REGISTER_MESH_FILE_FORMAT(3d, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(3ds, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(ac, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(ac3d, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(acc, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(ase, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(ask, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(b3d, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(bvh, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(cob, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(csm, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(enff, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(hmp, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(irr, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(irrmesh, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(lwo, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(lws, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(lxo, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(m3, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(md2, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(md3, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(md5anim, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(md5camera, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(md5mesh, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(mdc, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(mdl, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(mot, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(ms3d, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(ndo, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(nff, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(obj, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(off, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(ply, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(prj, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(q3o, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(q3s, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(raw, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(scn, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(smd, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(stl, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(ter, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(uc, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(vta, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(x, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(xgl, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(xml, OpenAssetImport::load, nullptr)
CARBON_REGISTER_MESH_FILE_FORMAT(zgl, OpenAssetImport::load, nullptr)

}

#endif
