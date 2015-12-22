/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "InternalDeferredLightingShadowMapping.glsl.frag"
#include "InternalDeferredLightingSpecular.glsl.frag"

varying vec2 tcScreen;

uniform sampler2D sNormalsTexture;
uniform sampler2D sDepthTexture;

uniform vec3 lightColor;
uniform float lightRadiusSquared;
uniform vec3 lightPosition;

uniform mat4 inverseViewProjectionMatrix;

#ifdef SPOTLIGHT
    uniform vec3 lightDirection;
    uniform vec2 spotlightConstants; // cos(minimumConeAngle), 1.0 / (cos(maximumConeAngle) - cos(minimumConeAngle))
    uniform float cosMaximumConeAngle;
#endif

#ifdef PROJECTION_TEXTURE
    uniform sampler2D sProjectionTexture;

    #define NEED_LIGHT_SPACE_CLIP_COORDINATES
#endif

#ifdef PROJECTION_CUBEMAP
    uniform samplerCube sProjectionCubemap;
    uniform mat3 lightOrientationInverse;
#endif

#ifdef SHADOW_MAPPING
    #define NEED_LIGHT_SPACE_CLIP_COORDINATES
#endif

#ifdef NEED_LIGHT_SPACE_CLIP_COORDINATES
    uniform mat4 lightViewProjectionMatrix;
#endif

void main()
{
    // Sample scene depth texture
    float depth = texture2D(sDepthTexture, tcScreen).r;

    // Convert to world position
    vec4 screenPosition = vec4(tcScreen.x, tcScreen.y, depth, 1.0) * 2.0 - 1.0;
    vec4 worldPosition = inverseViewProjectionMatrix * screenPosition;
    worldPosition /= worldPosition.w;

    // Read and unpack the surface normal
    vec4 normalsTextureSample = texture2D(sNormalsTexture, tcScreen);
    vec3 N = normalsTextureSample.xyz * 2.0 - 1.0;

    // Vector to the light source
    vec3 L = lightPosition - worldPosition.xyz;

    // Calculate attenuation
    float distance = length(L);
    float attenuation = max(0.0, 1.0 - (distance * distance) / lightRadiusSquared);

    // Compute N.L term
    L /= distance;
    float NDotL = clamp(dot(N, L), 0.0, 1.0);

    // Projective textures on this light, 2D and cubemap
    vec3 projection = vec3(1.0);

#ifdef NEED_LIGHT_SPACE_CLIP_COORDINATES
    // Calculate projective sampling coordinates from the world position, offsetting along the normal to avoid aliasing
    vec4 lightClipCoordinates = lightViewProjectionMatrix * vec4(worldPosition.xyz + N * 0.25, 1.0);
    lightClipCoordinates /= lightClipCoordinates.w;
#endif

#ifdef PROJECTION_TEXTURE
    projection *= texture2D(sProjectionTexture, clamp(lightClipCoordinates.xy, 0.0, 1.0)).rgb;
#endif

#ifdef PROJECTION_CUBEMAP
    projection *= textureCube(sProjectionCubemap, lightOrientationInverse * -L).rgb;
#endif

#ifdef SHADOW_MAPPING
    projection *= shadowMapping(NDotL, lightClipCoordinates);
#endif

#ifdef SPOTLIGHT
    // Calculate spotlight attenuation
    attenuation *= clamp(1.0 - (dot(L, -lightDirection) - spotlightConstants.x) * spotlightConstants.y, 0.0, 1.0);
#endif

#ifdef SPECULAR
    float specular = specular(N, L, worldPosition.xyz, normalsTextureSample.a * 255.0);
#else
    float specular = 0.0;
#endif

    gl_FragColor = vec4(projection * lightColor * NDotL, specular) * attenuation;
}
