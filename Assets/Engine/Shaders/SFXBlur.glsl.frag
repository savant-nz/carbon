/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec4 tcScreen;

uniform sampler2D sRefractionMap;

uniform vec4 diffuseColor;
uniform vec2 textureOffsets[6];

void main()
{
    vec2 tc = tcScreen.xy / tcScreen.w;

    // Sample refraction
    vec4 refraction;
    refraction  = texture2D(sRefractionMap, tc + textureOffsets[0]);
    refraction += texture2D(sRefractionMap, tc + textureOffsets[1]);
    refraction += texture2D(sRefractionMap, tc + textureOffsets[2]);
    refraction += texture2D(sRefractionMap, tc + textureOffsets[3]);
    refraction += texture2D(sRefractionMap, tc + textureOffsets[4]);
    refraction += texture2D(sRefractionMap, tc + textureOffsets[5]);
    refraction /= 6.0;

    gl_FragColor = refraction * diffuseColor;
}
