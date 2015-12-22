/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Core/Runnable.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Math/AABB.h"
#include "CarbonEngine/Math/Sphere.h"
#include "CarbonEngine/Math/Vec3.h"
#include "CarbonEngine/Render/DataBufferManager.h"
#include "CarbonEngine/Render/DrawItem.h"
#include "CarbonEngine/Render/VertexStream.h"

namespace Carbon
{

/**
 * Storage for geometry data that can be used in rendering. A geometry chunk is a set of arbitrary vertex data and index data
 * that describes a piece or pieces of geometry. Vertex data layout is described by a set of packed vertex streams (see the
 * VertexStream class). Index data layout is described by a list of draw items (see the DrawItem class for more information).
 * The methods on this class are structured such that it can never be put into an inconsistent state. This class also contains a
 * number of methods for processing geometry data.
 */
class CARBON_API GeometryChunk : public EventHandler
{
public:

    /**
     * The version info used when persisting a geometry chunk.
     */
    static const VersionInfo GeometryChunkVersionInfo;

    /**
     * An empty geometry chunk.
     */
    static const GeometryChunk Empty;

    GeometryChunk();

    /**
     * Copy constructor.
     */
    GeometryChunk(const GeometryChunk& other);

    /**
     * Move constructor.
     */
    GeometryChunk(GeometryChunk&& other) noexcept;

    ~GeometryChunk() override;

    /**
     * Assignment operator.
     */
    GeometryChunk& operator=(GeometryChunk other)
    {
        swap(*this, other);
        return *this;
    }

    /**
     * Swaps the contents of two GeometryChunk instances.
     */
    friend void swap(GeometryChunk& first, GeometryChunk& second);

    /**
     * Returns the array of vertex streams.
     */
    const Vector<VertexStream>& getVertexStreams() const { return vertexStreams_; }

    /**
     * Returns the VertexStream instance for the stream of the given type.
     */
    const VertexStream& getVertexStream(unsigned int streamType) const;

    /**
     * Returns whether or not this chunk has a vertex stream of the given type.
     */
    bool hasVertexStream(unsigned int streamType) const;

    /**
     * Returns the number of vertices in this geometry chunk.
     */
    unsigned int getVertexCount() const { return vertexCount_; }

    /**
     * Returns the size in bytes of a single vertex in this geometry chunk.
     */
    unsigned int getVertexSize() const { return vertexSize_; }

    /**
     * Returns the internal vertex data byte array which stores the vertex data for this chunk.
     */
    const byte_t* getVertexData() const { return vertexData_.getData(); }

    /**
     * Returns the size of the vertex data array in bytes, given by the number of vertices multiplied by the size in bytes of
     * each vertex.
     */
    unsigned int getVertexDataSize() const { return vertexCount_ * vertexSize_; }

    /**
     * Returns the drawitems of this geometry chunk.
     */
    const Vector<DrawItem>& getDrawItems() const { return drawItems_; }

    /**
     * Returns the type of the index data. Will be one of TypeUInt16 or TypeUInt32. The chunk will automatically use the most
     * compact index representation that it can given the number of vertices it contains.
     */
    DataType getIndexDataType() const { return indexDataType_; }

    /**
     * Returns the total number of indices stored in this chunk.
     */
    unsigned int getIndexCount() const { return indexData_.size() / getDataTypeSize(indexDataType_); }

    /**
     * Returns the size of the index data array in bytes. This is given by the number of indices multiplied by the size in bytes
     * of each index.
     */
    unsigned int getIndexDataSize() const { return indexData_.size(); }

    /**
     * Returns a pointer to the internal index data of this geometry chunk. This will be an array of either unsigned 16 or
     * 32-bit integers, depending on the current index data type.
     */
    const byte_t* getIndexData() const { return indexData_.getData(); }

    /**
     * Returns the index data data buffer allocation for this geometry chunk's index data. Will be null if this chunk is not
     * currently registered with the renderer. This is set by the GeometryChunk::registerWithRenderer() method.
     */
    DataBufferManager::AllocationObject getIndexAllocation() const { return indexAllocation_; }

