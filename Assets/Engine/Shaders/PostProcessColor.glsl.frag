/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec2 tcScreen;

uniform sampler2D sInputTexture;

uniform vec4 color;

void main()
{
    gl_FragColor = texture2D(sInputTexture, tcScreen) * color;
}
