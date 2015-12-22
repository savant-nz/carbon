/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

uniform vec3 cameraPosition;
uniform float specularIntensity;

float specular(vec3 N, vec3 L, vec3 worldPosition, float exponent)
{
    vec3 E = normalize(cameraPosition - worldPosition);
    vec3 R = reflect(-L, N);

    return pow(max(dot(R, E), 0.0), exponent) * specularIntensity;
}
