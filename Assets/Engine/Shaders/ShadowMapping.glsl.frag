/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef SHADOW_MAPPING

uniform sampler2DShadow sShadowMap;

varying vec4 lightSpace;

float shadowMapping()
{
    return shadow2D(sShadowMap, lightSpace.xyz / lightSpace.w).r;
}

#else

float shadowMapping()
{
    return 1.0;
}

#endif
