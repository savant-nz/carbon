/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/Interpolate.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Rect.h"
#include "CarbonEngine/Math/SimpleTransform.h"

namespace Carbon
{

const Rect Rect::Zero;
const Rect Rect::One(0.0f, 0.0f, 1.0f, 1.0f);

Rect::Rect(const Rect& rect, const SimpleTransform& transform)
{
    auto corners = std::array<Vec2, 4>();
    rect.getCorners(corners, transform);

    left_ = corners[0].x;
    right_ = corners[0].x;
    bottom_ = corners[0].y;
    top_ = corners[0].y;

    for (auto i = 1U; i < 4; i++)
        addPoint(corners[i]);
}

Rect::Rect(const Vector<Vec2>& points)
{
    if (points.empty())
    {
        left_ = 0.0f;
        right_ = 0.0f;
        top_ = 0.0f;
        bottom_ = 0.0f;
    }
    else
    {
        left_ = points[0].x;
        right_ = points[0].x;
        top_ = points[0].y;
        bottom_ = points[0].y;

        for (auto i = 1U; i < points.size(); i++)
            addPoint(points[i]);
    }
}

Rect::Rect(const Vector<Vec3>& points)
{
    if (points.empty())
    {
        left_ = 0.0f;
        right_ = 0.0f;
        top_ = 0.0f;
        bottom_ = 0.0f;
    }
    else
    {
        left_ = points[0].x;
        right_ = points[0].x;
        top_ = points[0].y;
        bottom_ = points[0].y;

        for (auto i = 1U; i < points.size(); i++)
            addPoint(points[i].toVec2());
    }
}

Vec2 Rect::getPoint(float u, float v, bool clamp) const
{
    return {Interpolate::linear(left_, right_, clamp ? Math::clamp01(u) : u),
            Interpolate::linear(bottom_, top_, clamp ? Math::clamp01(v) : v)};
}

Vec2 Rect::getRandomPoint() const
{
    return {Math::random(left_, right_), Math::random(bottom_, top_)};
}

void Rect::getCorners(std::array<Vec2, 4>& corners, const SimpleTransform& transform) const
{
    corners[0].setXY(left_, bottom_);
    corners[1].setXY(left_, top_);
    corners[2].setXY(right_, top_);
    corners[3].setXY(right_, bottom_);

    for (auto& corner : corners)
        corner = transform * corner;
}

void Rect::getCorners(std::array<Vec3, 4>& corners, const SimpleTransform& transform) const
{
    corners[0].setXYZ(left_, bottom_, 0.0f);
    corners[1].setXYZ(left_, top_, 0.0f);
    corners[2].setXYZ(right_, top_, 0.0f);
    corners[3].setXYZ(right_, bottom_, 0.0f);

    for (auto& corner : corners)
        corner = transform * corner;
}

bool Rect::getIntersection(const Rect& rect, Rect& overlap) const
{
    if (!intersect(rect))
        return false;

    overlap.setLeft(std::max(left_, rect.getLeft()));
    overlap.setRight(std::min(right_, rect.getRight()));
    overlap.setBottom(std::max(bottom_, rect.getBottom()));
    overlap.setTop(std::min(top_, rect.getTop()));

    return true;
}

void Rect::addPoint(const Vec2& p)
{
    left_ = std::min(left_, p.x);
    right_ = std::max(right_, p.x);
    bottom_ = std::min(bottom_, p.y);
    top_ = std::max(top_, p.y);
}

void Rect::merge(const Rect& rect)
{
    left_ = std::min(left_, rect.getLeft());
    right_ = std::max(right_, rect.getRight());
    bottom_ = std::min(bottom_, rect.getBottom());
    top_ = std::max(top_, rect.getTop());
}

void Rect::clamp(float lower, float upper)
{
    left_ = Math::clamp(left_, lower, upper);
    bottom_ = Math::clamp(bottom_, lower, upper);
    right_ = Math::clamp(right_, lower, upper);
    top_ = Math::clamp(top_, lower, upper);
}

}
