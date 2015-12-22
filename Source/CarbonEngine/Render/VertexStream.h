/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * A vertex stream describes a single vertex data stream, including its type, data type, and component count.
 */
class CARBON_API VertexStream
{
public:

    /**
     * An empty vertex stream description.
     */
    static const VertexStream Empty;

    /**
     * The integer type value for an unspecified vertex stream, will always be zero.
     */
    static const auto NoStream = 0U;

    /**
     * The integer type value for the 'Position' vertex stream.
     */
    static const unsigned int Position;

    /**
     * The integer type value for the 'Color' vertex stream.
     */
    static const unsigned int Color;

    /**
     * The integer type value for the 'Normal' vertex stream.
     */
    static const unsigned int Normal;

    /**
     * The integer type value for the 'Tangent' vertex stream.
     */
    static const unsigned int Tangent;

    /**
     * The integer type value for the 'Bitangent' vertex stream.
     */
    static const unsigned int Bitangent;

    /**
     * The integer type value for the 'DiffuseTextureCoordinate' vertex stream.
     */
    static const unsigned int DiffuseTextureCoordinate;

    /**
     * The integer type value for the 'LightmapTextureCoordinate' vertex stream.
     */
    static const unsigned int LightmapTextureCoordinate;

    /**
     * The integer type value for the 'AmbientOcclusionTextureCoordinate' vertex stream.
     */
    static const unsigned int AmbientOcclusionTextureCoordinate;

    /**
     * The integer type value for the 'DecalTextureCoordinate' vertex stream.
     */
    static const unsigned int DecalTextureCoordinate;

    /**
     * The integer type value for the 'Bones' vertex stream.
     */
    static const unsigned int Bones;

    /**
     * The integer type value for the 'Weights' vertex stream.
     */
    static const unsigned int Weights;

    /**
     * Converts a vertex stream name to the corresponding non-zero integer type value.
     */
    static unsigned int streamNameToType(const String& name);

    /**
     * Converts a vertex stream non-zero integer type to the corresponding name.
     */
    static const String& streamTypeToName(unsigned int type);

    VertexStream() {}

    /**
     * Initializes this vertex stream with the given stream type, component count, data type, and fixed point normalization
     * flag.
     */
    VertexStream(unsigned int type, unsigned int componentCount, DataType dataType = TypeFloat,
                 bool normalizeFixedPoint = true);

    /**
     * Equality operator. This ignores the vertex stream offset value.
     */
    bool operator==(const VertexStream& other) const
    {
        return type_ == other.type_ && componentCount_ == other.componentCount_ && dataType_ == other.dataType_;
    }

    /**
     * Inequality operator. This ignores the vertex stream offset value.
     */
    bool operator!=(const VertexStream& other) const { return !operator==(other); }

    /**
     * Returns the type of this vertex stream.
     */
    unsigned int getType() const { return type_; }

    /**
     * Returns the name of this vertex stream, this is a textual version of the type.
     */
    const String& getName() const { return streamTypeToName(type_); }

    /**
     * Returns the component count of this vertex stream. Will be either 1, 2, 3 or 4.
     */
    unsigned int getComponentCount() const { return componentCount_; }

    /**
     * Returns the data type of this vertex stream.
     */
    DataType getDataType() const { return dataType_; }

    /**
     * Returns the offset in bytes of this vertex stream's data from the beginning of each vertex definition.
     */
    unsigned int getOffset() const { return offset_; }

    /**
     * Returns the size in bytes of a single entry of this vertex stream. This is equal to the number of components in the
     * stream multiplied by the size of the stream data type in bytes.
     */
    unsigned int getSize() const { return componentCount_ * getDataTypeSize(Carbon::DataType(dataType_)); }

    /**
     * Returns whether the values stored in this vertex stream should be normalized into the 0-1 range or passed through
     * directly. The main instance when fixed-point normalization must be turned off is in a vertex stream containing skeletal
     * bone indices, most others will probably have it turned on, e.g. a 32-bit per-vertex color stream. This normalization flag
     * only applies to fixed point data types such as 8-bit, 16-bit and 32-bit integers, it is ignored for vertex streams that
     * use floating point values.
     */
    bool normalizeFixedPoint() const { return normalizeFixedPoint_; }

    /**
     * Saves this vertex stream to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const;

    /**
     * Loads this vertex stream from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file);

    /**
     * Performs linear interpolation between two vertices that use the given vertex stream layout.
     */
    static void interpolate(const Vector<VertexStream>& streams, const byte_t* v0, const byte_t* v1, byte_t* result, float t);

    /**
     * Returns the size in bytes of a vertex that uses the given vertex streams, this is calculated by summing the value of
     * VertexStream::getSize() for all the passed vertex streams.
     */
    static unsigned int getVertexSize(const Vector<VertexStream>& streams);

private:

    unsigned int type_ = 0;
    unsigned int componentCount_ = 4;
    DataType dataType_ = TypeFloat;
    bool normalizeFixedPoint_ = true;

    // Byte offset of this vertex stream from the beginning of a vertex's data, set by the GeometryChunk class
    unsigned int offset_ = 0;
    friend class GeometryChunk;
};

}
