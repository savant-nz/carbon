/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Runnable.h"
#include "CarbonEngine/Geometry/Triangle.h"
#include "CarbonEngine/Render/GeometryChunk.h"

namespace Carbon
{

/**
 * Manages an array of triangles which all have the same vertex layout.
 */
class CARBON_API TriangleArray : private Noncopyable
{
public:

    TriangleArray() { clear(); }

    /**
     * Copy constructor (not implemented).
     */
    TriangleArray(const TriangleArray& other);

    ~TriangleArray() { clear(); }

    /**
     * Returns the triangle at the given index in this triangle array.
     */
    const Triangle& operator[](unsigned int index) const { return triangles_[index]; }

    /**
     * Returns the triangle at the given index in this triangle array.
     */
    Triangle& operator[](unsigned int index) { return triangles_[index]; }

    /**
     * Erases the contents of this triangle array.
     */
    void clear();

    /**
     * Returns an iterator at the start of the vector of triangles.
     */
    const Triangle* begin() const { return triangles_.begin(); }

    /**
     * Returns an iterator at the end of the vector of triangles.
     */
    const Triangle* end() const { return triangles_.end(); }

    /**
     * Adds a triangle to this array that uses the given 3 vertex indices. Returns success flag.
     */
    bool addTriangle(unsigned int index0, unsigned int index1, unsigned int index2,
                     const String& material = String::Empty, const String& lightmap = String::Empty);

    /**
     * Adds a triangle to this array based on an existing triangle in another array. Returns success flag.
     */
    bool addTriangle(const Triangle& triangle);

    /**
     * Adds a triangle to this array with the given vertex data, material and lightmap. The vertex data pointers must
     * point to data that is correctly laid out for this triangle array. Returns success flag.
     */
    bool addTriangle(const void* v0, const void* v1, const void* v2, const String& material = String::Empty,
                     const String& lightmap = String::Empty);

    /**
     * Returns the number of triangles in this triangle array.
     */
    unsigned int size() const { return triangles_.size(); }

    /**
     * Returns whether this triangle array is empty.
     */
    bool empty() const { return triangles_.empty(); }

    /**
     * Erases the triangle at the given index.
     */
    void erase(unsigned int index) { triangles_.erase(index); }

    /**
     * Reserves storage space for the specified number of triangles, this is a hint to the triangle array that can help
     * avoid unnecessary internal resizing and copying while a triangle array is being constructed.
     */
    void reserve(unsigned int size) { triangles_.reserve(size); }

    /**
     * Removes any degenerate and malformed triangles from this triangle array.
     */
    bool removeDegenerateTriangles(Runnable& r = Runnable::Empty);

    /**
     * Appends the given array of triangles to this triangle array. Note that this will result in a complete second copy
     * of all the triangle data which can be avoided by using the TriangleArray::transferTriangle() method instead.
     * Returns success flag.
     */
    bool append(const TriangleArray& triangles);

    /**
     * Returns the vertex streams that define the vertex layout used by triangles in this array.
     */
    const Vector<VertexStream>& getVertexStreams() const { return vertexDataGeometryChunk_.getVertexStreams(); }

    /**
     * Sets the vertex streams to use for this array. This can only be called on empty triangle arrays.
     */
    bool setVertexStreams(const Vector<VertexStream>& streams);

    /**
     * Returns whether the vertex data in this triangle array contains the given vertex stream type.
     */
    bool hasVertexStream(unsigned int streamType) const { return vertexDataGeometryChunk_.hasVertexStream(streamType); }

    /**
     * Returns the vertex data at the given index.
     */
    const byte_t* getVertexData(unsigned int index) const;

    /**
     * Returns the internal geometry chunk that is used to store this triangle array's vertex data.
     */
    const GeometryChunk& getVertexDataGeometryChunk() const { return vertexDataGeometryChunk_; }

    /**
     * Copies the vertex stream layout and vertex data from a geometry chunk into the vertex data chunk for this
     * triangle array.
     */
    bool copyVertexStreamsAndDataFromGeometryChunk(const GeometryChunk& geometryChunk);

    /**
     * Returns a list of all the unique materials referenced by the triangles in this array.
     */
    Vector<String> getMaterials() const;

    /**
     * Returns a list of all the unique lightmaps referenced by the triangles in this array.
     */
    Vector<String> getLightmaps() const;

private:

    friend class Triangle;
    friend class TriangleArraySet;

    Vector<Triangle> triangles_;

    // The vertex stream description and all vertex data referenced by the triangles in this array is stored in a
    // geometry chunk
    GeometryChunk vertexDataGeometryChunk_;
    unsigned int usedVertexCount_ = 0;
};

}
