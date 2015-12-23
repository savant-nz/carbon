/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/Endian.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Geometry/TriangleArray.h"
#include "CarbonEngine/Geometry/TriangleStripper.h"
#include "CarbonEngine/Math/HashFunctions.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Render/Effect.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Texture/Texture.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

namespace Carbon
{

const GeometryChunk GeometryChunk::Empty;

const VersionInfo GeometryChunk::GeometryChunkVersionInfo(1, 3);

GeometryChunk::GeometryChunk()
{
    if (Globals::isEngineInitialized())
        events().addHandler<GatherMemorySummaryEvent>(this);

    clear();
}

GeometryChunk::GeometryChunk(const GeometryChunk& other)
    : vertexStreams_(other.vertexStreams_),
      vertexCount_(other.vertexCount_),
      vertexSize_(other.vertexSize_),
      vertexData_(other.vertexData_),
      isVertexDataSpecified_(other.isVertexDataSpecified_),
      drawItems_(other.drawItems_),
      areDrawItemLowestHighestIndicesCurrent_(other.areDrawItemLowestHighestIndicesCurrent_),
      indexDataType_(other.indexDataType_),
      indexData_(other.indexData_),
      isDynamic_(other.isDynamic_),
      isVertexDataLocked_(other.isVertexDataLocked_),

      // The vertex and index allocations aren't copied
      vertexAllocation_(nullptr),
      indexAllocation_(nullptr),

      aabb_(other.aabb_),
      isAABBDirty_(other.isAABBDirty_),
      sphere_(other.sphere_),
      isSphereDirty_(other.isSphereDirty_),
      plane_(other.plane_),
      isPlaneDirty_(other.isPlaneDirty_),
      parameters_(other.parameters_),
      effectSetupResults_(other.effectSetupResults_),
      textureReferences_(other.textureReferences_),