    /**
     * Returns whether this geometry chunk is flagged as having dynamic contents. This indicates that the vertex data of this
     * chunk is going to be updated regularly.
     */
    bool isDynamic() const { return isDynamic_; }

    /**
     * Sets whether this chunk is dynamic, see GeometryChunk::isDynamic() for more information.
     */
    bool setDynamic(bool dynamic);

    /**
     * Clears the contents of this geometry chunk and releases allocated memory.
     */
    void clear();

    /**
     * Locks the vertex data of this geometry chunk and returns a pointer to it that can be used to alter the vertex data
     * contents. Once the vertex data has been altered as required, GeometryChunk::unlockVertexData() should be called
     * immediately to free the lock. Geometry chunks that are going to be altered frequently at runtime should be set as dynamic
     * (see GeometryChunk::setDynamic()) so that the renderer can optimize for dynamic geometry.
     */
    byte_t* lockVertexData();

    /**
     * This is the same as GeometryChunk::lockVertexData() but additionally it casts the returned vertex data pointer to the
     * specified type, which saves a cast having to be done by the caller.
     */
    template <typename VertexType> VertexType* lockVertexData() { return reinterpret_cast<VertexType*>(lockVertexData()); }

    /**
     * Signals the end of a corresponding GeometryChunk::lockVertexData() call. After this call, the pointer returned by
     * GeometryChunk::lockVertexData() is no longer valid.
     */
    void unlockVertexData();

    /**
     * Returns whether the vertex data is currently locked.
     */
    bool isVertexDataLocked() const { return isVertexDataLocked_; }

    /**
     * WHen this geometry chunk's vertex data is locked for alteration by GeometryChunk::lockVertexData() then this method will
     * return the same pointer that was returned by the initial call to GeometryChunk::lockVertexData(). Returns null if this
     * chunk's vertex data is not currently locked.
     */
    template <typename T = byte_t> T* getLockedVertexDataPointer()
    {
        return isVertexDataLocked_ ? reinterpret_cast<T*>(vertexData_.getData()) : nullptr;
    }

    /**
     * Sets the number of vertices in this chunk. Returns false if the new vertex count would make any of the indices point to a
     * non-existent vertex, otherwise true.
     */
    bool setVertexCount(unsigned int newVertexCount, bool preserveData = true);

    /**
     * Goes through all the vertices on this chunk and checks that their position is within a sensible range for geometry. If a
     * bad vertex is found a warning is issued and false is returned. Bad vertex positions are those where any component is not
     * a real number in the range +/- one million.
     */
    bool validateVertexPositionData() const;

    /**
     * Removes any unused or duplicated vertices in this geometry chunk. Returns success flag.
     */
    bool optimizeVertexData(Runnable& r = Runnable::Empty);

    /**
     * Returns the index data value at the given index. If the given index is out of range then an error will be printed out and
     * zero will be returned.
     */
    unsigned int getIndexValue(unsigned int index) const;

    /**
     * Sets the index data value at the given index. If the given index is out of range or the new value exceeds the number of
     * vertices in this chunk then an error is logged and false is returned. If the value is set successfully then true is
     * returned.
     */
    bool setIndexValue(unsigned int index, unsigned int value);

    /**
     * Sets up the index data on this chunk from a list of drawitems and index data. The supplied draw items and indices are
     * validated and if any problems are found then an error is logged and false is returned, otherwise the new data is set up
     * and true is returned.
     */
    bool setupIndexData(const Vector<DrawItem>& newDrawItems, const Vector<unsigned int>& newIndices);

    /**
     * Adds the passed draw item to this geometry chunk, returns success flag.
     */
    bool appendDrawItem(const DrawItem& drawItem);

    /**
     * Removes all draw items from this geometry chunk, this does not change this chunk's index data.
     */
    void clearDrawItems();

    /**
     * Returns a copy of this chunk's index data.
     */
    Vector<unsigned int> copyIndexData() const;

