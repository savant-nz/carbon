/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "InternalDeferredLightingShadowMapping.glsl.frag"
#include "InternalDeferredLightingSpecular.glsl.frag"

varying vec2 tcScreen;

uniform sampler2D sNormalsTexture;

uniform vec3 lightColor;
uniform vec3 lightDirection;

#if defined(SPECULAR) || defined(SHADOW_MAPPING)
    uniform sampler2D sDepthTexture;
    uniform mat4 inverseViewProjectionMatrix;
#endif

#ifdef SHADOW_MAPPING
    uniform mat4 lightViewProjectionMatrix;
#endif

void main()
{
    // Read and unpack the surface normal
    vec4 normalsTextureSample = texture2D(sNormalsTexture, tcScreen);
    vec3 N = normalsTextureSample.xyz * 2.0 - vec3(1.0);

    // Compute N.L term
    float NDotL = clamp(dot(N, -lightDirection), 0.0, 1.0);

#if defined(SPECULAR) || defined(SHADOW_MAPPING)
    // Sample scene depth texture
    float depth = texture2D(sDepthTexture, tcScreen).r;

    // Convert to world position
    vec4 screenPosition = vec4(tcScreen.x, tcScreen.y, depth, 1.0) * 2.0 - 1.0;
    vec4 worldPosition = inverseViewProjectionMatrix * screenPosition;
    worldPosition /= worldPosition.w;
#endif

#ifdef SPECULAR
    float specular = specular(N, -lightDirection, worldPosition.xyz, normalsTextureSample.a * 255.0);
#else
    float specular = 0.0;
#endif

#ifdef SHADOW_MAPPING
    // Calculate shadow map sampling coordinates from the world position, offsetting along the normal to avoid aliasing
    vec4 lightClipCoordinates = lightViewProjectionMatrix * vec4(worldPosition.xyz + N * 0.25, 1.0);
    float shadow = shadowMapping(NDotL, lightClipCoordinates);
#else
    float shadow = 1.0;
#endif

    gl_FragColor = vec4(lightColor * NDotL * shadow, specular);
}