      // shaderProgramVertexAttributeArrayConfigurations_ is not copied
      shaderProgramVertexAttributeArrayConfigurations_()
{
    if (Globals::isEngineInitialized())
        events().addHandler<GatherMemorySummaryEvent>(this);

    // Explicitly reference the textures again for this chunk, just copying the vector isn't enough
    for (auto texture : textureReferences_)
        textures().setupTexture(texture->getTextureType(), texture->getName());
}

GeometryChunk::GeometryChunk(GeometryChunk&& other) noexcept
    : vertexStreams_(std::move(other.vertexStreams_)),
      vertexCount_(other.vertexCount_),
      vertexSize_(other.vertexSize_),
      vertexData_(std::move(other.vertexData_)),
      isVertexDataSpecified_(other.isVertexDataSpecified_),
      drawItems_(std::move(other.drawItems_)),
      areDrawItemLowestHighestIndicesCurrent_(other.areDrawItemLowestHighestIndicesCurrent_),
      indexDataType_(other.indexDataType_),
      indexData_(std::move(other.indexData_)),
      isDynamic_(other.isDynamic_),
      isVertexDataLocked_(other.isVertexDataLocked_),
      vertexAllocation_(other.vertexAllocation_),
      indexAllocation_(other.indexAllocation_),
      aabb_(other.aabb_),
      isAABBDirty_(other.isAABBDirty_),
      sphere_(other.sphere_),
      isSphereDirty_(other.isSphereDirty_),
      plane_(other.plane_),
      isPlaneDirty_(other.isPlaneDirty_),
      parameters_(std::move(other.parameters_)),
      effectSetupResults_(std::move(other.effectSetupResults_)),
      textureReferences_(std::move(other.textureReferences_)),
      shaderProgramVertexAttributeArrayConfigurations_(std::move(other.shaderProgramVertexAttributeArrayConfigurations_))
{
    if (Globals::isEngineInitialized())
        events().addHandler<GatherMemorySummaryEvent>(this);

    other.vertexAllocation_ = nullptr;
    other.indexAllocation_ = nullptr;
    other.clear();
}

GeometryChunk::~GeometryChunk()
{
    clear();

    if (Globals::isEngineInitialized())
        events().removeHandler(this);
}

void swap(GeometryChunk& first, GeometryChunk& second)
{
    using std::swap;

    swap(first.vertexStreams_, second.vertexStreams_);
    swap(first.vertexCount_, second.vertexCount_);
    swap(first.vertexSize_, second.vertexSize_);
    swap(first.vertexData_, second.vertexData_);
    swap(first.isVertexDataSpecified_, second.isVertexDataSpecified_);
    swap(first.drawItems_, second.drawItems_);
    swap(first.areDrawItemLowestHighestIndicesCurrent_, second.areDrawItemLowestHighestIndicesCurrent_);
    swap(first.indexDataType_, second.indexDataType_);
    swap(first.indexData_, second.indexData_);
    swap(first.isDynamic_, second.isDynamic_);
    swap(first.isVertexDataLocked_, second.isVertexDataLocked_);
    swap(first.vertexAllocation_, second.vertexAllocation_);
    swap(first.indexAllocation_, second.indexAllocation_);
    swap(first.aabb_, second.aabb_);
    swap(first.isAABBDirty_, second.isAABBDirty_);
    swap(first.sphere_, second.sphere_);
    swap(first.isSphereDirty_, second.isSphereDirty_);
    swap(first.plane_, second.plane_);
    swap(first.isPlaneDirty_, second.isPlaneDirty_);
    swap(first.parameters_, second.parameters_);
    swap(first.effectSetupResults_, second.effectSetupResults_);
    swap(first.textureReferences_, second.textureReferences_);
    swap(first.shaderProgramVertexAttributeArrayConfigurations_, second.shaderProgramVertexAttributeArrayConfigurations_);
}

bool GeometryChunk::processEvent(const Event& e)
{
    if (auto rwe = e.as<const RecreateWindowEvent>())
    {
        if (rwe->getWindowEventType() == RecreateWindowEvent::CloseWindow)
            deleteVertexAttributeArrayConfigurations();
    }
    else if (auto gmse = e.as<GatherMemorySummaryEvent>())
    {
        gmse->addAllocation("VertexData", "", vertexData_.getData(), vertexData_.size());
        gmse->addAllocation("IndexData", "", indexData_.getData(), indexData_.size());
    }

    return true;
}

const VertexStream& GeometryChunk::getVertexStream(unsigned int streamType) const
{
    return vertexStreams_.detect([&](const VertexStream& stream) { return stream.getType() == streamType; },
                                 VertexStream::Empty);
}

bool GeometryChunk::hasVertexStream(unsigned int streamType) const
{
    return vertexStreams_.has([&](const VertexStream& stream) { return stream.getType() == streamType; });
}

void GeometryChunk::clear()
{
    vertexStreams_.clear();
    vertexCount_ = 0;
    vertexSize_ = 0;

    vertexData_.clear();
    isVertexDataSpecified_ = false;

    areDrawItemLowestHighestIndicesCurrent_ = true;
    drawItems_.clear();

    indexDataType_ = TypeUInt16;
    indexData_.clear();

    isDynamic_ = false;
    isVertexDataLocked_ = false;

    aabb_ = AABB();
    isAABBDirty_ = true;

    sphere_ = Sphere();
    isSphereDirty_ = true;

    plane_ = Plane();
    isPlaneDirty_ = true;

    parameters_.clear();

    if (Globals::isEngineInitialized())
        unregisterWithRenderer();

    effectSetupResults_.clear();

    // Release texture references
    for (auto texture : textureReferences_)
        textures().releaseTexture(texture);
    textureReferences_.clear();
}

bool GeometryChunk::setDynamic(bool dynamic)
{
    if (isRegisteredWithRenderer())
    {
        LOG_ERROR << "Can't alter vertex count once the chunk has been registered with the renderer, must unregister first";
        return false;
    }

    if (isVertexDataLocked())
        return false;

    isDynamic_ = dynamic;

    return true;
}

byte_t* GeometryChunk::lockVertexData()
{
    if (isVertexDataLocked_)
        return nullptr;

    isVertexDataLocked_ = true;

    return vertexData_.getData();
}

void GeometryChunk::unlockVertexData()
{
    if (isVertexDataLocked_)
    {
        isVertexDataSpecified_ = true;
        isVertexDataLocked_ = false;
        dataBuffers().updateData(vertexAllocation_);

        if (!isDynamic_)
        {
            isAABBDirty_ = true;
            isSphereDirty_ = true;
            isPlaneDirty_ = true;
        }
    }
}

unsigned int GeometryChunk::getIndexValue(unsigned int index) const
{
    if (index >= getIndexCount())
    {
        LOG_ERROR << "Invalid index: " << index << ", greater than the index count: " << getIndexCount();
        return 0;
    }

    if (indexDataType_ == TypeUInt16)
        return indexData_.as<uint16_t>()[index];

    if (indexDataType_ == TypeUInt32)
        return indexData_.as<unsigned int>()[index];

    return 0;
}

bool GeometryChunk::setIndexValue(unsigned int index, unsigned int value)
{
    if (indexAllocation_)
    {
        LOG_ERROR << "Can't alter index data once the chunk has been registered with the renderer, must unregister first";
        return false;
    }

    if (index >= getIndexCount())
    {
        LOG_ERROR << "Invalid index: " << index << ", greater than the index count: " << getIndexCount();
        return false;
    }

    if (value >= vertexCount_)
    {
        LOG_ERROR << "Invalid index value: " << value << ", greater than the vertex count: " << vertexCount_;
        return false;
    }

    if (indexDataType_ == TypeUInt16)
        indexData_.as<uint16_t>()[index] = uint16_t(value);
    else if (indexDataType_ == TypeUInt32)
        indexData_.as<unsigned int>()[index] = value;
    else
        return false;

    areDrawItemLowestHighestIndicesCurrent_ = false;

    return true;
}

bool GeometryChunk::addVertexStream(const VertexStream& vertexStream)
{
    if (isRegisteredWithRenderer())
    {
        LOG_ERROR << "Can't alter vertex streams once the chunk has been registered with the renderer, must unregister first";
        return false;
    }

    // Check the vertex stream is valid
    if (vertexStream.getType() == 0 || vertexStream.getDataType() == TypeNone)
    {
        LOG_ERROR << "Invalid vertex stream type or data type";
        return false;
    }

    // Check this stream doesn't already exist in this chunk
    if (hasVertexStream(vertexStream.getType()))
    {
        if (getVertexStream(vertexStream.getType()) == vertexStream)
            return true;

        LOG_ERROR << "This chunk already has a vertex stream of type " << vertexStream.getName()
                  << " but with a different component count or data type";
        return false;
    }

    // Add new vertex stream
    vertexStreams_.append(vertexStream);

    // Store previous vertex size
    auto oldVertexSize = vertexSize_;

    updateVertexSizeAndStreamOffsets();

    // Allocate new vertex data array
    auto newVertexData = Vector<byte_t>();
    try
    {
        newVertexData.resize(getVertexDataSize());
    }
    catch (const std::bad_alloc&)
    {
        LOG_ERROR << "Failed allocating memory for the new vertex data";

        vertexStreams_.popBack();
        updateVertexSizeAndStreamOffsets();

        return false;
    }

    // Copy previous vertex data into new array
    for (auto i = 0U; i < vertexCount_; i++)
        memcpy(&newVertexData[i * vertexSize_], &vertexData_[i * oldVertexSize], oldVertexSize);

    // Switch to new array
    swap(vertexData_, newVertexData);

    return true;
}

bool GeometryChunk::deleteVertexStream(unsigned int streamType)
{
    if (isRegisteredWithRenderer())
    {
        LOG_ERROR << "Can't alter vertex streams once the chunk has been registered with the renderer, must unregister first";
        return false;
    }

    if (!hasVertexStream(streamType))
        return false;

    // Remove the vertex stream
    auto offsetToStream = 0U;
    auto entrySize = 0U;
    for (auto i = 0U; i < vertexStreams_.size(); i++)
    {
        if (vertexStreams_[i].getType() != streamType)
            continue;

        if (i == 0 && vertexStreams_.size() > 1)
        {
            LOG_ERROR << "Can't delete the position vertex stream when there are other vertex streams present";
            return false;
        }

        offsetToStream = vertexStreams_[i].getOffset();
        entrySize = vertexStreams_[i].getSize();

        vertexStreams_.erase(i);
        break;
    }

    // Store previous vertex size
    auto oldVertexSize = vertexSize_;

    updateVertexSizeAndStreamOffsets();

    // Get rid of the deleted stream's vertex data
    if (isVertexDataSpecified_)
    {
        auto dataSizeAfterDeletedStream = oldVertexSize - offsetToStream - entrySize;
        for (auto i = 0U; i < vertexCount_; i++)
        {
            memcpy(&vertexData_[i * vertexSize_], &vertexData_[i * oldVertexSize], offsetToStream);
            memcpy(&vertexData_[i * vertexSize_ + offsetToStream], &vertexData_[i * oldVertexSize + offsetToStream + entrySize],
                   dataSizeAfterDeletedStream);
        }
    }

    // Reduce size of vertex data array
    vertexData_.resize(vertexCount_ * vertexSize_);

    return true;
}

bool GeometryChunk::setVertexStreams(const Vector<VertexStream>& vertexStreams)
{
    if (vertexCount_)
    {
        LOG_ERROR << "Can't set vertex streams when there are vertices present";
        return false;
    }

    vertexStreams_ = vertexStreams;
    updateVertexSizeAndStreamOffsets();

    return true;
}

bool GeometryChunk::compactIndexData()
{
    if (indexAllocation_)
    {
        LOG_ERROR << "Can't alter index data once the chunk has been registered with the renderer, must unregister first";
        return false;
    }

    if (indexDataType_ != TypeUInt32 || vertexCount_ > 65536)
        return true;

    auto indexCount = indexData_.size() / getDataTypeSize(indexDataType_);

    auto newIndexData = Vector<byte_t>();

    try
    {
        newIndexData.resize(indexCount * sizeof(uint16_t));
    }
    catch (const std::bad_alloc&)
    {
        LOG_ERROR << "Failed allocating memory for the compacted index data";
        return false;
    }

    for (auto i = 0U; i < indexCount; i++)
        newIndexData.as<uint16_t>()[i] = uint16_t(indexData_.as<unsigned int>()[i]);

    swap(indexData_, newIndexData);
    indexDataType_ = TypeUInt16;

    return true;
}

bool GeometryChunk::setIndexDataStraight()
{
    if (vertexCount_ % 3)
    {
        LOG_ERROR << "Vertex count is not a multiple of 3";
        return false;
    }

    auto newIndices = Vector<unsigned int>(vertexCount_);
    for (auto i = 0U; i < newIndices.size(); i++)
        newIndices[i] = i;

    return setupIndexData(Vector<DrawItem>(1, DrawItem(GraphicsInterface::TriangleList, newIndices.size(), 0)), newIndices);
}

bool GeometryChunk::generateTriangleStrips(Runnable& r)
{
    if (isVertexDataLocked())
        return false;

    auto originalIndices = copyIndexData();

    auto triangleIndices = Vector<unsigned int>();
    auto nonTriangleDrawItems = Vector<const DrawItem*>();

    // To construct the triangle indices we need to concatenate all the lists and strips Drawitems that are points or lines
    // can't be stripped
    for (auto& drawItem : drawItems_)
    {
        if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleList)
        {
            for (auto j = 0U; j < drawItem.getIndexCount(); j++)
                triangleIndices.append(originalIndices[drawItem.getIndexOffset() + j]);
        }
        else if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleStrip)
        {
            // Triangle strips have to be 'unstripped' to get the triangle indices
            for (auto j = 0U; j < drawItem.getIndexCount() - 2; j++)
            {
                auto triIndices = std::array<unsigned int, 3>{{originalIndices[drawItem.getIndexOffset() + j],
                                                               originalIndices[drawItem.getIndexOffset() + j + 1],
                                                               originalIndices[drawItem.getIndexOffset() + j + 2]}};

                // Ignore degenerate tris
                if (triIndices[0] != triIndices[1] && triIndices[0] != triIndices[2] && triIndices[1] != triIndices[2])
                {
                    if (j & 1)
                        std::swap(triIndices[1], triIndices[2]);

                    for (auto index : triIndices)
                        triangleIndices.append(index);
                }
            }
        }
        else
            nonTriangleDrawItems.append(&drawItem);
    }

    // Run the triangle stripper
    auto groups = Vector<TriangleStripper::PrimitiveWithIndices>();
    if (!TriangleStripper::run(triangleIndices, groups, r))
        return false;

    // Convert triangle stripper output into draw items
    auto newDrawItems = Vector<DrawItem>();
    auto newIndices = Vector<unsigned int>();
    for (const auto& group : groups)
    {
        newDrawItems.emplace(group.first, group.second.size(), newIndices.size());
        newIndices.append(group.second);
    }

    groups.clear();

    // Put the drawitems that are either points or lines on the end on the new drawitems list, and append their indices
    for (const auto& nonTriangleDrawItem : nonTriangleDrawItems)
    {
        newDrawItems.append(*nonTriangleDrawItem);
        newDrawItems.back().setIndexOffset(newIndices.size());

        for (auto i = 0U; i < nonTriangleDrawItem->getIndexCount(); i++)
            newIndices.append(originalIndices[nonTriangleDrawItem->getIndexOffset() + i]);
    }

    return setupIndexData(newDrawItems, newIndices);
}

