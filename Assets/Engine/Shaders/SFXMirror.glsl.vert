/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

attribute vec3 vsPosition;
attribute vec2 vsDiffuseTextureCoordinate;
attribute vec3 vsNormal;

varying vec2 tcTextureMap;
varying vec3 normal;
varying vec3 eyeVector;
varying vec4 tcScreen;

uniform vec3 cameraPosition;
uniform mat4 modelViewProjection;
uniform mat4 screenProjection;

void main()
{
    gl_Position = modelViewProjection * vec4(vsPosition, 1.0);

    tcTextureMap = vsDiffuseTextureCoordinate;
    normal = vsNormal;

    eyeVector = cameraPosition - vsPosition;

    tcScreen = screenProjection * vec4(vsPosition, 1.0);
}
