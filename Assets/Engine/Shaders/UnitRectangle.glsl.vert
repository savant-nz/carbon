/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

attribute vec3 vsPosition;

varying vec2 tcScreen;

void main()
{
    gl_Position = vec4(vsPosition.xy * 2.0 - 1.0, 0.0, 1.0);

    tcScreen = vsPosition.xy;
}