void GeometryChunk::updateVertexSizeAndStreamOffsets()
{
    vertexSize_ = VertexStream::getVertexSize(vertexStreams_);

    // Calculate vertex offsets
    if (vertexStreams_.size())
    {
        auto offset = 0U;
        for (auto& vertexStream : vertexStreams_)
        {
            vertexStream.offset_ = offset;
            offset += vertexStream.getSize();
        }
    }
}

void GeometryChunk::updateDrawItemLowestHighestIndices() const
{
    if (areDrawItemLowestHighestIndicesCurrent_)
        return;

    // Calculate index bounds
    for (auto& drawItem : drawItems_)
    {
        if (indexDataType_ == TypeUInt16)
            drawItem.updateLowestAndHighestIndices(indexData_.as<uint16_t>());
        else if (indexDataType_ == TypeUInt32)
            drawItem.updateLowestAndHighestIndices(indexData_.as<unsigned int>());
    }

    areDrawItemLowestHighestIndicesCurrent_ = true;
}

bool GeometryChunk::registerWithRenderer()
{
    if (!Globals::isEngineInitialized())
        return false;

    // The drawitem highest and lowest indices need to be up to date because the renderer uses them
    updateDrawItemLowestHighestIndices();

    if (!vertexAllocation_)
    {
        vertexAllocation_ =
            dataBuffers().allocate(GraphicsInterface::VertexDataBuffer, getVertexDataSize(), vertexData_.getData(), isDynamic_);

        if (!vertexAllocation_)
        {
            LOG_ERROR << "Failed allocating vertex memory";
            return false;
        }
    }

    if (!indexAllocation_)
    {
        indexAllocation_ =
            dataBuffers().allocate(GraphicsInterface::IndexDataBuffer, getIndexDataSize(), indexData_.getData(), false);

        if (!indexAllocation_)
        {
            LOG_ERROR << "Failed allocating index memory";

            unregisterWithRenderer();

            return false;
        }
    }

    // The cached VertexAttributeArrayConfiguration objects stored on this chunk are deleted when the main window is recreated,
    // they are tied to the active graphics interface
    events().addHandler<RecreateWindowEvent>(this, true);

    return true;
}