    /**
     * Sets all the index values in this geometry chunk to be a direct 1-1 matching such that index 0 is 0, index 1 is 1, index
     * 2 is 2, and so on. This is useful when setting up a geometry chunk that is initially a simple list of vertices, three for
     * each triangle, prior to further processing. A single drawitem for the list of triangles is also setup. Returns success
     * flag.
     */
    bool setIndexDataStraight();

    /**
     * Triangle strips the indices of this geometry chunk for more efficient storage and rendering. This process can take some
     * time depending on how many triangles this chunk has.
     */
    bool generateTriangleStrips(Runnable& r = Runnable::Empty);

    /**
     * Adds a vertex stream to this geometry chunk. There can only be one vertex stream of each type, so if a stream of the
     * given type already exists then this routine does nothing. Otherwise the new vertex stream is appended to the vertex data
     * and the return value is true.
     */
    bool addVertexStream(const VertexStream& vertexStream);

    /**
     * Removes a vertex stream from this geometry chunk. Returns success flag.
     */
    bool deleteVertexStream(unsigned int streamType);

    /**
     * Directly sets the vertex streams on this chunk that describe the layout of the vertex data. This method only works with
     * chunks that have no vertices and so is usually used in an initial setup phase. Returns success flag.
     */
    bool setVertexStreams(const Vector<VertexStream>& vertexStreams);

    /**
     * Transforms the specified 3xfloat vertex tream by the given 4x4 transform matrix. Returns success flag.
     */
    bool transformVertexStream(unsigned int streamType, const Matrix4& transform);

    /**
     * Calculates tangent bases for the triangles and vertices in this geometry chunk. This method requires that the passed
     * chunk has position and diffuse texture coordinate vertex streams, and it will add VertexStream::Tangent and
     * VertexStream::Bitangent vertex streams. Returns success flag.
     */
    bool calculateTangentBases();

    /**
     * Registers this geometry chunk with the renderer so it can be rendered, unregistered geometry chunks will be skipped by
     * the renderer. Returns success flag.
     */
    bool registerWithRenderer();

    /**
     * Unregisters this geometry chunk from the renderer. Returns success flag
     */
    bool unregisterWithRenderer();

    /**
     * Returns whether this geometry chunk is currently registered with the renderer.
     */
    bool isRegisteredWithRenderer() const { return vertexAllocation_ && indexAllocation_; }

    /**
     * Returns the number of triangles in this geometry chunk. This is calculated by adding together all the individual
     * drawitems that are triangle lists or strips.
     */
    unsigned int getTriangleCount() const;

    /**
     * Copies this geometry chunk's data into a single triangle array.
     */
    bool getTriangles(TriangleArray& triangles) const;

    /**
     * Returns the AABB for this geometry chunk.
     */
    const AABB& getAABB() const;

    /**
     * Returns the bounding sphere for this geometry chunk.
     */
    const Sphere& getSphere() const;

    /**
     * Returns the plane for this geometry chunk. Some rendering methods such as water surfaces assume they are operating on a
     * planar piece of geometry, and need to know the equation of that plane. The plane for a geometry chunk is computed from
     * its first three vertices.
     */
    const Plane& getPlane() const;

    /**
     * Returns the list of parameters associated with this geometry chunk.
     */
    ParameterArray& getParameters() { return parameters_; }

    /**
     * \copydoc GeometryChunk::getParameters()
     */
    const ParameterArray& getParameters() const { return parameters_; }

    /**
     * This method is used internally by the renderer to ensure that this chunk is ready to be rendered with the given effect.
     * The main task here is to load any texture parameters that are specified on this chunk. See Shader::prepareParameters()
     * for more details. This also checks whether there are any missing vertex streams, in which case it logs them as errors and
     * returns false.
     */
    bool setupForEffect(const Effect* effect) const;

    /**
     * Holds details of a ray intersection found by GeometryChunk::intersect().
     */
    class IntersectionResult
    {
    public:

