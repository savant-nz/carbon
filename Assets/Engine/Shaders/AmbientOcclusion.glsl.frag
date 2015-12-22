/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef AMBIENT_OCCLUSION

varying vec2 tcAmbientOcclusionMap;

uniform sampler2D sAmbientOcclusionMap;

vec4 ambientOcclusion()
{
    return texture2D(sAmbientOcclusionMap, tcAmbientOcclusionMap);
}

#else

vec4 ambientOcclusion()
{
    return vec4(1.0);
}

#endif
