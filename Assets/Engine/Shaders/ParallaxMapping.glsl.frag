/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef PARALLAX_MAPPING

varying vec3 eyeVector;

uniform sampler2D sNormalMap;

uniform vec3 parallaxConstants; // scale, maximum step count, number of texels across diagonal of the height map

vec2 parallaxMapping(vec2 tc)
{
    // Normalize eye vector
    vec3 E = normalize(eyeVector);

    // The maximum amount that the texture coordinates can be offset is based on the viewing angle and the parallax scale
    float parallaxLimit = parallaxConstants.x * (length(E.xy) / E.z);

    // Calculate the number of sampling steps to do based on the resolution of the height map and the parallax limit
    float stepCount = parallaxLimit * parallaxConstants.z;

    // Clamp the number of sampling steps to the specified maximum
    stepCount = min(stepCount, parallaxConstants.y);

    // Work out the amount to shift for each iteration over the height map
    float heightStep = 1.0 / stepCount;
    vec2 tcStep = normalize(vec2(-E.x, E.y)) * parallaxLimit * heightStep;

    // Starting at the maximum height, move through the heightmap along the eye vector until the surface is hit
    float rayHeight = 1.0;
    float currentHeight, previousHeight = 1.0;
    for (float i = 0.0; i < stepCount; i += 1.0)
    {
        // Sample the height map
        currentHeight = texture2D(sNormalMap, tc).a;

        // If the ray has intersected the height map then stop iterating
        if (currentHeight > rayHeight)
        {
            // Calculate the linear intersection point with the height map
            tc -= tcStep * ((previousHeight - (rayHeight + heightStep)) / (heightStep + (currentHeight - previousHeight)));
            break;
        }

        // Move on to next heightmap sample
        rayHeight -= heightStep;
        tc += tcStep;

        previousHeight = currentHeight;
    }

    return tc;
}

#else

vec2 parallaxMapping(vec2 tc)
{
    return tc;
}

#endif
