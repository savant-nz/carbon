/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Runnable.h"
#include "CarbonEngine/Exporters/ExportInfo.h"
#include "CarbonEngine/Physics/PhysicsInterface.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Scene/IntersectionResult.h"

namespace Carbon
{

/**
 * Holds geometry and material information for a single mesh. This is the base container for all static geometry. Meshes are
 * loaded through MeshFormatRegistry and then attached to entities for rendering using Entity::attachMesh().
 */
class CARBON_API Mesh : private Noncopyable
{
public:

    Mesh() {}

    /**
     * Copy constructor (not implemented).
     */
    Mesh(const Mesh& other);

    ~Mesh() { clear(); }

    /**
     * A mesh is composed of a number of mesh components and each mesh component consists of a material and a geometry chunk.
     */
    class MeshComponent
    {
    public:

        /**
         * Returns the material to use to render this mesh component.
         */
        const String& getMaterial() const { return material_; }

        /**
         * Returns the geometry chunk for this mesh component.
         */
        const GeometryChunk& getGeometryChunk() const { return geometryChunk_; }

        /**
         * Saves this mesh component to a file stream.
         */
        void save(FileWriter& file) const { file.write(material_, geometryChunk_); }

        /**
         * Loads this mesh component from a file stream.
         */
        void load(FileReader& file) { file.read(material_, geometryChunk_); }

    private:

        String material_;
        GeometryChunk geometryChunk_;

        friend class Mesh;
    };

    /**
     * The directory which meshes are stored under, currently "Meshes/".
     */
    static const UnicodeString MeshDirectory;

    /**
     * The file extension for meshes, currently ".mesh".
     */
    static const UnicodeString MeshExtension;

    /**
     * Returns the name of this mesh.
     */
    const String& getName() const { return name_; }

    /**
     * Sets the name of this mesh.
     */
    void setName(const String& name) { name_ = name; }

    /**
     * Erases all the geometry stored in this mesh.
     */
    void clear();

    /**
     * Sets up this mesh from the given triangle set.
     */
    bool setupFromTriangles(TriangleArraySet& triangleSet, Runnable& r = Runnable::Empty);

    /**
     * Extracts all the triangles stored in this mesh. Returns success flag.
     */
    bool getTriangles(TriangleArraySet& triangleSet) const;

    /**
     * Sets the specified parameter value on all the mesh component geometry chunks.
     */
    void setParameter(const String& name, const Parameter& value);

    /**
     * Saves this mesh to a file stream.
     */
    void save(FileWriter& file) const;

    /**
     * Saves this mesh to a mesh file. Returns succes flag.
     */
    bool save(const String& name) const;

    /**
     * Loads this mesh from a file stream.
     */
    void load(FileReader& file);

    /**
     * Returns all the intersections of the given ray with this mesh.
     */
    void intersectRay(const Ray& ray, Vector<IntersectionResult>& results) const;

    /**
     * Returns the vector of mesh components that make up this mesh.
     */
    const Vector<MeshComponent>& getMeshComponents() const { return meshComponents_; }

    /**
     * Returns the triangle count of this mesh.
     */
    unsigned int getTriangleCount() const;

    /**
     * Returns an AABB that encloses this mesh.
     */
    AABB getAABB() const;

    /**
     * Returns a bounding sphere that encloses this mesh.
     */
    Sphere getSphere() const;

    /**
     * Returns the physics body template for this mesh. If one has not been created then it will created automatically by
     * calling this method. Returns null on failure.
     */
    PhysicsInterface::BodyTemplateObject getPhysicsBodyTemplate() const;

private:

    String name_;
    Vector<MeshComponent> meshComponents_;

    mutable PhysicsInterface::BodyTemplateObject physicsBodyTemplate_ = nullptr;

    friend class MeshManager;
    unsigned int referenceCount_ = 1;
};

}
