/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "EyeVectorInTangentSpace.glsl.vert"
#include "SkeletalAnimation.glsl.vert"

attribute vec3 vsPosition;
attribute vec2 vsDiffuseTextureCoordinate;
attribute vec3 vsTangent;
attribute vec3 vsBitangent;
attribute vec3 vsNormal;

uniform mat4 modelViewProjection;

varying vec2 tcTextureMap;

void main()
{
    vec3 newPosition = vsPosition;
    vec3 newTangent = vsTangent;
    vec3 newBitangent = vsBitangent;
    vec3 newNormal = vsNormal;

    skeletalAnimation(newPosition, newTangent, newBitangent, newNormal);

    gl_Position = modelViewProjection * vec4(newPosition, 1.0);

    tcTextureMap = vsDiffuseTextureCoordinate;

    eyeVectorInTangentSpace(newPosition, newTangent, newBitangent, newNormal);
}
