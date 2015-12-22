/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Scene/Mesh/Mesh.h"
#include "CarbonEngine/Scene/Mesh/MeshFormatRegistry.h"
#include "CarbonEngine/Scene/Mesh/MeshManager.h"

namespace Carbon
{

MeshManager::~MeshManager()
{
    for (auto mesh : meshes_)
    {
        LOG_WARNING << "Unreleased mesh: " << mesh->getName() << ", reference count: " << mesh->referenceCount_;
        delete mesh;
    }

    meshes_.clear();
}

const Mesh* MeshManager::getMesh(const String& name)
{
    // Check whether this mesh has already been loaded
    auto mesh = meshes_.detect([&](const Mesh* m) { return m->getName() == name; }, nullptr);
    if (mesh)
    {
        mesh->referenceCount_++;
        return mesh;
    }

    // Create and load the new mesh
    mesh = new Mesh;
    if (!MeshFormatRegistry::loadMeshFile(Mesh::MeshDirectory + name, *mesh))
        LOG_ERROR << "Failed loading mesh: " << name;

    meshes_.append(mesh);
    mesh->setName(name);

    return mesh;
}

Mesh* MeshManager::createMesh()
{
    meshes_.append(new Mesh);
    return meshes_.back();
}

void MeshManager::releaseMesh(const Mesh* m)
{
    if (!m)
        return;

    auto mesh = const_cast<Mesh*>(m);

    if (mesh->referenceCount_ < 2)
    {
        meshes_.unorderedEraseValue(mesh);

        delete mesh;
        mesh = nullptr;
    }
    else
        mesh->referenceCount_--;
}

bool MeshManager::convertMeshToNativeFormat(const String& name)
{
    auto mesh = Mesh();
    if (!MeshFormatRegistry::loadMeshFile(Mesh::MeshDirectory + name, mesh))
    {
        LOG_ERROR << "Failed converting mesh: " << name;
        return false;
    }

    try
    {
        auto file = FileWriter();
        fileSystem().open(Mesh::MeshDirectory + name + Mesh::MeshExtension, file);
        file.write(mesh);
    }
    catch (const Exception& e)
    {
        LOG_ERROR << name << " - " << e;
        return false;
    }

    return true;
}

}
