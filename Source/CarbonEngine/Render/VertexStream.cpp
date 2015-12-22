/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/Interpolate.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Render/VertexStream.h"

namespace Carbon
{

const VertexStream VertexStream::Empty;

static auto vertexStreamTypes = Vector<String>();

#define DEFINE_VERTEX_STREAM_TYPE(Type) const unsigned int VertexStream::Type = VertexStream::streamNameToType(#Type)

DEFINE_VERTEX_STREAM_TYPE(Position);
DEFINE_VERTEX_STREAM_TYPE(Color);
DEFINE_VERTEX_STREAM_TYPE(Normal);
DEFINE_VERTEX_STREAM_TYPE(Tangent);
DEFINE_VERTEX_STREAM_TYPE(Bitangent);
DEFINE_VERTEX_STREAM_TYPE(DiffuseTextureCoordinate);
DEFINE_VERTEX_STREAM_TYPE(LightmapTextureCoordinate);
DEFINE_VERTEX_STREAM_TYPE(AmbientOcclusionTextureCoordinate);
DEFINE_VERTEX_STREAM_TYPE(DecalTextureCoordinate);
DEFINE_VERTEX_STREAM_TYPE(Bones);
DEFINE_VERTEX_STREAM_TYPE(Weights);

unsigned int VertexStream::streamNameToType(const String& name)
{
    // Map old names for the built-in vertex streams to the current names, this ensures backwards compatibility
    if (name == "TCDiffuse")
        return DiffuseTextureCoordinate;
    if (name == "TCLightmap")
        return LightmapTextureCoordinate;
    if (name == "TCAmbientOcclusion")
        return AmbientOcclusionTextureCoordinate;
    if (name == "TCDecal")
        return DecalTextureCoordinate;

    // Matching of vertex stream names is case insensitive
    for (auto i = 0U; i < vertexStreamTypes.size(); i++)
    {
        if (vertexStreamTypes[i].asLower() == name.asLower())
            return i + 1;
    }

    vertexStreamTypes.append(name);

    return vertexStreamTypes.size();
}

const String& VertexStream::streamTypeToName(unsigned int type)
{
    return (type && type <= vertexStreamTypes.size()) ? vertexStreamTypes[type - 1] : String::Empty;
}

VertexStream::VertexStream(unsigned int type, unsigned int componentCount, DataType dataType, bool normalizeFixedPoint)
    : type_(type), dataType_(dataType), normalizeFixedPoint_(normalizeFixedPoint)
{
    componentCount_ = Math::clamp<unsigned int>(componentCount, 1, 4);
}

void VertexStream::save(FileWriter& file) const
{
    file.write(streamTypeToName(type_), componentCount_);
    file.writeEnum(dataType_);
    file.write(offset_);
    file.write(normalizeFixedPoint_);
}

void VertexStream::load(FileReader& file)
{
    // Read the stream name as a string and convert it to a type
    auto streamName = String();
    file.read(streamName);
    type_ = streamNameToType(streamName);

    // Read vertex stream details
    file.read(componentCount_, dataType_, offset_);

    // Whether to normalize fixed point data, prior to v1.3 this wasn't persisted so just set a sensible default
    auto readVersion = file.findVersionedSection(GeometryChunk::GeometryChunkVersionInfo);
    if (readVersion.getMinor() < 3)
        normalizeFixedPoint_ = (type_ != Bones);
    else
        file.read(normalizeFixedPoint_);
}

void VertexStream::interpolate(const Vector<VertexStream>& streams, const byte_t* v0, const byte_t* v1, byte_t* result, float t)
{
    for (auto& stream : streams)
    {
        auto dataType = stream.getDataType();
        auto dataTypeSize = getDataTypeSize(dataType);

        for (auto j = 0U; j < stream.getComponentCount(); j++)
        {
            if (dataType == TypeFloat)
            {
                *reinterpret_cast<float*>(result) =
                    Interpolate::linear(*reinterpret_cast<const float*>(v0), *reinterpret_cast<const float*>(v1), t);
            }
            else if (dataType == TypeUInt8)
                *result = byte_t(Interpolate::linear(float(*v0), float(*v1), t));
            else
                LOG_WARNING << "Don't know how to interpolate this data type";

            v0 += dataTypeSize;
            v1 += dataTypeSize;
            result += dataTypeSize;
        }
    }
}

unsigned int VertexStream::getVertexSize(const Vector<VertexStream>& streams)
{
    auto vertexSize = 0U;

    for (auto& stream : streams)
        vertexSize += stream.getSize();

    return vertexSize;
}

}
