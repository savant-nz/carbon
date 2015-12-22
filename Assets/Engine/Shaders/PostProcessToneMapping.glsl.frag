/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PostProcessToneMappingLuminance.glsl.frag"

varying vec2 tcScreen;

uniform sampler2D sInputTexture;
uniform sampler2D sAverageSceneLuminanceTexture;

uniform float exposure;
uniform float whitePoint;

float getLuminance(vec3 color);

void main()
{
    float adjustedAverageSceneLuminance = exposure / texture2D(sAverageSceneLuminanceTexture, vec2(0.5, 0.5)).r;

    vec4 scenePixel = texture2D(sInputTexture, tcScreen);
    float pixelLuminance = getLuminance(scenePixel.rgb);

    float Lxy = pixelLuminance * adjustedAverageSceneLuminance;
    float Ld = Lxy * (1.0 + Lxy / (whitePoint * whitePoint)) / (1.0 + Lxy);

    gl_FragColor = vec4(scenePixel.rgb / pixelLuminance * Ld, scenePixel.a);
}