bool GeometryChunk::unregisterWithRenderer()
{
    dataBuffers().free(vertexAllocation_);
    vertexAllocation_ = nullptr;

    dataBuffers().free(indexAllocation_);
    indexAllocation_ = nullptr;

    // Clear effect setup results and referenced textures, now that the chunk data has changed they are no longer valid
    effectSetupResults_.clear();
    for (auto texture : textureReferences_)
        textures().releaseTexture(texture);
    textureReferences_.clear();

    deleteVertexAttributeArrayConfigurations();

    return true;
}

void GeometryChunk::deleteVertexAttributeArrayConfigurations()
{
    for (auto& config : shaderProgramVertexAttributeArrayConfigurations_)
        graphics().deleteVertexAttributeArrayConfiguration(config.configuration);

    shaderProgramVertexAttributeArrayConfigurations_.clear();
}

GraphicsInterface::ArraySource GeometryChunk::getArraySourceForVertexStream(unsigned int streamType) const
{
    if (!isRegisteredWithRenderer())
        return GraphicsInterface::ArraySource();

    auto& vertexStream = getVertexStream(streamType);
    if (vertexStream.getType() == VertexStream::NoStream)
        return GraphicsInterface::ArraySource();

    auto offset = uintptr_t();
    auto dataBuffer = dataBuffers().getAllocationBufferObject(vertexAllocation_, offset);

    return GraphicsInterface::ArraySource(dataBuffer, offset + vertexStream.getOffset(), getVertexSize(),
                                          vertexStream.getComponentCount(), DataType(vertexStream.getDataType()),
                                          vertexStream.normalizeFixedPoint());
}

bool GeometryChunk::save(FileWriter& file) const
{
    try
    {
#ifdef CARBON_BIG_ENDIAN
        throw Exception("Saving of geometry data on big endian systems is not implemented");
#endif

        file.beginVersionedSection(GeometryChunkVersionInfo);

        file.writeBytes(nullptr, 1);    // Unused
        file.write(vertexStreams_, vertexCount_, vertexSize_, isVertexDataSpecified_);

        if (isVertexDataSpecified_)
            file.write(vertexData_);

        updateDrawItemLowestHighestIndices();

        file.write(drawItems_);
        file.writeEnum(indexDataType_);
        file.write(indexData_);

        file.write(isDynamic_);

        // Update boundings prior to saving them
        getAABB();
        getSphere();
        file.write(aabb_, sphere_);

        file.write(parameters_);

        file.endVersionedSection();

        getSphere().warnIfNotWellFormed();

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        return false;
    }
}

