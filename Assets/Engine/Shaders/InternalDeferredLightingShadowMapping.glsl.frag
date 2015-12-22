/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef SHADOW_MAPPING

uniform sampler2D sShadowMap;
uniform float halfShadowMapTexelSize;

float shadowMapping(in float NDotL, in vec4 lightClipCoordinates)
{
    // Only shadow pixels facing the light
    if (NDotL <= 0.0)
        return 0.0;

    // Clamp light-space depth
    lightClipCoordinates.z = clamp(lightClipCoordinates.z, 0.0, 1.0);

    // Sample the shadow map using a 4-tap filter
    float shadow = 0.0;
    shadow  = texture2D(sShadowMap, lightClipCoordinates.xy + vec2( halfShadowMapTexelSize,  halfShadowMapTexelSize)).r
              < lightClipCoordinates.z ? 0.0 : 1.0;
    shadow += texture2D(sShadowMap, lightClipCoordinates.xy + vec2(-halfShadowMapTexelSize,  halfShadowMapTexelSize)).r
              < lightClipCoordinates.z ? 0.0 : 1.0;
    shadow += texture2D(sShadowMap, lightClipCoordinates.xy + vec2( halfShadowMapTexelSize, -halfShadowMapTexelSize)).r
              < lightClipCoordinates.z ? 0.0 : 1.0;
    shadow += texture2D(sShadowMap, lightClipCoordinates.xy + vec2(-halfShadowMapTexelSize, -halfShadowMapTexelSize)).r
              < lightClipCoordinates.z ? 0.0 : 1.0;

    return shadow * 0.25;
}

#else

float shadowMapping(in float NDotL, in vec4 lightClipCoordinates)
{
    return 1.0;
}

#endif
