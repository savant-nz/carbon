/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Manages all the mesh objects in the engine, meshes are reference counted and are loaded through MeshFormatRegistry.
 */
class CARBON_API MeshManager : private Noncopyable
{
public:

    /**
     * Returns the mesh object with the given name. This will either trigger a mesh load or return a pointer to an already
     * loaded mesh and increase its reference count.
     */
    const Mesh* getMesh(const String& name);

    /**
     * Creates a new empty mesh with a reference count of 1, this reference mesh should be released
     */
    Mesh* createMesh();

    /**
     * Releases a mesh reference taken either by MeshManager::getMesh() or MeshManager::createMesh(), if the reference count for
     * a mesh reaches zero then it is unloaded.
     */
    void releaseMesh(const Mesh* m);

    /**
     * Takes a mesh that is not stored in Carbon's native format, loads it, and then saves it back out to the local file system
     * in the native mesh format. This is useful to avoid additional mesh processing that occurs when loading non-native meshes
     * at runtime, as the native version can then be used instead of the original source mesh file. Returns success flag.
     */
    bool convertMeshToNativeFormat(const String& name);

private:

    Vector<Mesh*> meshes_;

    MeshManager() {}
    ~MeshManager();
    friend class Globals;
};

}
