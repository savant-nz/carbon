/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec3 eyeVector;

uniform vec3 specularColor;
uniform float specularExponent;

vec3 specular(vec3 N, vec4 gloss, vec3 lightVector)
{
    // Compute specular term
    vec3 E = normalize(eyeVector);
    vec3 H = normalize(E + normalize(lightVector));

    return pow(clamp(dot(H, N), 0.0, 1.0), max(specularExponent * gloss.a, 2.0)) * gloss.rgb * specularColor;
}
