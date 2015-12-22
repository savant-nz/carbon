/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec2 tcScreen;

uniform sampler2D sInputTexture;

uniform float brightThreshold;

void main()
{
	vec4 c = texture2D(sInputTexture, tcScreen);

    if (c.r > brightThreshold || c.g > brightThreshold || c.b > brightThreshold)
        gl_FragColor = c;
    else
    	gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
}
