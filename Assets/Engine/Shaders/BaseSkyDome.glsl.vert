/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

attribute vec3 vsPosition;

varying vec3 tcSkyDome;

uniform mat4 modelViewProjection;

void main()
{
    gl_Position = modelViewProjection * vec4(vsPosition, 1.0);

    // The local space vertex position is used to as a cubemap lookup
    tcSkyDome = vsPosition.xyz;
}