void GeometryChunk::load(FileReader& file)
{
    try
    {
        clear();

        auto readVersion = file.beginVersionedSection(GeometryChunkVersionInfo);

        // Dump ancient versions
        if (readVersion.getMinor() < 2)
            throw Exception("GeometryChunk version is too old, please re-export");

        file.skip(1);
        file.read(vertexStreams_, vertexCount_, vertexSize_, isVertexDataSpecified_);

        if (isVertexDataSpecified_)
        {
            // Read vertex data
            file.read(vertexData_);

#ifdef CARBON_BIG_ENDIAN
            // Change the endianness of the vertex data by going through each stream component of each vertex individually and
            // converting it
            auto vertexData = vertexData_.getData();

            for (auto i = 0U; i < vertexCount_; i++)
            {
                for (const auto& vertexStream : vertexStreams_)
                {
                    auto size = getDataTypeSize(DataType(vertexStream.getDataType()));

                    for (auto k = 0U; k < vertexStream.getComponentCount(); k++)
                    {
                        std::reverse(vertexData, vertexData + size);
                        vertexData += size;
                    }
                }
            }
#endif
        }
        else
        {
            updateVertexSizeAndStreamOffsets();

            // Allocate vertex data array
            try
            {
                vertexData_.resize(getVertexDataSize());
            }
            catch (const std::bad_alloc&)
            {
                throw Exception("Failed allocating memory for the vertex data");
            }
        }

        // Read index data
        file.read(drawItems_);
        file.read(indexDataType_);
        if (indexDataType_ != TypeUInt16 && indexDataType_ != TypeUInt32)
            throw Exception("Invalid index data type");
        file.read(indexData_);

#ifdef CARBON_BIG_ENDIAN
        // Convert index data endianness
        if (indexDataType_ == TypeUInt16)
            Endian::convertArray(indexData_.as<uint16_t>(), indexData_.size() / sizeof(uint16_t));
        else if (indexDataType_ == TypeUInt32)
            Endian::convertArray(indexData_.as<uint32_t>(), indexData_.size() / sizeof(uint32_t));
#endif

        // Read dynamic flag
        file.read(isDynamic_);

        // Read bounding data
        file.read(aabb_, sphere_);

        getSphere().warnIfNotWellFormed();

        isAABBDirty_ = false;
        isSphereDirty_ = false;
        isPlaneDirty_ = true;

        file.read(parameters_);

        file.endVersionedSection();

        // If there are tangent and normal vertex streams but no bitangent stream then add one now, newer meshes will have a
        // bitangent stream already but older meshes may not
        if (!hasVertexStream(VertexStream::Bitangent) && hasVertexStream(VertexStream::Tangent) &&
            hasVertexStream(VertexStream::Normal))
            calculateTangentBases();
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

unsigned int GeometryChunk::getTriangleCount() const
{
    auto triangleCount = 0U;

    for (auto& drawItem : drawItems_)
        triangleCount += drawItem.getTriangleCount();

    return triangleCount;
}

bool GeometryChunk::getTriangles(TriangleArray& triangles) const
{
    triangles.clear();
    triangles.copyVertexStreamsAndDataFromGeometryChunk(*this);

    // Loop through all the draw items that contain triangles and add their contents to the output array
    for (auto& drawItem : drawItems_)
    {
        if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleList)
        {
            for (auto j = 0U; j < drawItem.getIndexCount(); j += 3)
            {
                if (!triangles.addTriangle(getIndexValue(drawItem.getIndexOffset() + j + 0),
                                           getIndexValue(drawItem.getIndexOffset() + j + 1),
                                           getIndexValue(drawItem.getIndexOffset() + j + 2)))
                    return false;
            }
        }
        else if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleStrip)
        {
            for (auto j = 0U; j < drawItem.getIndexCount() - 2; j++)
            {
                if (!triangles.addTriangle(getIndexValue(drawItem.getIndexOffset() + j + 0),
                                           getIndexValue(drawItem.getIndexOffset() + j + ((j & 1) ? 2 : 1)),
                                           getIndexValue(drawItem.getIndexOffset() + j + ((j & 1) ? 1 : 2))))
                    return false;
            }
        }
    }

    return true;
}

const AABB& GeometryChunk::getAABB() const
{
    if (isAABBDirty_)
    {
        // Update AABB

        isAABBDirty_ = false;
        aabb_ = AABB();

        if (!isVertexDataSpecified_)
            return aabb_;

        auto itPosition = getVertexStreamConstIterator<Vec3>(VertexStream::Position);
        for (auto i = 0U; i < vertexCount_; i++, itPosition++)
            aabb_.addPoint(*itPosition);

        if (vertexCount_ == 0)
            aabb_.addPoint(Vec3::Zero);
    }

    return aabb_;
}

const Sphere& GeometryChunk::getSphere() const
{
    if (isSphereDirty_)
    {
        // TODO: This algorithm is much too simple and results in larger bounding spheres than necessary. It should be replaced
        // with a bona fide minimum bounding sphere algorithm

        isSphereDirty_ = false;
        sphere_ = Sphere();

        if (!isVertexDataSpecified_ || !vertexCount_)
            return sphere_;

        // Calculate center
        auto itPosition = getVertexStreamConstIterator<Vec3>(VertexStream::Position);
        auto factor = 1.0f / vertexCount_;
        for (auto i = 0U; i < vertexCount_; i++, itPosition++)
            sphere_.setOrigin(sphere_.getOrigin() + *itPosition * factor);

        // Calculate radius
        itPosition = getVertexStreamConstIterator<Vec3>(VertexStream::Position);
        for (auto i = 0U; i < vertexCount_; i++, itPosition++)
            sphere_.setRadius(std::max(sphere_.getRadius(), (sphere_.getOrigin() - *itPosition).length()));

        getSphere().warnIfNotWellFormed();
    }

    return sphere_;
}

const Plane& GeometryChunk::getPlane() const
{
    if (isPlaneDirty_)
    {
        // Construct the plane from the first vertex's position and normal
        if (isVertexDataSpecified_ && vertexCount_ >= 1 && hasVertexStream(VertexStream::Position) &&
            hasVertexStream(VertexStream::Normal))
        {
            auto p = getVertexStreamConstIterator<Vec3>(VertexStream::Position);
            auto n = getVertexStreamConstIterator<Vec3>(VertexStream::Normal);

            plane_ = Plane(*p, *n);
        }
        else
        {
            LOG_WARNING << "Unable to calculate a plane for this geometry chunk";

            plane_ = Plane(Vec3::Zero, Vec3::UnitY);
        }

        isPlaneDirty_ = false;
    }

    return plane_;
}

