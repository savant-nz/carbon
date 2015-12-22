/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "Specular.glsl.frag"
#include "AmbientOcclusion.glsl.frag"
#include "DecalMapping.glsl.frag"
#include "ParallaxMapping.glsl.frag"

#ifdef NORMAL_MAPPING
    #define NORMAL
#endif

#ifdef SPECULAR
    #define NORMAL
#endif

varying vec2 tcTextureMap;
uniform sampler2D sDiffuseMap;
uniform vec4 diffuseColor;
uniform vec4 lightColor;
uniform vec4 lightAmbient;

#ifdef NORMAL
    uniform sampler2D sNormalMap;
    varying vec3 lightVector;
#endif

#ifdef SPECULAR
    uniform sampler2D sGlossMap;
#endif

#ifdef VERTEX_COLOR
    varying vec4 vertexColor;
#endif

void main()
{
    // Apply parallax mapping to the main texture coordinates
    vec2 tc = parallaxMapping(tcTextureMap);

#ifdef NORMAL
    // Expand normal
    vec3 N = vec3(texture2D(sNormalMap, tc)) * 2.0 - 1.0;
#endif

#ifdef NORMAL_MAPPING
    // Compute N.L term
    float NDotL = max(dot(N, lightVector), 0.0);
#else
    float NDotL = 1.0;
#endif

    // Compute final diffuse color
    vec4 diffuseMapSample = decalMapping(texture2D(sDiffuseMap, tc));
    vec4 diffuse = diffuseMapSample * diffuseColor;

#ifdef VERTEX_COLOR
    diffuse *= vertexColor;
#endif

    // Compute specular term
#ifdef SPECULAR
    vec3 specular = specular(N, decalMappingGloss(texture2D(sGlossMap, tc)), lightVector);
#else
    vec3 specular = vec3(0.0);
#endif

    gl_FragColor   = diffuse * clamp(lightColor * NDotL * ambientOcclusion() + lightAmbient, 0.0, 1.0) +
                     vec4(specular, 1.0) * lightColor;
    gl_FragColor.a = diffuse.a;
}
