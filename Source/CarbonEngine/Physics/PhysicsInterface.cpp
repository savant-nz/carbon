/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Image/Image.h"
#include "CarbonEngine/Math/Line.h"
#include "CarbonEngine/Physics/PhysicsInterface.h"
#include "CarbonEngine/Physics/Bullet/Bullet.h"
#include "CarbonEngine/Physics/PhysX/PhysX.h"

namespace Carbon
{

CARBON_DEFINE_INTERFACE_REGISTRY(PhysicsInterface)
{
    return i->isAvailable() && i->setup();
}

typedef PhysicsInterface NullInterface;
CARBON_REGISTER_INTERFACE_IMPLEMENTATION(PhysicsInterface, NullInterface, 0)

#ifdef CARBON_INCLUDE_BULLET
    CARBON_REGISTER_INTERFACE_IMPLEMENTATION(PhysicsInterface, Bullet, 100)
#endif
#ifdef CARBON_INCLUDE_PHYSX
    CARBON_REGISTER_INTERFACE_IMPLEMENTATION(PhysicsInterface, PhysX, 50)
#endif

const Vec3 PhysicsInterface::DefaultGravityVector(0.0f, -9.8f, 0.0f);
const AABB PhysicsInterface::DefaultWorldLimits(Vec3(-5000.0f), Vec3(5000.0f));

// Once a horizontal or vertical edge reaches this size then it will be favoured by the simplification process
const auto straightEdgeLength = 30;

struct PolygonVertex
{
    int x = 0;
    int y = 0;
    bool keep = false;    // Stops further simplification on this vertex

    PolygonVertex() {}
    PolygonVertex(int x_, int y_) : x(x_), y(y_) {}

    PolygonVertex operator-(const PolygonVertex& other) const { return {x - other.x, y - other.y}; }

    float distance(const PolygonVertex& v) const { return sqrtf(float((v.x - x) * (v.x - x) + (v.y - y) * (v.y - y))); }

    Vec2 toVec2() const { return {float(x), float(y)}; }
    Vec3 toVec3() const { return {float(x), float(y), 0.0f}; }

    bool isAxialEdge(const PolygonVertex& other) const
    {
        return (x == other.x && abs(y - other.y) > straightEdgeLength) ||
            (y == other.y && abs(x - other.x) > straightEdgeLength);
    }

    bool isRightAngle(const PolygonVertex& other) const
    {
        return fabsf(toVec2().normalized().dot(other.toVec2().normalized())) < 0.05f;
    }
};

struct Bitmap
{
    const int width = 0;
    const int height = 0;
    Vector<bool> data;

    Bitmap(int width_, int height_) : width(width_), height(height_) { data.resize(width * height); }

    void set(int x, int y, bool value) { data[y * width + x] = value; }

    bool get(int x, int y) const
    {
        if (x >= 0 && x < width && y >= 0 && y < height)
            return data[y * width + x];

        return false;
    }

    bool isAdjacent(const PolygonVertex& a, const PolygonVertex& b) const
    {
        return (a.x == b.x || a.y == b.y) && abs(a.x - b.x) <= 1 && abs(a.y - b.y) <= 1;
    }

    bool isPixelNearEdge(int x, int y, int threshold = 2) const
    {
        auto solids = 0U;
        auto empties = 0U;

        for (auto i = -threshold; i <= threshold; i++)
        {
            for (auto j = -threshold; j <= threshold; j++)
            {
                if (get(x + i, y + j))
                    solids++;
                else
                    empties++;

                if (solids && empties)
                    return true;
            }
        }

        return false;
    }

