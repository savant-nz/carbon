/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Color.h"
#include "CarbonEngine/Math/SimpleTransform.h"

namespace Carbon
{

/**
 * RenderQueueItem is the base class for all items that go into a RenderQueueItemArray. Render queue items are either a change
 * of transform or a draw command.
 */
class CARBON_API RenderQueueItem
{
public:

    virtual ~RenderQueueItem() {}

    /**
     * Template method that is used to check the render queue item type, null is returned if this item is not of the specified
     * type, and if it is of the specified type then a pointer to that event type is returned. Internally this uses a
     * dynamic_cast.
     */
    template <typename T> const T* as() const { return dynamic_cast<const T*>(this); }
};

/**
 * This render queue item signals a change to the current transform, the new transform is given by the \a position, \a
 * orientation, and \a scale values.
 */
class CARBON_API ChangeTransformRenderQueueItem : public RenderQueueItem
{
public:

    /**
     * Initializes this change transform render queue item with the given values.
     */
    ChangeTransformRenderQueueItem(const SimpleTransform& transform, const Vec3& scale) : transform_(transform), scale_(scale)
    {
    }

    /**
     * Returns the new world space transform to use when rendering.
     */
    const SimpleTransform& getTransform() const { return transform_; }

    /**
     * Returns the new scale to use when rendering.
     */
    const Vec3& getScale() const { return scale_; }

private:

    const SimpleTransform transform_;
    const Vec3 scale_;
};

/**
 * This render queue item specifies a geometry chunk to be drawn using the current state. There are also flags controlling
 * shadowing.
 */
class CARBON_API DrawGeometryChunkRenderQueueItem : public RenderQueueItem
{
public:

    /**
     * Constructs this draw geometry chunk render queue item with the given chunk.
     */
    DrawGeometryChunkRenderQueueItem(const GeometryChunk& geometryChunk) : geometryChunk_(geometryChunk) {}

    /**
     * Constructs this draw geometry chunk render queue item with the given chunk and draw item index.
     */
    DrawGeometryChunkRenderQueueItem(const GeometryChunk& geometryChunk, int drawItemIndex = -1)
        : geometryChunk_(geometryChunk), drawItemIndex_(drawItemIndex)
    {
    }

    /**
     * Returns the GeometryChunk to draw.
     */
    const GeometryChunk& getGeometryChunk() const { return geometryChunk_; }

    /**
     * Returns the index of the single draw item in the geometry chunk to render, or -1 if all draw items should be rendered.
     */
    int getDrawItemIndex() const { return drawItemIndex_; }

private:

    const GeometryChunk& geometryChunk_;
    const int drawItemIndex_ = -1;
};

/**
 * This render queue item specifies a rectangle to be drawn using the current state. This is primarily a convenience item used
 * to render 2D elements which avoids having to provide a GeometryChunk.
 */
class CARBON_API DrawRectangleRenderQueueItem : public RenderQueueItem
{
public:

    /**
     * Initializes this draw rectangle render queue item with the given rectangle dimensions.
     */
    DrawRectangleRenderQueueItem(float width, float height) : width_(width), height_(height) {}

    /**
     * Returns the width of the rectangle to be rendered.
     */
    float getWidth() const { return width_; }

    /**
     * Returns the height of the rectangle to be rendered.
     */
    float getHeight() const { return height_; }

private:

    float width_ = 0.0f;
    float height_ = 0.0f;
};

/**
 * This render queue item specifies a text string to be drawn using the current state. The font can be set, as well as the text
 * color.
 */
class DrawTextRenderQueueItem : public RenderQueueItem
{
public:

    /**
     * Initializes this draw text render queue item with the given values.
     */
    DrawTextRenderQueueItem(const Font* font, float fontSize, UnicodeString text, const Color& color)
        : font_(font), fontSize_(fontSize), text_(std::move(text)), color_(color)
    {
    }

    /**
     * Returns the Font to use to render the text.
     */
    const Font* getFont() const { return font_; }

    /**
     * Returns the size of the font to render at.
     */
    float getFontSize() const { return fontSize_; }

    /**
     * Returns the text string to be rendered.
     */
    const UnicodeString& getText() const { return text_; }

    /**
     * Returns the color of the text to be rendered.
     */
    const Color& getColor() const { return color_; }

private:

    const Font* font_ = nullptr;
    float fontSize_ = 0.0f;
    const UnicodeString text_;
    const Color color_;
};

}
