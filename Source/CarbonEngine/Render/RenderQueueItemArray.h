/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Render/RenderQueueItem.h"

namespace Carbon
{

/**
 * This class manages a vector of render queue items, with individual methods for adding each different type of item. This
 * allows more optimal internal allocation and management of individual render queue item instances. See RenderQueueItem for
 * more details.
 */
class CARBON_API RenderQueueItemArray : private Noncopyable
{
public:

    RenderQueueItemArray() {}

    /**
     * Copy constructor (not implemented).
     */
    RenderQueueItemArray(const RenderQueueItemArray& other);

    ~RenderQueueItemArray();

    /**
     * Adds a ChangeTransformRenderQueueItem to the back of this render queue item array.
     */
    void addChangeTransformItem(const SimpleTransform& transform, const Vec3& scale = Vec3::One);

    /**
     * Adds a DrawGeometryChunkRenderQueueItem to the back of this render queue item array.
     */
    void addDrawGeometryChunkItem(const GeometryChunk& geometryChunk, int drawItemIndex = -1);

    /**
     * Adds a DrawRectangleRenderQueueItem to the back of this render queue item array.
     */
    void addDrawRectangleItem(float width, float height);

    /**
     * Adds a DrawTextRenderQueueItem to the back of this render queue item array.
     */
    void addDrawTextItem(const Font* font, float fontSize, const UnicodeString& text, const Color& color = Color::White);

    /**
     * Returns an iterator at the start of the vector of render queue items.
     */
    const RenderQueueItem* const* begin() const { return items_.begin(); }

    /**
     * Returns an iterator at the end of the vector of render queue items.
     */
    const RenderQueueItem* const* end() const { return items_.end(); }

    /**
     * Returns the number of items in this render queue item array.
     */
    unsigned int size() const { return items_.size(); }

    /**
     * Returns the render queue item at the given \a index. \a index must be less than the value returned by
     * RenderQueueItemArray::size().
     */
    const RenderQueueItem& operator[](unsigned int index) const { return *items_[index]; }

    /**
     * Returns the most recently added render queue item.
     */
    RenderQueueItem& back() { return *items_.back(); }

    /**
     * Prints this render queue item array to the main logfile.
     */
    void debugTrace() const;

private:

    Vector<RenderQueueItem*> items_;
};

}
