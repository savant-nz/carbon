/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef SHADOW_MAPPING

varying vec4 lightSpace;

uniform mat4 lightViewProjectionMatrix;

void shadowMapping(vec3 vertex)
{
    lightSpace = lightViewProjectionMatrix * vec4(vertex, 1.0);
}

#else

void shadowMapping(vec3 vertex)
{

}

#endif
