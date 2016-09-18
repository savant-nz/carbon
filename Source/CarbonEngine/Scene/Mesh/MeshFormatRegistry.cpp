/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Scene/Mesh/Mesh.h"
#include "CarbonEngine/Scene/Mesh/MeshFormatRegistry.h"

namespace Carbon
{

CARBON_DEFINE_FILE_FORMAT_REGISTRY(ReadMeshFormatFunction, WriteMeshFormatFunction)

bool MeshFormatRegistry::loadMeshFile(const UnicodeString& filename, Mesh& mesh)
{
    auto file = FileReader();
    auto fnReader = loadFile(filename, file);

    return fnReader && fnReader(file, mesh);
}

bool MeshFormatRegistry::saveMeshFile(const UnicodeString& filename, const Mesh& mesh)
{
    auto file = FileWriter();
    auto fnWriter = saveFile(filename, file);

    return fnWriter && fnWriter(file, mesh);
}

// This class hooks the native Mesh class save and load up to the MeshFormatRegistry mechanism so that .mesh files save
// and load like all other supported mesh formats.
class NativeMesh
{
public:

    static bool load(FileReader& file, Mesh& mesh)
    {
        try
        {
            mesh.load(file);
            return true;
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;
            return false;
        }
    }

    static bool save(FileWriter& file, const Mesh& mesh)
    {
        try
        {
            mesh.save(file);
            return true;
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;
            return false;
        }
    }
};

CARBON_REGISTER_MESH_FILE_FORMAT(mesh, NativeMesh::load, NativeMesh::save)

}

#include "CarbonEngine/Scene/Mesh/OpenAssetImport.h"
