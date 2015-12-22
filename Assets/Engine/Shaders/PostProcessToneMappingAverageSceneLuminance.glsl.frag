/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PostProcessToneMappingLuminance.glsl.frag"

uniform sampler2D sInputTexture;
uniform sampler2D sPreviousAverageSceneLuminanceTexture;

uniform float weighting;

const int samples = 32;
const float step = 1.0 / float(samples - 1);
const float scale = 1.0 / float(samples * samples);

void main()
{
    float result = 0.0;
    vec2 location = vec2(0.0);

    for (int x = 0; x < samples; x++)
    {
        for (int y = 0; y < samples; y++)
        {
            result += getLogLuminance(texture2D(sInputTexture, location).rgb);
            location.y += step;
        }

        location.x += step;
        location.y = 0.0;
    }

    float sceneLuminance = exp(result / float(samples * samples));

    float previousSceneLuminance = texture2D(sPreviousAverageSceneLuminanceTexture, vec2(0.5, 0.5)).r;

    gl_FragColor = vec4(mix(previousSceneLuminance, sceneLuminance, weighting));
}
