/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Render/EffectQueue.h"
#include "CarbonEngine/Render/EffectQueueArray.h"
#include "CarbonEngine/Render/Font.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"

namespace Carbon
{

// Immediate triangles that get gathered are rendered by putting their geometry into the following geometry chunk and appending
// a draw item to it. This geometry chunk has its draw items cleared at the start of every frame which means it will grow in
// size as needed and never shrink back down, thus avoiding unnecessary allocations.
static auto immediateTriangleGeometry = GeometryChunk();
static auto immediateTriangleCount = 0U;

static bool onFrameBeginEvent(const FrameBeginEvent& e)
{
    immediateTriangleGeometry.clearDrawItems();
    immediateTriangleCount = 0;
    return true;
}
CARBON_REGISTER_EVENT_HANDLER_FUNCTION(FrameBeginEvent, onFrameBeginEvent)

static void clearImmediateTriangleGeometry()
{
    immediateTriangleGeometry.clear();
}
CARBON_REGISTER_SHUTDOWN_FUNCTION(clearImmediateTriangleGeometry, 0)

GeometryGather::GeometryGather(const Vec3& cameraPosition, const ConvexHull& frustum, bool isShadowGeometryGather,
                               EffectQueueArray& queues)
    : cameraPosition_(cameraPosition),
      frustum_(frustum),
      isShadowGeometryGather_(isShadowGeometryGather),
      scale_(Vec3::One),
      queues_(queues)
{
    materialQueueInfos_.reserve(1024);
}

GeometryGather::~GeometryGather()
{
    // Now that the gather has completed it is important to unlock the vertex data so it has an opportunity to be uploaded to
    // the graphics interface
    immediateTriangleGeometry.unlockVertexData();
}

void GeometryGather::changeMaterial(const String& material, const ParameterArray& materialOverrideParameters)
{
    currentQueue_ = nullptr;

    if (materialOverrideParameters.empty())
    {
        // Try and find an existing queue that uses the specified material, the current priority, and has no custom parameters
        for (auto& q : materialQueueInfos_)
        {
            if (q.queue->getPriority() == currentPriority_ && q.material == material && !q.queue->hasCustomParams() &&
                !q.queue->getInternalParams().size())
            {
                currentQueue_ = &q;
                currentQueue_->isTransformCurrent = false;

                return;
            }
        }
    }

    // No existing material queue can be used, so create a new one
    newMaterial(&materials().getMaterial(material), materialOverrideParameters);
}

void GeometryGather::newMaterial(Material* material, const ParameterArray& materialOverrideParameters,
                                 const ParameterArray& internalParams)
{
    // Create new material queue entry
    auto queue = queues_.create(currentPriority_, material->getEffect(), internalParams);
    materialQueueInfos_.emplace(material->getName(), queue);
    currentQueue_ = &materialQueueInfos_.back();

    material->update();
    material->setupEffectQueue(currentQueue_->queue);

    // If override parameters are specified then add them as custom parameters to this queue
    if (!materialOverrideParameters.empty())
    {
        for (auto parameter : materialOverrideParameters)
            currentQueue_->queue->setCustomParameter(parameter.getLookup(), parameter.getValue());
    }
}

void GeometryGather::ensureTransformIsCurrent()
{
    if (!currentQueue_->isTransformCurrent)
    {
        currentQueue_->queue->getItems().addChangeTransformItem(transform_, scale_);
        currentQueue_->isTransformCurrent = true;
    }
}

void GeometryGather::addGeometryChunk(const GeometryChunk& geometryChunk, int drawItemIndex)
{
    ensureTransformIsCurrent();
    currentQueue_->queue->getItems().addDrawGeometryChunkItem(geometryChunk, drawItemIndex);
}

void GeometryGather::addRectangle(float width, float height)
{
    ensureTransformIsCurrent();
    currentQueue_->queue->getItems().addDrawRectangleItem(width, height);
}

void GeometryGather::addText(const Font* font, float fontSize, const UnicodeString& text, const Color& color)
{
    if (!font->isReadyForUse() || !text.length())
        return;

    changeMaterial("Font");

    ensureTransformIsCurrent();
    currentQueue_->queue->getItems().addDrawTextItem(font, fontSize, text, color);
}

struct ImmediateVertex
{
    Vec3 p;
    Vec2 st;
    unsigned int color = 0;
};

void GeometryGather::addImmediateTriangles(unsigned int triangleCount)
{
    changeMaterial("ImmediateGeometry");

    // Expand the immediate triangles chunk if needed
    if ((immediateTriangleCount + triangleCount) * 3 > immediateTriangleGeometry.getVertexCount())
    {
        immediateTriangleGeometry.unlockVertexData();

        immediateTriangleGeometry.unregisterWithRenderer();

        if (immediateTriangleGeometry.getVertexCount() == 0)
        {
            immediateTriangleGeometry.setDynamic(true);
            immediateTriangleGeometry.addVertexStream({VertexStream::Position, 3});
            immediateTriangleGeometry.addVertexStream({VertexStream::DiffuseTextureCoordinate, 2});
            immediateTriangleGeometry.addVertexStream({VertexStream::Color, 4, TypeUInt8});
        }

        auto initialVertexCount = immediateTriangleGeometry.getVertexCount();

        immediateTriangleGeometry.setVertexCount(std::max(immediateTriangleGeometry.getVertexCount() * 2,
                                                          immediateTriangleGeometry.getVertexCount() + triangleCount * 3));

        // Reset the new vertex data
        auto vertices = immediateTriangleGeometry.lockVertexData<ImmediateVertex>();
        for (auto i = initialVertexCount; i < immediateTriangleGeometry.getVertexCount(); i++)
            vertices[i] = ImmediateVertex();
        immediateTriangleGeometry.unlockVertexData();

        // Setup indices while preserving draw items
        auto indices = Vector<unsigned int>(immediateTriangleGeometry.getVertexCount());
        for (auto i = 0U; i < indices.size(); i++)
            indices[i] = i;
        immediateTriangleGeometry.setupIndexData(immediateTriangleGeometry.getDrawItems(), indices);

        immediateTriangleGeometry.registerWithRenderer();
    }

    // Add a drawitem for these immediate triangles and queue it for rendering
    immediateTriangleGeometry.appendDrawItem({GraphicsInterface::TriangleList, triangleCount * 3, immediateTriangleCount * 3});

    addGeometryChunk(immediateTriangleGeometry, immediateTriangleGeometry.getDrawItems().size() - 1);
}

void GeometryGather::addImmediateTriangle(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Color& color)
{
    if (!immediateTriangleGeometry.isVertexDataLocked())
        immediateTriangleGeometry.lockVertexData();

    assert(immediateTriangleGeometry.isVertexDataLocked());

    auto vertices = immediateTriangleGeometry.getLockedVertexDataPointer<ImmediateVertex>() + immediateTriangleCount * 3;

    vertices[0].p = v0;
    vertices[0].color = color.toRGBA8();
    vertices[1].p = v1;
    vertices[1].color = vertices[0].color;
    vertices[2].p = v2;
    vertices[2].color = vertices[0].color;

    immediateTriangleCount++;
}

}
