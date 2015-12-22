/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec3 localVertexPosition;
varying vec2 tcNormalMap;

uniform sampler2D sBaseMap;
uniform sampler2D sDetailMap;
uniform sampler2D sNormalMap;

uniform vec4 scales; // finalScale, heightScale, textureScale, terrainScale

uniform vec4 lightColor;
uniform vec4 lightAmbient;
uniform vec3 lightDirection;

void main()
{
    // Expand normal
    vec3 normal = vec3(texture2D(sNormalMap, tcNormalMap)) * 2.0 - 1.0;

    // Lighting term
    float lit = max(dot(normal, -lightDirection), 0.0);

    vec4 baseMapSample = texture2D(sBaseMap, localVertexPosition.xz * scales.z / scales.w);
    vec4 detailMapSample = texture2D(sDetailMap, localVertexPosition.xz * scales.z);

    vec4 surfaceColor = mix(detailMapSample, baseMapSample, baseMapSample.a);

    gl_FragColor = (lightColor * lit + lightAmbient) * surfaceColor;
}
