/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef DECAL_MAPPING

varying vec2 tcDecalMap;

uniform sampler2D sDecalMap;
uniform sampler2D sDecalGlossMap;

vec4 decalMapping(vec4 diffuseColor)
{
    vec4 decalColor = texture2D(sDecalMap, tcDecalMap);

    return mix(diffuseColor, decalColor, decalColor.a);
}

vec4 decalMappingGloss(vec4 gloss)
{
    vec4 decalColor = texture2D(sDecalMap, tcDecalMap);

    return mix(gloss, texture2D(sDecalGlossMap, tcDecalMap), decalColor.a);
}

#else

vec4 decalMapping(vec4 diffuseColor)
{
    return diffuseColor;
}

vec4 decalMappingGloss(vec4 gloss)
{
    return gloss;
}

#endif
