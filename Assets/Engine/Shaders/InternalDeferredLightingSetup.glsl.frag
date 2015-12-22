/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ParallaxMapping.glsl.frag"

varying vec2 tcTextureMap;
varying mat3 tangentToWorld;

uniform sampler2D sNormalMap;
uniform sampler2D sGlossMap;

uniform float specularExponent;

void main()
{
    vec2 tc = parallaxMapping(tcTextureMap);

    vec3 tangentSpaceNormal = texture2D(sNormalMap, tc).xyz * 2.0 - 1.0;

    vec3 worldSpaceNormal = normalize(tangentToWorld * tangentSpaceNormal);

    // Compute final specular exponent based on the gloss value, this is then output in the alpha channel
    float alpha = max(specularExponent * texture2D(sGlossMap, tc).a, 2.0) / 255.0;

    gl_FragColor = vec4(worldSpaceNormal * 0.5 + vec3(0.5), alpha);
}
