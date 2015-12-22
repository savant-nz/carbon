/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

attribute vec3 vsPosition;

varying vec2 tcNormalMap;
varying vec4 tcReflectionRefraction;
varying vec3 eyeVector;

uniform vec3 cameraPosition;
uniform mat4 modelViewProjection;
uniform mat4 reflectionRefractionProjection;
uniform float tilingFactor;

void main()
{
    gl_Position = modelViewProjection * vec4(vsPosition, 1.0);

    // Normal map texture coordinates come from the xz position
    tcNormalMap = vsPosition.xz * tilingFactor;

    // Vertex to eye vector
    eyeVector = cameraPosition - vsPosition;

    // Screen space projective coords that get distorted by the fragment shader
    tcReflectionRefraction = reflectionRefractionProjection * vec4(vsPosition, 1.0);
}
