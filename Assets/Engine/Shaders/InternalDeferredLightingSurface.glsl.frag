/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AmbientOcclusion.glsl.frag"
#include "DecalMapping.glsl.frag"
#include "ParallaxMapping.glsl.frag"

varying vec2 tcDiffuseMap;
varying vec4 tcLightingTexture;

uniform sampler2D sDiffuseMap;
uniform sampler2D sGlossMap;
uniform sampler2D sLightingTexture;

uniform vec3 diffuseColor;
uniform vec3 specularColor;

void main()
{
    // Apply parallax mapping to the main texture coordinates
    vec2 tc = parallaxMapping(tcDiffuseMap);

    vec4 lightingSample = texture2D(sLightingTexture, tcLightingTexture.xy / tcLightingTexture.w);
    lightingSample *= ambientOcclusion();

    vec3 diffuse = decalMapping(texture2D(sDiffuseMap, tc)).rgb * diffuseColor * clamp(lightingSample.rgb, 0.0, 1.0);

    vec4 glossSample = decalMappingGloss(texture2D(sGlossMap, tc));
    vec3 specular = specularColor * lightingSample.a * glossSample.rgb;

    gl_FragColor = vec4(diffuse + specular, 1.0);
}
