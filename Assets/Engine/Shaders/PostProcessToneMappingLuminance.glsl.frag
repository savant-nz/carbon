/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

float getLuminance(vec3 color)
{
    return dot(color, vec3(0.27, 0.67, 0.06));
}

float getLogLuminance(vec3 color)
{
    float epsilon = 0.01;

    return log(max(epsilon, getLuminance(color)));
}
