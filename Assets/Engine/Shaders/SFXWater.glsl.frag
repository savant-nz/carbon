/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec2 tcNormalMap;
varying vec4 tcReflectionRefraction;
varying vec3 eyeVector;

uniform sampler2D sNormalMap;
uniform sampler2D sReflectionMap;
uniform sampler2D sRefractionMap;

uniform vec2 distortionFactors; // Reflection, refraction
uniform vec4 reflectionTint;
uniform vec4 refractionTint;

void main()
{
    // Sample normal map
    vec4 normalMapSample = texture2D(sNormalMap, tcNormalMap).xzyw;
    vec3 normal = normalMapSample.xyz * 2.0 - 1.0;

    // Screen space distortions
    vec2 reflectionDistortion = normal.xz * distortionFactors.x * normalMapSample.a;
    vec2 refractionDistortion = normal.zx * distortionFactors.y * normalMapSample.a;

    // Sample textures
    vec2 st = tcReflectionRefraction.xy / tcReflectionRefraction.w;
    vec4 reflection = texture2D(sReflectionMap, st + reflectionDistortion) * reflectionTint;
    vec4 refraction = texture2D(sRefractionMap, st + refractionDistortion) * refractionTint;

    // Fresnel term to blend reflection and refraction
    float fresnel = pow(1.0 - max(dot(normalize(eyeVector), normal), 0.0), 4.0);

    gl_FragColor = mix(refraction, reflection, fresnel);
}
