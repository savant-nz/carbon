/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec2 tcTextureMap;
varying vec3 eyeVector;

uniform sampler2D sDiffuseMap;
uniform sampler2D sNormalMap;
uniform sampler2D sEdgeLookupMap;

uniform vec4 diffuseColor;

void main()
{
    // Expand normal
    vec3 normal = vec3(texture2D(sNormalMap, tcTextureMap)) * 2.0 - 1.0;

    // Edge term
    float NDotE = max(dot(normal, normalize(eyeVector)), 0.0);
    vec4 edgeLookupResult = texture2D(sEdgeLookupMap, vec2(NDotE, 0.0));

    // Sample texture
    vec4 diffuseTexture = texture2D(sDiffuseMap, tcTextureMap);

    gl_FragColor = edgeLookupResult * diffuseTexture * diffuseColor;
}
