/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SkeletalAnimation.glsl.vert"

attribute vec3 vsPosition;

uniform mat4 modelViewProjection;

void main()
{
    vec3 newPosition = vsPosition;

    skeletalAnimation(newPosition);

    gl_Position = modelViewProjection * vec4(newPosition, 1.0);
}