        /**
         * Returns the distance along the ray to the intersection.
         */
        float getDistance() const { return distance_; }

        /**
         * Returns the surface normal at the point of intersection.
         */
        const Vec3& getNormal() const { return normal_; }

        /**
         * Constructs this intersection result with the specified distance and normal.
         */
        IntersectionResult(float distance, const Vec3& normal) : distance_(distance), normal_(normal) {}

    private:

        float distance_ = 0.0f;
        Vec3 normal_;
    };

    /**
     * Intersects a ray with the triangles in this geometry chunk. All intersections are appended to the \a results vector and
     * give the distance along the ray to the intersection point and the normal at the intersection point. The return value is
     * the number of intersections found.
     */
    unsigned int intersect(const Ray& ray, Vector<IntersectionResult>& results) const;

    /**
     * Saves this geometry chunk to a file stream. Throws an Exception if an error occurs.
     */
    bool save(FileWriter& file) const;

    /**
     * Loads this geometry chunk from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file);

    /**
     * Reports this chunk's memory usage to GatherMemorySummaryEvent.
     */
    bool processEvent(const Event& e) override;

    /**
     * Prints all this geometry chunk's data to the logfile for inspection, this includes all vertex data and all draw items
     * with indices.
     */
    void debugTrace() const;

    /**
     * Constant iterator for reading vertex stream data in a geometry chunk.
     */
    template <typename T> class VertexStreamConstIterator
    {
    public:

        /**
         * Dereferencing this iterator returns the value of this vertex stream for this vertex.
         */
        const T& operator*() const { return *reinterpret_cast<const T*>(data_); }

        /**
         * Increments this iterator to the next vertex in the geometry chunk.
         */
        VertexStreamConstIterator<T>& operator++()
        {
            data_ += vertexSize_;
            return *this;
        }

        /**
         * Increments this iterator to the next vertex in the geometry chunk.
         */
        VertexStreamConstIterator<T> operator++(int)
        {
            auto it = *this;
            data_ += vertexSize_;
            return it;
        }

        /**
         * Treats this vertex stream's data as an array of type T, returning the value at the specified index.
         */
        const T& operator[](unsigned int index) const { return reinterpret_cast<const T*>(data_)[index]; }

    private:

        const byte_t* data_ = nullptr;
        unsigned int vertexSize_ = 0;

        VertexStreamConstIterator(const byte_t* data, unsigned int vertexSize) : data_(data), vertexSize_(vertexSize) {}

        friend class GeometryChunk;
    };

    /**
     * Returns a new constant vertex stream iterator for the given vertex stream type.
     */
    template <typename T> VertexStreamConstIterator<T> getVertexStreamConstIterator(unsigned int vertexStream) const
    {
        assert(!isVertexDataLocked() && "Geometry chunk's vertex data is locked");
        assert(hasVertexStream(vertexStream) && "Geometry chunk does not have the requested vertex stream");

        return VertexStreamConstIterator<T>(getVertexData() + getVertexStream(vertexStream).getOffset(), getVertexSize());
    }

    /**
     * Iterator for reading and writing vertex stream data in a geometry chunk.
     */
    template <typename T> class VertexStreamIterator
    {
    public:

        /**
         * Dereferencing this iterator returns the value of this vertex stream for this vertex.
         */
        T& operator*() const { return *reinterpret_cast<T*>(data_); }

        /**
         * Treats this vertex stream's data as an array of type T, returning the value at the specified index.
         */
        T& operator[](unsigned int index) { return reinterpret_cast<T*>(data_)[index]; }

        /**
         * Increments this iterator to the next vertex in the geometry chunk.
         */
        VertexStreamIterator<T>& operator++()
        {
            data_ += vertexSize_;
            return *this;
        }

        /**
         * Increments this iterator to the next vertex in the geometry chunk.
         */
        VertexStreamIterator<T> operator++(int)
        {
            auto it = *this;
            data_ += vertexSize_;
            return it;
        }

    private:

        byte_t* data_ = nullptr;
        unsigned int vertexSize_ = 0;

        VertexStreamIterator(byte_t* data, unsigned int vertexSize) : data_(data), vertexSize_(vertexSize) {}

        friend class GeometryChunk;
    };

