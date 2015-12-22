/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Render/RenderQueueItemArray.h"

namespace Carbon
{

RenderQueueItemArray::~RenderQueueItemArray()
{
    for (auto item : items_)
        delete item;
}

void RenderQueueItemArray::addChangeTransformItem(const SimpleTransform& transform, const Vec3& scale)
{
    items_.append(new ChangeTransformRenderQueueItem(transform, scale));
}

void RenderQueueItemArray::addDrawGeometryChunkItem(const GeometryChunk& geometryChunk, int drawItemIndex)
{
    items_.append(new DrawGeometryChunkRenderQueueItem(geometryChunk, drawItemIndex));
}

void RenderQueueItemArray::addDrawRectangleItem(float width, float height)
{
    items_.append(new DrawRectangleRenderQueueItem(width, height));
}

void RenderQueueItemArray::addDrawTextItem(const Font* font, float fontSize, const UnicodeString& text, const Color& color)
{
    items_.append(new DrawTextRenderQueueItem(font, fontSize, text, color));
}

void RenderQueueItemArray::debugTrace() const
{
    for (auto i = 0U; i < items_.size(); i++)
    {
        if (auto transformItem = items_[i]->as<ChangeTransformRenderQueueItem>())
        {
            LOG_DEBUG << "    " << i << " - ChangeTransform - " << transformItem->getTransform()
                      << ", scale: " << transformItem->getScale();
        }
        else if (auto drawChunkItem = items_[i]->as<DrawGeometryChunkRenderQueueItem>())
        {
            auto& geometryChunk = drawChunkItem->getGeometryChunk();

            if (drawChunkItem->getDrawItemIndex() == -1)
            {
                LOG_DEBUG << "    " << i << " - DrawGeometryChunk - chunk: " << &geometryChunk
                          << ", triangles: " << geometryChunk.getTriangleCount();
            }
            else
            {
                LOG_DEBUG
                    << "    " << i << " - DrawGeometryChunk - chunk: " << &geometryChunk
                    << ", triangles: " << geometryChunk.getDrawItems()[drawChunkItem->getDrawItemIndex()].getTriangleCount();
            }
        }
        else if (auto drawRectItem = items_[i]->as<DrawRectangleRenderQueueItem>())
        {
            LOG_DEBUG << "    " << i << " - DrawRectangle - width: " << drawRectItem->getWidth()
                      << ", height: " << drawRectItem->getHeight();
        }
        else if (auto drawTextItem = items_[i]->as<DrawTextRenderQueueItem>())
        {
            LOG_DEBUG << "    " << i << " - DrawText - text: \"" << drawTextItem->getText()
                      << "\", color: " << drawTextItem->getColor();
        }
    }
}

}
