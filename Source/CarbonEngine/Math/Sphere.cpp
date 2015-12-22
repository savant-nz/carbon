/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/AABB.h"
#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Math/Sphere.h"

namespace Carbon
{

Sphere::Sphere(const AABB& aabb)
{
    origin_ = aabb.getCenter();
    radius_ = (aabb.getMaximum() - aabb.getMinimum()).length() * 0.5f;
}

Sphere Sphere::getTransformedAndScaled(const SimpleTransform& transform, const Vec3& scale) const
{
    return {transform * (origin_ * scale), radius_ * scale.getLargestComponent()};
}

void Sphere::merge(const Vec3& p)
{
    if (origin_.distanceSquared(p) <= radius_ * radius_)
        return;

    radius_ = (radius_ + origin_.distance(p)) * 0.5f;
    origin_ = p + (origin_ - p).ofLength(radius_);
}

void Sphere::merge(const Sphere& sphere, float scale)
{
    // Vector between sphere origins
    auto v = sphere.getOrigin() * scale - origin_;
    auto originSeparation = v.length();
    v /= originSeparation;

    // New radius to be used assuming it is necessary to grow this sphere
    auto newRadius = (radius_ + sphere.getRadius() * scale + originSeparation) * 0.5f;

    // Check if the new sphere is completely contained
    if (newRadius < radius_)
        return;

    // Check if the new sphere completely contains the old one
    if (newRadius < sphere.getRadius())
    {
        origin_ = sphere.getOrigin() * scale;
        radius_ = sphere.getRadius() * scale;
    }
    else
    {
        origin_ += v * (newRadius - radius_);
        radius_ = newRadius;
    }
}

void Sphere::warnIfNotWellFormed() const
{
    if (!origin_.isFinite() || !std::isfinite(radius_) || radius_ > 1e+10f)
        LOG_WARNING << "Sphere is not well formed, there may be corruption";
}

bool Sphere::intersect(const Ray& ray, float* t) const
{
    if (t)
        *t = 0.0f;

    auto q = origin_ - ray.getOrigin();
    auto c = q.dot(q);
    auto v = q.dot(ray.getDirection());
    auto d = radius_ * radius_ - (c - v * v);

    if (d < 0.0f)
        return false;

    if (t)
        *t = v - sqrtf(d);

    return true;
}

}