bool GeometryChunk::setupForEffect(const Effect* effect) const
{
    for (auto& effectSetupResult : effectSetupResults_)
    {
        if (effectSetupResult.effect == effect->getName())
            return effectSetupResult.result == EffectSetupResult::Success;
    }

    // Setup for new effect, this basically just involves loading textures for internal parameters that are stored on this
    // chunk. For example, if this chunk is to be drawn with lightmapping, the lightmap texture name is not stored in the
    // material but is stored in the chunk's parameters, which is where the shader will read it from. Texture parameters have to
    // be setup by Shader::prepareParameters().
    auto shader = effect->getActiveShader();
    if (!shader)
        return false;

    shader->prepareParameters(parameters_, textureReferences_);

    // Check for missing vertex streams
    for (auto& vertexStream : effect->getVertexStreams())
    {
        if (!hasVertexStream(vertexStream.getType()))
        {
            LOG_ERROR << "Can't draw with " << effect->getName() << ", missing vertex stream: " << vertexStream.getName();

            effectSetupResults_.emplace(effect->getName(), EffectSetupResult::MissingVertexStream);
            return false;
        }
    }

    // No missing vertex streams, the chunk is now setup for this effect
    effectSetupResults_.emplace(effect->getName(), EffectSetupResult::Success);

    return true;
}

bool GeometryChunk::setupIndexData(const Vector<DrawItem>& newDrawItems, const Vector<unsigned int>& newIndices)
{
    try
    {
        if (indexAllocation_)
            throw Exception("Can't alter index data when the chunk is registered with the renderer");

        // Check the new draw items are valid
        for (auto& drawItem : newDrawItems)
        {
            if (drawItem.getIndexOffset() + drawItem.getIndexCount() > newIndices.size())
                throw Exception("Drawitem goes past the end of the index buffer");
        }

        // Check the new indices are valid
        for (auto index : newIndices)
        {
            if (index >= vertexCount_)
                throw Exception() << "Index " << index << " is greater than the number of vertices";
        }

        // Allocate space for the new indices
        auto newIndexData = Vector<byte_t>();
        try
        {
            newIndexData.resize(newIndices.size() * sizeof(unsigned int));
        }
        catch (const std::bad_alloc&)
        {
            throw Exception("Failed allocating memory for the index data");
        }

        // Fill the new index buffer
        memcpy(newIndexData.getData(), newIndices.getData(), newIndices.getDataSize());

        // Update internal index data pointer
        swap(indexData_, newIndexData);
        indexDataType_ = TypeUInt32;

        // Compact index data if possible
        compactIndexData();

        // Update drawitems
        drawItems_ = newDrawItems;
        areDrawItemLowestHighestIndicesCurrent_ = false;
        updateDrawItemLowestHighestIndices();

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        return false;
    }
}

bool GeometryChunk::appendDrawItem(const DrawItem& drawItem)
{
    if (drawItem.getIndexOffset() + drawItem.getIndexCount() > getIndexCount())
    {
        LOG_ERROR << "Drawitem extends past the end of the index data";
        return false;
    }

    drawItems_.append(drawItem);

    if (areDrawItemLowestHighestIndicesCurrent_)
    {
        if (indexDataType_ == TypeUInt16)
            drawItems_.back().updateLowestAndHighestIndices(indexData_.as<uint16_t>());
        else if (indexDataType_ == TypeUInt32)
            drawItems_.back().updateLowestAndHighestIndices(indexData_.as<unsigned int>());
    }

    return true;
}

void GeometryChunk::clearDrawItems()
{
    drawItems_.clear();
    areDrawItemLowestHighestIndicesCurrent_ = true;
}

bool GeometryChunk::setVertexCount(unsigned int newVertexCount, bool preserveData)
{
    try
    {
        if (isRegisteredWithRenderer())
            throw Exception("Can't alter vertex count when the chunk is registered with the renderer");

        if (isVertexDataLocked())
            throw Exception("Can't alter vertex count when vertex data is locked");

        // Check that the new vertex count wouldn't cause any of the indices to be out of range
        for (auto i = 0U; i < getIndexCount(); i++)
        {
            if (getIndexValue(i) >= newVertexCount)
                throw Exception() << "New vertex count would cause index value " << i << " to be invalid";
        }

        // Allocate memory for the new vertex data array
        auto newVertexData = Vector<byte_t>();
        try
        {
            newVertexData.resize(newVertexCount * vertexSize_);
        }
        catch (const std::bad_alloc&)
        {
            throw Exception() << "Vertex allocation failed, size " << FileSystem::formatByteSize(newVertexCount * vertexSize_);
        }

        // Copy old data into new array if that is requested
        if (preserveData)
            memcpy(newVertexData.getData(), vertexData_.getData(), std::min(vertexCount_, newVertexCount) * vertexSize_);

        // Update the vertex data pointer
        swap(vertexData_, newVertexData);
        vertexCount_ = newVertexCount;

        // Reduce index data size if possible
        compactIndexData();

        // Let chunk know the vertex data has changed
        lockVertexData();
        unlockVertexData();

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        return false;
    }
}

Vector<unsigned int> GeometryChunk::copyIndexData() const
{
    auto indexData = Vector<unsigned int>(getIndexCount());

    for (auto i = 0U; i < indexData.size(); i++)
        indexData[i] = getIndexValue(i);

    return indexData;
}

bool GeometryChunk::validateVertexPositionData() const
{
    auto itPosition = getVertexStreamConstIterator<Vec3>(VertexStream::Position);

    for (auto i = 0U; i < vertexCount_; i++, itPosition++)
    {
        auto& v = *itPosition;

        if (!v.isFinite())
        {
            LOG_WARNING << "Vertex " << i << " is not finite: " << v;
            return false;
        }

        auto threshold = 1000000.0f;

        if (fabsf(v.x) > threshold || fabsf(v.y) > threshold || fabsf(v.z) > threshold)
        {
            LOG_WARNING << "Vertex " << i << " is too large: " << v;
            return false;
        }
    }

    return true;
}

