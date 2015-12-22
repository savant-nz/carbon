/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec2 tcScreen;

uniform sampler2D sInputTexture;
uniform sampler2D sAddTexture;

uniform float addTextureFactor;
uniform float finalScale;

void main()
{
    // Sample the first texture and clamp values < 0
    vec4 clampedInput = max(vec4(0.0), texture2D(sInputTexture, tcScreen));

    // Sample the texture to overlay and multiply through by the alpha value
    vec4 addColor = texture2D(sAddTexture, tcScreen);
    addColor *= addColor.a;

    gl_FragColor = (clampedInput + addTextureFactor * addColor) * finalScale;
}
