/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Plane.h"

namespace Carbon
{

/**
 * A bare-bones triangle class that just has three points that define a triangle.
 */
class CARBON_API RawTriangle
{
public:

    RawTriangle() {}

    /**
     * Constructor that initializes the three vertices of this triangle
     */
    RawTriangle(const Vec3& v0, const Vec3& v1, const Vec3& v2) : vertices_{{v0, v1, v2}} {}

    /**
     * Returns the specified vertex of this triangle.
     */
    const Vec3& getVertex(unsigned int index) const { return vertices_[index]; }

    /**
     * Sets the specified vertex of this triangle.
     */
    void setVertex(unsigned int index, const Vec3& v) { vertices_[index] = v; }

    /**
     * Calculates and returns the normal of this triangle.
     */
    Vec3 getNormal() const { return ((vertices_[2] - vertices_[0]).cross(vertices_[1] - vertices_[0])).normalized(); }

    /**
     * Classifies this triangle against a plane.
     */
    Plane::ClassifyResult classify(const Plane& plane) const
    {
        return plane.classify(vertices_[0], vertices_[1], vertices_[2]);
    }

    /**
     * Saves this triangle to a file stream.
     */
    void save(FileWriter& file) const { file.write(vertices_); }

    /**
     * Loads this triangle from a file stream.
     */
    void load(FileReader& file) { file.read(vertices_); }

private:

    std::array<Vec3, 3> vertices_;
};

/**
 * A bare-bones indexed triangle class that just has three indices that define a triangle.
 */
class CARBON_API RawIndexedTriangle
{
public:

    RawIndexedTriangle() {}

    /**
     * Constructor that initializes the three index values.
     */
    RawIndexedTriangle(unsigned int index0, unsigned int index1, unsigned index2) : indices_{{index0, index1, index2}}
    {
    }

    /**
     * Offsets this triangle's indices by the specified amount.
     */
    RawIndexedTriangle operator+(unsigned int offset) const
    {
        return {indices_[0] + offset, indices_[1] + offset, indices_[2] + offset};
    }

    /**
     * Returns the three indices that define this indexed triangle.
     */
    const std::array<unsigned int, 3>& getIndices() const { return indices_; }

    /**
     * Returns the specified index of this triangle.
     */
    unsigned int getIndex(unsigned int index) const { return indices_[index]; }

    /**
     * Sets the specified index of this triangle.
     */
    void setIndex(unsigned int index, unsigned int value) { indices_[index] = value; }

    /**
     * Saves this indexed triangle to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(indices_); }

    /**
     * Loads this indexed triangle from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file) { file.read(indices_); }

private:

    std::array<unsigned int, 3> indices_ = {};
};

/**
 * Triangle class, used primarily in mesh compilers and exporters.
 */
class CARBON_API Triangle
{
public:

    Triangle() {}

    /**
     * Constructs this triangle to point to the specified vertices in the given triangle array.
     */
    Triangle(TriangleArray* array, unsigned int index0, unsigned int index1, unsigned int index2,
             const String& material = String::Empty, const String& lightmap = String::Empty)
        : indices_{{index0, index1, index2}}, array_(array), material_(material), lightmap_(lightmap)
    {
    }

    /**
     * Returns this triangle's indices, these index into the vertex array data on the TriangleArray class that this
     * triangle is being managed by.
     */
    const std::array<unsigned int, 3>& getIndices() const { return indices_; }

    /**
     * Returns the specified index of this triangle.
     */
    unsigned int getIndex(unsigned int index) const { return indices_[index]; }

    /**
     * Returns the specified index of this triangle.
     */
    void setIndex(unsigned int index, unsigned int value) { indices_[index] = value; }

    /**
     * Returns this triangle's material.
     */
    const String& getMaterial() const { return material_; }

    /**
     * Sets this triangle's material.
     */
    void setMaterial(const String& material) { material_ = material; }

    /**
     * Returns this lightmap's material.
     */
    const String& getLightmap() const { return lightmap_; }

    /**
     * Sets this triangle's lightmap.
     */
    void setLightmap(const String& lightmap) { lightmap_ = lightmap; }

    /**
     * Returns the triangle array that this triangle is part of, or null if this triangle is not in a triangle array,
     */
    const TriangleArray* getParentTriangleArray() const { return array_; }

    /**
     * Returns a pointer to the vertex data referenced by the given index in this triangle. Returns null if this
     * triangle is not currently in a triangle array.
     */
    const byte_t* getVertexData(unsigned int v) const;

    /**
     * Returns the position of a vertex in this triangle. Returns a zero vector if this triangle is not currently in a
     * triangle array.
     */
    const Vec3& getVertexPosition(unsigned int v) const;

    /**
     * Returns the vertex data referenced by the given vertex of this triangle in a vector. Returns an empty vector if
     * this triangle is not currently in a triangle array.
     */
    Vector<byte_t> copyVertexData(unsigned int v) const;

    /**
     * Calculates and returns the normal of this triangle.
     */
    Vec3 getNormal() const;

    /**
     * Splits this triangle by a plane, and returns the resultant pieces. The total number of triangles after splitting
     * will be no less than one and no more than three. The resulting pieces only have the vertices set, they have no
     * material or lightmap data set.
     */
    void split(const Plane& plane, TriangleArray& frontPieces, TriangleArray& backPieces) const;

    /**
     * Calculates the area of this triangle.
     */
    float calculateArea() const;

    /**
     * Classifies this triangle against a plane.
     */
    Plane::ClassifyResult classify(const Plane& plane) const;

private:

    std::array<unsigned int, 3> indices_ = {};

    TriangleArray* array_ = nullptr;

    String material_;
    String lightmap_;

    friend class TriangleArray;
};

}
