/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Runnable.h"
#include "CarbonEngine/Geometry/TriangleArraySet.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

/**
 * Compiles an ABT from a triangle soup.
 */
class CARBON_API ABTCompiler
{
public:

    /**
     * The lighting types that an ABT can be compiled for.
     */
    enum LightingType
    {
        LightingPerPixel,
        LightingLightmap
    };

    /**
     * The default triangle recursion threshold value.
     */
    static const auto DefaultTriangleRecursionThreshold = 5000U;

    /**
     * The default maximum overgrowth value, currently 10.
     */
    static const auto DefaultMaxOvergrowth = 10U;

    /**
     * Returns the current triangle recursion threshold. This value is used to determine when to stop the building of
     * the ABT. Defaults to 5000.
     */
    static unsigned int getTriangleRecursionThreshold() { return triangleRecursionThreshold_; }

    /**
     * Sets the triangle recursion threshold. The value is clamped to a minimum of 50.
     */
    static void setTriangleRecursionThreshold(unsigned int threshold)
    {
        triangleRecursionThreshold_ = std::max(threshold, 50U);
    }

    /**
     * Returns the current max overgrowth value. Defaults to 10.0f.
     */
    static float getMaxOvergrowth() { return maxOvergrowth_; }

    /**
     * Sets the max overgrowth value.
     */
    static void setMaxOvergrowth(float overgrowth) { maxOvergrowth_ = overgrowth; }

    /**
     * Returns the current lighting style to compile a scene for.
     */
    static LightingType getLightingType() { return lightingType_; }

    /**
     * Sets the current lighting style to compile a scene for.
     */
    static void setLightingType(LightingType lightingType) { lightingType_ = lightingType; }

    /**
     * Compiles a triangle soup into an ABT in the given scene.
     */
    static bool compile(Scene& scene, TriangleArraySet& triangleSet, Runnable& r = Runnable::Empty);

private:

    static bool subdivide(Scene& scene, CullingNode* node, TriangleArraySet& triangleSet, Runnable& r);

    class CompileNode
    {
    public:

        CullingNode* node = nullptr;
        TriangleArraySet triangleSet;
    };

    static unsigned int triangleRecursionThreshold_;
    static float maxOvergrowth_;
    static LightingType lightingType_;

    static unsigned int initialTriangleCount_;
    static Vector<CompileNode*> finalNodes_;
};

}