    /**
     * Returns a new vertex stream iterator for the given vertex stream type.
     */
    template <typename T> VertexStreamIterator<T> getVertexStreamIterator(unsigned int vertexStream)
    {
        assert(isVertexDataLocked() && "Geometry chunk's vertex data is not locked");
        assert(hasVertexStream(vertexStream) && "Geometry chunk does not have the requested vertex stream");

        return VertexStreamIterator<T>(getLockedVertexDataPointer() + getVertexStream(vertexStream).getOffset(),
                                       getVertexSize());
    }

private:

    // Vertex streams in this geometry chunk in the order they are stored in vertexData_
    Vector<VertexStream> vertexStreams_;

    unsigned int vertexCount_ = 0;
    unsigned int vertexSize_ = 0;

    // Updates vertexSize_ and the individual vertex stream offsets
    void updateVertexSizeAndStreamOffsets();

    // Interleaved vertex stream data
    Vector<byte_t> vertexData_;
    bool isVertexDataSpecified_ = false;

    template <typename T> const T* getVertexDataAt(unsigned int vertexIndex, unsigned int streamOffset) const
    {
        return reinterpret_cast<const T*>(vertexData_.getData() + vertexIndex * vertexSize_ + streamOffset);
    }

    // The drawitems of this geometry chunk
    Vector<DrawItem> drawItems_;
    mutable bool areDrawItemLowestHighestIndicesCurrent_ = false;
    void updateDrawItemLowestHighestIndices() const;

    // The actual index data referenced by the drawitems
    DataType indexDataType_ = TypeUInt16;
    Vector<byte_t> indexData_;
    bool compactIndexData();    // Reduces 32-bit indices down to 16-bit indices if the vertex count is <= 2^16

    bool isDynamic_ = false;
    bool isVertexDataLocked_ = false;

    DataBufferManager::AllocationObject vertexAllocation_ = nullptr;
    DataBufferManager::AllocationObject indexAllocation_ = nullptr;

    // Bounding volumes for the vertex data
    mutable AABB aabb_;
    mutable bool isAABBDirty_ = true;

    mutable Sphere sphere_;
    mutable bool isSphereDirty_ = true;

    mutable Plane plane_;
    mutable bool isPlaneDirty_ = true;

    // Parameters
    mutable ParameterArray parameters_;

    // The list of effects that this chunk has been setup against during this run
    struct EffectSetupResult
    {
        String effect;
        enum Result
        {
            Unknown,
            MissingVertexStream,
            Success
        } result = Unknown;

        EffectSetupResult() {}
        EffectSetupResult(String effect_, Result result_) : effect(std::move(effect_)), result(result_) {}
    };
    mutable Vector<EffectSetupResult> effectSetupResults_;
    mutable Vector<const Texture*> textureReferences_;

    // ShaderProgram instances that use this geometry chunk as a vertex source will cache a vertex attribute array configuration
    // on it the first time it is rendered in order to improve rendering performance
    friend class Shader;
    struct ShaderProgramVertexAttributeArrayConfiguration
    {
        ShaderProgram* program = nullptr;
        GraphicsInterface::VertexAttributeArrayConfigurationObject configuration = nullptr;

        ShaderProgramVertexAttributeArrayConfiguration(
            ShaderProgram* program_, GraphicsInterface::VertexAttributeArrayConfigurationObject configuration_)
            : program(program_), configuration(configuration_)
        {
        }
    };
    mutable Vector<ShaderProgramVertexAttributeArrayConfiguration> shaderProgramVertexAttributeArrayConfigurations_;
    void deleteVertexAttributeArrayConfigurations();

    // Returns a graphics interface array source for reading the specified vertex stream out of this geometry chunk
    GraphicsInterface::ArraySource getArraySourceForVertexStream(unsigned int streamType) const;
};

}
