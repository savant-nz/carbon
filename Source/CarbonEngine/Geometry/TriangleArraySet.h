/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Manages a set of triangle arrays where each array has a different vertex stream layout.
 */
class CARBON_API TriangleArraySet : private Noncopyable
{
public:

    TriangleArraySet() {}

    /**
     * Copy constructor (not implemented).
     */
    TriangleArraySet(const TriangleArraySet& other);

    ~TriangleArraySet() { clear(); }

    /**
     * Returns an iterator at the start of the vector of triangle arrays.
     */
    const TriangleArray* const* begin() const { return arrays_.begin(); }

    /**
     * Returns an iterator at the end of the vector of triangle arrays.
     */
    const TriangleArray* const* end() const { return arrays_.end(); }

    /**
     * Returns an iterator at the start of the vector of triangle arrays.
     */
    TriangleArray** begin() { return arrays_.begin(); }

    /**
     * Returns an iterator at the end of the vector of triangle arrays.
     */
    TriangleArray** end() { return arrays_.end(); }

    /**
     * Returns the number of triangle arrays in this set.
     */
    unsigned int size() const { return arrays_.size(); }

    /**
     * Returns whether this triangle array set is empty.
     */
    bool empty() const { return arrays_.empty(); }

    /**
     * Returns the triangle array at the given index in this set.
     */
    TriangleArray& operator[](unsigned int index) { return *arrays_[index]; }

    /**
     * Returns the triangle array at the given index in this set.
     */
    const TriangleArray& operator[](unsigned int index) const { return *arrays_[index]; }

    /**
     * Returns the triangle array in this set that has the given vertex stream layout, or if no such triangle array
     * exists then one is created with the given vertex stream layout.
     */
    TriangleArray* findOrCreateArrayByVertexStreamLayout(const Vector<VertexStream>& streams);

    /**
     * Adds a new triangle array to this set.
     */
    void append(TriangleArray* array) { arrays_.append(array); }

    /**
     * Returns the total number of triangles in all the triangle arrays in this set.
     */
    unsigned int getTriangleCount() const;

    /**
     * Clears the contents of this triangle set and all of its triangle arrays.
     */
    void clear();

    /**
     * Removes the given triangle array from this set and erases all its contents. Returns success flag.
     */
    bool remove(TriangleArray* array);

    /**
     * Transforms all the vertex position data in this triangle set's triangle arrays by the given 4x4 matrix. Returns
     * success flag.
     */
    bool transformPositionData(const Matrix4& transform);

    /**
     * Transfers the contents of this set to another set and then clears this set.
     */
    void transfer(TriangleArraySet& target);

    /**
     * Returns a list of all the unique materials referenced by the triangles in this set.
     */
    Vector<String> getMaterials() const;

private:

    Vector<TriangleArray*> arrays_;
};

}
