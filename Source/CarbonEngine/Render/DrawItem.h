/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

/**
 * A drawitem describes a primitive type to draw and an index range that contains the indices to use when drawing this drawitem.
 * This class is used by GeometryChunk to desribe the individual primitive drawing commands needed to render its stored vertex
 * and index data.
 */
class CARBON_API DrawItem
{
public:

    DrawItem() {}

    /**
     * Initializes this draw item with the given primitive type, index count and index offset.
     */
    DrawItem(GraphicsInterface::PrimitiveType primitiveType, unsigned int indexCount, unsigned int indexOffset)
        : primitiveType_(primitiveType), indexCount_(indexCount), indexOffset_(indexOffset)
    {
    }

    /**
     * Equality operator.
     */
    bool operator==(const DrawItem& other) const
    {
        return primitiveType_ == other.primitiveType_ && indexCount_ == other.indexCount_ && indexOffset_ == other.indexOffset_;
    }

    /**
     * Inequality operator.
     */
    bool operator!=(const DrawItem& other) const
    {
        return primitiveType_ != other.primitiveType_ || indexCount_ != other.indexCount_ || indexOffset_ != other.indexOffset_;
    }

    /**
     * Returns the type of primitive this drawitem describes.
     */
    GraphicsInterface::PrimitiveType getPrimitiveType() const { return primitiveType_; }

    /**
     * Returns the number of indices this drawitem uses.
     */
    unsigned int getIndexCount() const { return indexCount_; }

    /**
     * Returns the offset into the geometry chunk indices where this drawitem's indices start.
     */
    unsigned int getIndexOffset() const { return indexOffset_; }

    /**
     * Sets the offset into the geometry chunk indices where this drawitem's indices start.
     */
    void setIndexOffset(unsigned int indexOffset) { indexOffset_ = indexOffset; }

    /**
     * Returns the lowest index value referenced by this drawitem.
     */
    unsigned int getLowestIndex() const { return lowestIndex_; }

    /**
     * Returns the highest index value referenced by this drawitem.
     */
    unsigned int getHighestIndex() const { return highestIndex_; }

    /**
     * Returns the number of triangles specified by this draw item.
     */
    unsigned int getTriangleCount() const
    {
        switch (primitiveType_)
        {
            case GraphicsInterface::TriangleList:
                return indexCount_ / 3;
            case GraphicsInterface::TriangleStrip:
                return indexCount_ > 2 ? indexCount_ - 2 : 0;
            case GraphicsInterface::TriangleListWithAdjacency:
                return indexCount_ / 6;
            case GraphicsInterface::TriangleStripWithAdjacency:
                return indexCount_ > 5 ? indexCount_ / 2 - 2 : 0;
            default:
                return 0;
        }
    }

    /**
     * Saves this drawitem to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const
    {
        file.writeEnum(primitiveType_);
        file.write(indexCount_, indexOffset_, lowestIndex_, highestIndex_);
    }

    /**
     * Loads this drawitem from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file)
    {
        file.readEnum(primitiveType_, GraphicsInterface::PrimitiveLast);
        file.read(indexCount_, indexOffset_, lowestIndex_, highestIndex_);
    }

    /**
     * Updates this drawitem's lowest and highest index values using the passed index data.
     */
    template <typename T> void updateLowestAndHighestIndices(const T* indexData) const
    {
        auto lowest = T();
        auto highest = T();

        Math::calculateBounds(indexData + indexOffset_, indexCount_, lowest, highest);

        lowestIndex_ = lowest;
        highestIndex_ = highest;
    }

private:

    GraphicsInterface::PrimitiveType primitiveType_ = GraphicsInterface::TriangleStrip;

    unsigned int indexCount_ = 0;
    unsigned int indexOffset_ = 0;

    // Highest and lowest indices used by this drawitem
    mutable unsigned int lowestIndex_ = 0;
    mutable unsigned int highestIndex_ = 0;
};

}
