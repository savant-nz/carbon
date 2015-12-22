/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec2 tcTextureMap;
varying vec3 normal;
varying vec3 eyeVector;
varying vec4 tcScreen;

uniform sampler2D sDiffuseMap;
uniform sampler2D sReflectanceMap;
uniform sampler2D sReflectionMap;

uniform vec4 diffuseColor;
uniform vec4 reflectionColor;
uniform float fresnelExponent;

void main()
{
    vec3 N = normalize(normal);

    vec4 diffuse = texture2D(sDiffuseMap, tcTextureMap) * diffuseColor;

    // Sample reflection map
    vec4 reflection = texture2D(sReflectionMap, tcScreen.xy / tcScreen.w) * reflectionColor;

    // Compute fresnel term and modulate with reflectance map
    float reflectance = pow(1.0 - max(dot(normalize(eyeVector), N), 0.0), fresnelExponent);
    reflectance *= texture2D(sReflectanceMap, tcTextureMap).r;

    gl_FragColor = mix(diffuse, reflection, reflectance);
}
