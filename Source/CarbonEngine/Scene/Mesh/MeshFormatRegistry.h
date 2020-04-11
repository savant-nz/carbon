/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/FileFormatRegistry.h"

namespace Carbon
{

/**
 * Typedef for a mesh file reading function.
 */
typedef std::function<bool(FileReader& file, Mesh& mesh)> ReadMeshFormatFunction;

/**
 * Typedef for a mesh file writing function.
 */
typedef std::function<bool(FileWriter& file, const Mesh& mesh)> WriteMeshFormatFunction;

/**
 * Handles the registration of supported mesh formats and provides access to the reading and writing functions for each
 * supported format. Mesh formats can be registered with the CARBON_REGISTER_MESH_FILE_FORMAT() macro.
 */
class CARBON_API MeshFormatRegistry : public FileFormatRegistry<ReadMeshFormatFunction, WriteMeshFormatFunction>
{
public:

    /**
     * Given a filename that may or may not have an extension this method tries to load a mesh out of it. If the give
     * filename contains an extension then that format will be assumed, otherwise the filesystem will be searched for a
     * matching filename with an extension that has a reader function available. If one is found then it will be used to
     * read the mesh. Returns success flag.
     */
    static bool loadMeshFile(const UnicodeString& filename, Mesh& mesh);

    /**
     * Saves the passed mesh to a file, the format of the mesh file is determined by the extension present on the passed
     * filename. Note that some supported mesh formats do not support all of the features of the Mesh class, and so
     * certain parts of the mesh data may be skipped depending on the target format. The only mesh format which is
     * guaranteed to be able to persist a Mesh instance without losing any data is the native '.mesh' format, this is
     * because the '.mesh' format is a memory dump of the entire contents of a Mesh instance. However, because it is a
     * native format, '.mesh' files are not supported by other mesh viewing or editing applications. Returns success
     * flag.
     */
    static bool saveMeshFile(const UnicodeString& filename, const Mesh& mesh);
};

CARBON_DECLARE_FILE_FORMAT_REGISTRY(ReadMeshFormatFunction, WriteMeshFormatFunction);

/**
 * \file
 */

/**
 * Registers reading and writing functions for the mesh file format with the given extension. If a null function pointer
 * is specified it will be ignored.
 */
#define CARBON_REGISTER_MESH_FILE_FORMAT(Extension, ReaderFunction, WriterFunction) \
    CARBON_REGISTER_FILE_FORMAT(Carbon::MeshFormatRegistry, Extension, ReaderFunction, WriterFunction)
}
