/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define FILTER_TAP_COUNT 13

varying vec2 tcScreen;

uniform sampler2D sInputTexture;

uniform vec3 offsetsAndWeights[FILTER_TAP_COUNT];
uniform float blurScale;
uniform vec4 color;

void main()
{
    vec4 samples = vec4(0.0);

    for (int i = 0; i < FILTER_TAP_COUNT; i++)
        samples += texture2D(sInputTexture, tcScreen + offsetsAndWeights[i].xy * blurScale) * offsetsAndWeights[i].z;

    gl_FragColor = samples * color;
}