    bool arePixelsConnectedByEdge(const PolygonVertex& a, const PolygonVertex& b)
    {
        auto step = Vec2();
        auto stepCount = 0U;

        if (fabsf(float(a.x - b.x)) > fabsf(float(a.y - b.y)))
        {
            stepCount = abs(a.x - b.x);
            step.x = Math::getSign(float(b.x - a.x));
            step.y = float(b.y - a.y) / float(stepCount);
        }
        else
        {
            stepCount = abs(a.y - b.y);
            step.x = float(b.x - a.x) / float(stepCount);
            step.y = Math::getSign(float(b.y - a.y));
        }

        auto p = Vec2(float(a.x), float(a.y));

        for (auto i = 0U; i < stepCount; i++)
        {
            p += step;
            if (!isPixelNearEdge(int(p.x), int(p.y)))
                return false;
        }

        return true;
    }
};

bool PhysicsInterface::convertImageAlphaTo2DPolygons(const Image& image, Vector<Vector<Vec2>>& outPolygons,
                                                     bool flipHorizontally, bool flipVertically)
{
    // Check image is valid
    if (!image.isValid2DImage())
    {
        LOG_ERROR << "The passed image is not a valid 2D image: " << image;
        return false;
    }

    auto bitmap = Bitmap(image.getWidth(), image.getHeight());

    // Setup bitmap contents
    auto width = int(image.getWidth());
    for (auto i = 0U; i < bitmap.data.size(); i++)
        bitmap.set(i % width, i / width, image.getPixelColor(i % width, i / width).a > 0.5f);

    // Find all the edge pixels
    auto edgePixels = Vector<PolygonVertex>();
    for (auto y = 0; y < bitmap.height; y++)
    {
        for (auto x = 0; x < bitmap.width; x++)
        {
            if (bitmap.get(x, y) &&
                (!bitmap.get(x - 1, y - 1) || !bitmap.get(x - 1, y) || !bitmap.get(x - 1, y + 1) || !bitmap.get(x, y - 1) ||
                 !bitmap.get(x, y + 1) || !bitmap.get(x + 1, y - 1) || !bitmap.get(x + 1, y) || !bitmap.get(x + 1, y + 1)))
                edgePixels.emplace(x, y);
        }
    }

    while (!edgePixels.empty())
    {
        // Start the next polygon at an unused edge pixel
        auto polygon = Vector<PolygonVertex>(1, edgePixels.popBack());

        // Each pixel that is put onto polygon can be backtracked if it leads to a dead end, this fixes problems with pointy
        // angles that can cause the edge walking to get stuck.
        auto hasBacktracked = false;

        while (true)
        {
            // Continue building this polygon by finding the next adjacent edge pixel
            auto adjacentPixel = 0U;
            for (; adjacentPixel < edgePixels.size(); adjacentPixel++)
            {
                if (bitmap.isAdjacent(polygon.back(), edgePixels[adjacentPixel]))
                    break;
            }

            // If there was no adjacent edge pixel then this polygon is malformed, so skip it and keep trying to build more
            if (adjacentPixel == edgePixels.size())
            {
                if (!hasBacktracked)
                {
                    polygon.popBack();
                    hasBacktracked = true;

                    if (polygon.empty())
                        break;

                    continue;
                }
                else
                    break;
            }

            // Add the adjacent edge pixel to this polygon
            polygon.append(edgePixels[adjacentPixel]);
            edgePixels.erase(adjacentPixel);
            hasBacktracked = false;

            // Check whether this polygon is now complete, at least 4 points are required for a valid polygon
            if (polygon.size() < 4 || !bitmap.isAdjacent(polygon[0], polygon.back()))
                continue;

            // Now that a complete polygon has been constructed it needs to be simplified down as much as possible while
            // retaining key features such as large straight edges and right angles

            // Simplify perfectly horizontal and vertical edges as much as possible
            for (auto i = 0; i < int(polygon.size()); i++)
            {
                const auto& a = polygon[i];
                const auto& b = polygon[(i + 1) % polygon.size()];
                const auto& c = polygon[(i + 2) % polygon.size()];

                if ((a.x == b.x && a.x == c.x) || (a.y == b.y && a.y == c.y))
                    polygon.erase((i-- + 1) % polygon.size());
            }

            // Identify horizontal and vertical edges that are on the outside edge of the bitmap and mark their vertices as
            // important
            for (auto i = 0U; i < polygon.size(); i++)
            {
                auto& a = polygon[i];
                auto& b = polygon[(i + 1) % polygon.size()];
                if ((a.x == 0 || a.x == int(image.getWidth() - 1) || a.y == 0 || a.y == int(image.getHeight() - 1)) &&
                    a.isAxialEdge(b))
                {
                    a.keep = true;
                    b.keep = true;
                }
            }

            // Identify axial right angles and flag the relevant vertices as important
            for (auto i = 0U; i < polygon.size(); i++)
            {
                const auto& a = polygon[i];
                const auto& c = polygon[(i + 2) % polygon.size()];
                auto& b = polygon[(i + 1) % polygon.size()];

                if (a.isAxialEdge(b) && b.isAxialEdge(c) && (a - b).isRightAngle(c - b))
                    b.keep = true;
            }

            // The ends of straight edges that are not part of a right angle shape are pulled inwards by inserting new
            // vertices one pixel apart, this allows the ends of straight edges to undergo subsequent simplification. The
            // 'body' of the straight edge is then flagged as important to avoid any further simplification, which will
            // preserve the straight edge in the final result.
            const auto straightEdgePullBackSize = straightEdgeLength / 3;
            for (auto i = 0U; i < polygon.size(); i++)
            {
                auto a = polygon[i];
                auto b = polygon[(i + 1) % polygon.size()];

                if (a.isAxialEdge(b))
                {
                    auto xSign = Math::getSign(b.x - a.x);
                    auto ySign = Math::getSign(b.y - a.y);

                    if (!a.keep)
                    {
                        for (auto j = 0U; j < straightEdgePullBackSize; j++)
                            polygon.insert(i++, PolygonVertex(a.x + xSign * (j + 1), a.y + (j + 1) * ySign));

                        polygon[i].keep = true;
                    }

                    if (!b.keep)
                    {
                        for (auto j = 0U; j < straightEdgePullBackSize; j++)
                        {
                            polygon.insert(i++, PolygonVertex(b.x - (straightEdgePullBackSize - j) * xSign,
                                                              b.y - (straightEdgePullBackSize - j) * ySign));
                        }
                        polygon[i - straightEdgePullBackSize + 1].keep = true;
                    }
                }
            }

            // This is the main simplification loop, it works by trying to do progressively larger and larger simplifcations
            // on the polygon
            auto simplificationThreshold = 1.5f;
            while (polygon.size() > 3)
            {
                for (auto i = 0U; i < polygon.size(); i++)
                {
                    const auto& a = polygon[i];
                    const auto& b = polygon[(i + 1) % polygon.size()];
                    const auto& c = polygon[(i + 2) % polygon.size()];

                    // If b is important then don't try to get rid of it
                    if (b.keep)
                        continue;

                    // Get rid of point b if the line a-c is connected by an edge in the bitmap
                    if (a.distance(c) < simplificationThreshold && bitmap.arePixelsConnectedByEdge(a, c))
                        polygon.erase((i + 1) % polygon.size());
                }

                simplificationThreshold += 1.0f;
                if (simplificationThreshold >= std::max(image.getWidth(), image.getHeight()))
                    break;
            }

            if (polygon.size() < 3)
                break;

            outPolygons.enlarge(1);
            auto& outPolygon = outPolygons.back();

            // Scale to the range 0-1
            for (const auto& vertex : polygon)
                outPolygon.append(vertex.toVec2() / Vec2(float(image.getWidth() - 1), float(image.getHeight() - 1)));

            // Apply horizontal and vertical flips if requested
            if (flipHorizontally)
            {
                for (auto& vertex : outPolygon)
                    vertex.setXY(1.0f - vertex.x, vertex.y);
            }
            if (flipVertically)
            {
                for (auto& vertex : outPolygon)
                    vertex.setXY(vertex.x, 1.0f - vertex.y);
            }

            // Order vertices clockwise
            auto center = outPolygon.getAverage();
            if ((Vec3(outPolygon[0]) - center).cross(Vec3(outPolygon[1]) - center).z > 0.0f)
                outPolygon.reverse();

            break;
        }
    }

    return !outPolygons.empty();
}

void PhysicsInterface::convert2DPolygonsToCollisionGeometry(const Vector<Vector<Vec2>>& polygons, Vector<Vec3>& vertices,
                                                            Vector<RawIndexedTriangle>& triangles, float zScale)
{
    for (auto& polygon : polygons)
    {
        auto indexOffset = vertices.size();

        for (auto j = 0U; j < polygon.size(); j++)
        {
            vertices.emplace(polygon[j].x, polygon[j].y, -zScale);
            vertices.emplace(polygon[j].x, polygon[j].y, zScale);

            triangles.emplace(indexOffset + j * 2, indexOffset + j * 2 + 1, indexOffset + ((j + 1) % polygon.size()) * 2);
            triangles.emplace(indexOffset + j * 2 + 1, indexOffset + ((j + 1) % polygon.size()) * 2 + 1,
                              indexOffset + ((j + 1) % polygon.size()) * 2);
        }
    }
}

PhysicsInterface::BodyObject PhysicsInterface::createGeometryBodyFrom2DLineStrip(const Vector<Vec2>& points, float mass,
                                                                                 bool fixed, const Entity* entity,
                                                                                 const SimpleTransform& initialTransform)
{
    auto vertices = Vector<Vec3>();
    auto triangles = Vector<RawIndexedTriangle>();

    // Convert 2D line strip to an actual triangle mesh usable as a collision hull
    for (auto i = 0U; i < points.size(); i++)
    {
        vertices.emplace(points[i].x, points[i].y, -10.0f);
        vertices.emplace(points[i].x, points[i].y, 10.0f);

        triangles.emplace(i * 2 + 0, i * 2 + 1, ((i + 1) % points.size()) * 2 + 0);
        triangles.emplace(i * 2 + 1, ((i + 1) % points.size()) * 2 + 1, ((i + 1) % points.size()) * 2 + 0);
    }

    return physics().createGeometryBodyFromTemplate(physics().createBodyTemplateFromGeometry(vertices, triangles, true, 0.5f),
                                                    mass, fixed, nullptr, initialTransform);
}

}