bool GeometryChunk::optimizeVertexData(Runnable& r)
{
    if (isVertexDataLocked())
        return false;

    // The new vertex array won't be any bigger than the current one so ust allocate that much space up front
    auto newVertexCount = 0U;
    auto newVertexData = Vector<byte_t>();

    try
    {
        newVertexData.resize(getVertexDataSize());
    }
    catch (const std::bad_alloc&)
    {
        LOG_ERROR << "Failed allocating memory for the new vertex data";
        return false;
    }

    auto hashTable = std::unordered_map<unsigned int, std::vector<unsigned int>>(4093);

    for (auto i = 0U; i < getIndexCount(); i++)
    {
        // Get vertex referenced by this index
        auto v = &vertexData_[getIndexValue(i) * vertexSize_];
        auto& hashLineVertices = hashTable[HashFunctions::hash(v, vertexSize_)];
        auto matched = false;

        for (auto index : hashLineVertices)
        {
            if (memcmp(&newVertexData[index * vertexSize_], v, vertexSize_) == 0)
            {
                // We have a match, use it
                setIndexValue(i, index);
                matched = true;
                break;
            }
        }

        if (!matched)
        {
            setIndexValue(i, newVertexCount);
            memcpy(&newVertexData[newVertexCount * vertexSize_], v, vertexSize_);
            hashLineVertices.push_back(newVertexCount);
            newVertexCount++;
        }

        if (r.setTaskProgress(i + 1, getIndexCount()))
            return false;
    }

    // Replace existing vertex data
    vertexCount_ = newVertexCount;
    swap(vertexData_, newVertexData);
    vertexData_.resize(vertexCount_ * vertexSize_);

    return true;
}

bool GeometryChunk::transformVertexStream(unsigned int streamType, const Matrix4& transform)
{
    if (getVertexStream(streamType) != VertexStream(streamType, 3) || !lockVertexData())
        return false;

    auto itPosition = getVertexStreamIterator<Vec3>(streamType);

    for (auto i = 0U; i < vertexCount_; i++, itPosition++)
        *itPosition = transform * *itPosition;

    unlockVertexData();

    return true;
}

bool GeometryChunk::calculateTangentBases()
{
    if (isVertexDataLocked())
        return false;

    // Check we have the expected position and diffuse texture coordinate streams
    if (getVertexStream(VertexStream::Position) != VertexStream(VertexStream::Position, 3) ||
        getVertexStream(VertexStream::DiffuseTextureCoordinate) != VertexStream(VertexStream::DiffuseTextureCoordinate, 2))
        return false;

    // Make sure that there are vertex streams for the tangent and bitangent data
    if (!addVertexStream({VertexStream::Tangent, 3}) || !addVertexStream({VertexStream::Bitangent, 3}))
        return false;

    auto positionOffset = getVertexStream(VertexStream::Position).getOffset();
    auto tcOffset = getVertexStream(VertexStream::DiffuseTextureCoordinate).getOffset();
    auto tangentOffset = getVertexStream(VertexStream::Tangent).getOffset();
    auto bitangentOffset = getVertexStream(VertexStream::Bitangent).getOffset();

    auto data = lockVertexData();

    // Loop over all the triangles
    for (const auto& drawItem : drawItems_)
    {
        auto j = uint();
        if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleList)
            j = 0;
        else if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleStrip)
            j = 2;
        else
            continue;

        while (j < drawItem.getIndexCount())
        {
            // Read indices for this triangle
            auto indices = std::array<unsigned int, 3>();
            if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleList)
            {
                indices[0] = getIndexValue(drawItem.getIndexOffset() + j);
                indices[1] = getIndexValue(drawItem.getIndexOffset() + j + 1);
                indices[2] = getIndexValue(drawItem.getIndexOffset() + j + 2);

                j += 3;
            }
            else if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleStrip)
            {
                indices[0] = getIndexValue(drawItem.getIndexOffset() + j - 2);
                indices[1] = getIndexValue(drawItem.getIndexOffset() + j - 1);
                indices[2] = getIndexValue(drawItem.getIndexOffset() + j);

                if (j & 1)
                    std::swap(indices[1], indices[2]);

                j++;
            }

            // Skip degenerate triangles
            if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
                continue;

            // Vertex positions for this triangle
            auto& v1 = *reinterpret_cast<const Vec3*>(data + indices[0] * vertexSize_ + positionOffset);
            auto& v2 = *reinterpret_cast<const Vec3*>(data + indices[1] * vertexSize_ + positionOffset);
            auto& v3 = *reinterpret_cast<const Vec3*>(data + indices[2] * vertexSize_ + positionOffset);

            // Texture coordinates for this triangle
            auto& w1 = *reinterpret_cast<const Vec2*>(data + indices[0] * vertexSize_ + tcOffset);
            auto& w2 = *reinterpret_cast<const Vec2*>(data + indices[1] * vertexSize_ + tcOffset);
            auto& w3 = *reinterpret_cast<const Vec2*>(data + indices[2] * vertexSize_ + tcOffset);

            auto x1 = v2.x - v1.x;
            auto x2 = v3.x - v1.x;
            auto y1 = v2.y - v1.y;
            auto y2 = v3.y - v1.y;
            auto z1 = v2.z - v1.z;
            auto z2 = v3.z - v1.z;

            auto s1 = w2.x - w1.x;
            auto s2 = w3.x - w1.x;
            auto t1 = w2.y - w1.y;
            auto t2 = w3.y - w1.y;

            auto r1 = 1.0f / (s1 * t2 - s2 * t1);
            auto sDir = Vec3((t2 * x1 - t1 * x2) * r1, (t2 * y1 - t1 * y2) * r1, (t2 * z1 - t1 * z2) * r1);
            auto tDir = Vec3((s1 * x2 - s2 * x1) * r1, (s1 * y2 - s2 * y1) * r1, (s1 * z2 - s2 * z1) * r1);

            for (auto index : indices)
            {
                *reinterpret_cast<Vec3*>(data + index * vertexSize_ + tangentOffset) += sDir;
                *reinterpret_cast<Vec3*>(data + index * vertexSize_ + bitangentOffset) += tDir;
            }
        }
    }

    // Normalize the tangent and bitangent vectors
    for (auto i = 0U; i < vertexCount_; i++)
    {
        reinterpret_cast<Vec3*>(data + i * vertexSize_ + tangentOffset)->normalize();
        reinterpret_cast<Vec3*>(data + i * vertexSize_ + bitangentOffset)->normalize();
    }

    unlockVertexData();

    return true;
}

