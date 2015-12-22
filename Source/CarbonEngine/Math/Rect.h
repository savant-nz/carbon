/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Vec2.h"

namespace Carbon
{

/**
 * Simple 2D rectangle class. The coordinate system is x increasing going to the right and y increasing going up.
 */
class CARBON_API Rect
{
public:

    /**
     * Default constructor that sets the left, bottom, right and top values to zero.
     */
    Rect() {}

    /**
     * Component constructor that sets the rect's left, bottom, right and top values.
     */
    Rect(float left, float bottom, float right, float top) : left_(left), bottom_(bottom), right_(right), top_(top) {}

    /**
     * Constructor that creates a zero-volume rect at the given position.
     */
    explicit Rect(const Vec2& p) : left_(p.x), bottom_(p.y), right_(p.x), top_(p.y) {}

    /**
     * Constructor that sets this rect to be around the passed transformed rect.
     */
    Rect(const Rect& rect, const SimpleTransform& transform);

    /**
     * Constructs this rect so that it bounds the passed 2D points.
     */
    Rect(const Vector<Vec2>& points);

    /**
     * Constructs this rect so that it bounds the passed 3D points, the z values are ignored
     */
    Rect(const Vector<Vec3>& points);

    /**
     * Returns the left edge of this rectangle.
     */
    float getLeft() const { return left_; }

    /**
     * Sets the left edge of this rectangle.
     */
    void setLeft(float left) { left_ = left; }

    /**
     * Returns the bottom edge of this rectangle.
     */
    float getBottom() const { return bottom_; }

    /**
     * Sets the bottom edge of this rectangle.
     */
    void setBottom(float bottom) { bottom_ = bottom; }

    /**
     * Returns the right edge of this rectangle.
     */
    float getRight() const { return right_; }

    /**
     * Sets the right edge of this rectangle.
     */
    void setRight(float right) { right_ = right; }

    /**
     * Returns the top edge of this rectangle.
     */
    float getTop() const { return top_; }

    /**
     * Sets the top edge of this rectangle.
     */
    void setTop(float top) { top_ = top; }

    /**
     * Sets the left, bottom, right and top members of this rectangle.
     */
    void set(float left, float bottom, float right, float top)
    {
        left_ = left;
        bottom_ = bottom;
        right_ = right;
        top_ = top;
    }

    /**
     * Rect comparison.
     */
    bool operator==(const Rect& other) const
    {
        return left_ == other.left_ && bottom_ == other.bottom_ && right_ == other.right_ && top_ == other.top_;
    }

    /**
     * Rect comparison.
     */
    bool operator!=(const Rect& other) const
    {
        return left_ != other.left_ || bottom_ != other.bottom_ || right_ != other.right_ || top_ != other.top_;
    }

    /**
     * Offsets this rect.
     */
    Rect operator+(const Vec2& v) const { return {left_ + v.x, bottom_ + v.y, right_ + v.x, top_ + v.y}; }

    /**
     * Returns a copy of this rect scaled by the given value.
     */
    Rect operator*(float scale) const { return {left_ * scale, bottom_ * scale, right_ * scale, top_ * scale}; }

    /**
     * Returns a copy of this rect scaled by the given x and y values.
     */
    Rect operator*(const Vec2& v) const { return {left_ * v.x, bottom_ * v.y, right_ * v.x, top_ * v.y}; }

    /**
     * Returns a copy of this rect scaled by the given x and y values.
     */
    Rect operator/(const Vec2& v) const { return {left_ / v.x, bottom_ / v.y, right_ / v.x, top_ / v.y}; }

    /**
     * Scales this rect by the given x and y values.
     */
    Rect& operator*=(const Vec2& v)
    {
        left_ *= v.x;
        right_ *= v.x;
        bottom_ *= v.y;
        top_ *= v.y;

        return *this;
    }

    /**
     * Returns the width of this rect.
     */
    float getWidth() const { return right_ - left_; }

    /**
     * Returns the height of this rect.
     */
    float getHeight() const { return top_ - bottom_; }

    /**
     * Returns the width and height of this rect as a Vec2.
     */
    Vec2 getSize() const { return {getWidth(), getHeight()}; }

    /**
     * Returns the bottom left corner of this rect.
     */
    Vec2 getMinimum() const { return {left_, bottom_}; }

    /**
     * Returns the top right corner of this rect.
     */
    Vec2 getMaximum() const { return {right_, top_}; }

    /**
     * Returns the middle of this rect.
     */
    Vec2 getMiddle() const { return {(left_ + right_) * 0.5f, (bottom_ + top_) * 0.5f}; }

    /**
     * Returns a point inside this rect, both \a u and \a v should be normalized, and will be clamped to the range 0-1 if \a
     * clamp is true.
     */
    Vec2 getPoint(float u, float v, bool clamp = true) const;

    /**
     * Returns a random point within this rect.
     */
    Vec2 getRandomPoint() const;

    /**
     * Returns the four corners of this rect, transformed by the passed transform.
     */
    void getCorners(std::array<Vec2, 4>& corners, const SimpleTransform& transform) const;

    /**
     * Returns the four corners of this rect, transformed by the passed transform.
     */
    void getCorners(std::array<Vec3, 4>& corners, const SimpleTransform& transform) const;

    /**
     * Returns whether the passed point is inside this rectangle.
     */
    bool intersect(const Vec2& point) const
    {
        return point.x >= left_ && point.x <= right_ && point.y >= bottom_ && point.y <= top_;
    }

    /**
     * Returns whether the passed rectangle intersects with this rectangle.
     */
    bool intersect(const Rect& other) const
    {
        return other.getLeft() < right_ && other.getRight() < left_ && other.getTop() < bottom_ && other.getBottom() < top_;
    }

    /**
     * Computes the intersection between this rect and the passed rect, if there is an overlap then true is returned and the
     * overlapping area is returned in \a overlap.
     */
    bool getIntersection(const Rect& rect, Rect& overlap) const;

    /**
     * Returns the aspect ratio of this rect.
     */
    float getAspectRatio() const { return getWidth() / getHeight(); }

    /**
     * Increases the size of this rect to encompass the given point. If the point is already contained in this rect then this
     * method does nothing.
     */
    void addPoint(const Vec2& p);

    /**
     * Increases the size of this rect to encompass the given rect. If the given rect is already contained in this rect then
     * this method does nothing.
     */
    void merge(const Rect& rect);

    /**
     * Clamps the left, bottom, right and top values of this rect if they lie outside the given bounds.
     */
    void clamp(float lower = 0.0f, float upper = 1.0f);

    /**
     * Returns a new rect that covers the left half of this rect.
     */
    Rect getLeftHalf() const { return {left_, bottom_, left_ + getWidth() * 0.5f, top_}; }

    /**
     * Returns a new rect that covers the right half of this rect.
     */
    Rect getRightHalf() const { return {left_ + getWidth() * 0.5f, bottom_, right_, top_}; }

    /**
     * Converts this rect into a string of form "left bottom right top".
     */
    operator UnicodeString() const { return UnicodeString() << left_ << " " << bottom_ << " " << right_ << " " << top_; }

    /**
     * Saves this rect to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(left_, bottom_, right_, top_); }

    /**
     * Loads this rect from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file) { file.read(left_, bottom_, right_, top_); }

    /**
     * Rect instance with all components set to zero.
     */
    static const Rect Zero;

    /**
     * Rect instance with left and bottom set to zero and right and top set to one.
     */
    static const Rect One;

private:

    float left_ = 0.0f;
    float bottom_ = 0.0f;
    float right_ = 0.0f;
    float top_ = 0.0f;
};

}
