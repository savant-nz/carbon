/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec2 tcScreen;

uniform sampler2D sSceneTexture;
uniform sampler2D sSceneBlurTexture;
uniform sampler2D sDepthTexture;

uniform vec3 planeDistances;

uniform vec3 nearFarPlaneDistanceConstants; // near * far, far, far - near
uniform float zFar;
uniform float focalLength;
uniform float focalRange;
uniform float maxRadius;

void main()
{
    // Sample scene depth texture
    float depth = texture2D(sDepthTexture, tcScreen).r;

    // Convert to camera space depth
    float z = nearFarPlaneDistanceConstants.x / (nearFarPlaneDistanceConstants.y - depth * nearFarPlaneDistanceConstants.z);

    // Get radius of blur
    float blurFactor = clamp(abs((z - focalLength)) / focalRange, 0.0, 1.0);

    // Accumulate the blur
    gl_FragColor = mix(texture2D(sSceneTexture, tcScreen), texture2D(sSceneBlurTexture, tcScreen), blurFactor);
}