unsigned int GeometryChunk::intersect(const Ray& ray, Vector<IntersectionResult>& results) const
{
    // Check the ray intersects the bounding sphere for the chunk
    if (!getSphere().intersect(ray))
        return 0;

    // Check there is a 3xfloat position stream
    if (getVertexStream(VertexStream::Position) != VertexStream(VertexStream::Position, 3))
        return 0;

    auto initialResultsSize = results.size();

    auto positionOffset = getVertexStream(VertexStream::Position).getOffset();

    for (auto& drawItem : drawItems_)
    {
        if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleList)
        {
            for (auto j = 0U; j < drawItem.getIndexCount(); j += 3)
            {
                auto vertices = std::array<const Vec3*, 3>{
                    {getVertexDataAt<Vec3>(getIndexValue(drawItem.getIndexOffset() + j), positionOffset),
                     getVertexDataAt<Vec3>(getIndexValue(drawItem.getIndexOffset() + j + 1), positionOffset),
                     getVertexDataAt<Vec3>(getIndexValue(drawItem.getIndexOffset() + j + 2), positionOffset)}};

                auto t = 0.0f;
                if (ray.intersect(*vertices[0], *vertices[1], *vertices[2], &t))
                    results.emplace(t, Plane::normalFromPoints(*vertices[0], *vertices[1], *vertices[2]));
            }
        }
        else if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleStrip)
        {
            for (auto j = 0U; j < drawItem.getIndexCount() - 2; j++)
            {
                auto vertices = std::array<const Vec3*, 3>{
                    {getVertexDataAt<Vec3>(getIndexValue(drawItem.getIndexOffset() + j), positionOffset),
                     getVertexDataAt<Vec3>(getIndexValue(drawItem.getIndexOffset() + j + ((j & 1) ? 2 : 1)), positionOffset),
                     getVertexDataAt<Vec3>(getIndexValue(drawItem.getIndexOffset() + j + ((j & 1) ? 1 : 2)), positionOffset)}};

                auto t = 0.0f;
                if (ray.intersect(*vertices[0], *vertices[1], *vertices[2], &t))
                    results.emplace(t, Plane::normalFromPoints(*vertices[0], *vertices[1], *vertices[2]));
            }
        }
    }

    return results.size() - initialResultsSize;
}

void GeometryChunk::debugTrace() const
{
    LOG_DEBUG << "Logging contents of GeometryChunk at " << this << ", vertex count: " << vertexCount_
              << ", index count: " << getIndexCount();

    if (vertexCount_)
    {
        LOG_DEBUG << "";

        // Log vertices
        for (auto i = 0U; i < vertexCount_; i++)
        {
            LOG_DEBUG << "Vertex " << i;

            for (auto& stream : vertexStreams_)
            {
                auto value = String();
                if (stream.getDataType() == TypeUInt8)
                {
                    auto values = &vertexData_[i * vertexSize_ + stream.getOffset()];
                    for (auto k = 0U; k < stream.getComponentCount(); k++)
                        value << values[k] << " ";
                }
                else if (stream.getDataType() == TypeFloat)
                {
                    auto values = getVertexDataAt<float>(i, stream.getOffset());
                    for (auto k = 0U; k < stream.getComponentCount(); k++)
                        value << values[k] << " ";
                }

                LOG_DEBUG << "    " << stream.getName() << ": " << value;
            }
        }
    }

    if (drawItems_.size())
    {
        LOG_DEBUG << "";

        // Log drawitems
        for (auto i = 0U; i < drawItems_.size(); i++)
        {
            auto& drawItem = drawItems_[i];

            if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleList)
            {
                LOG_DEBUG << "DrawItem " << i << " - TriangleList with " << drawItem.getIndexCount() << " indices";

                for (auto j = 0U; j < drawItem.getIndexCount(); j += 3)
                {
                    LOG_DEBUG << "    Triangle: " << getIndexValue(drawItem.getIndexOffset() + j + 0) << " "
                              << getIndexValue(drawItem.getIndexOffset() + j + 1) << " "
                              << getIndexValue(drawItem.getIndexOffset() + j + 2);
                }
            }
            else if (drawItem.getPrimitiveType() == GraphicsInterface::TriangleStrip)
            {
                LOG_DEBUG << "DrawItem " << i << " - TriangleStrip with " << drawItem.getIndexCount() << " indices";

                for (auto j = 2U; j < drawItem.getIndexCount(); j++)
                {
                    LOG_DEBUG << "    Triangle: " << getIndexValue(drawItem.getIndexOffset() + j - 2) << " "
                              << getIndexValue(drawItem.getIndexOffset() + j - (j & 1 ? 0 : 1)) << " "
                              << getIndexValue(drawItem.getIndexOffset() + j - (j & 1 ? 1 : 0));
                }
            }
            else
                LOG_DEBUG << "DrawItem " << i << " - don't know how to log this primitive type";
        }
    }
}

}
