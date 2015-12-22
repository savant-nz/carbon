/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "AmbientOcclusion.glsl.vert"
#include "DecalMapping.glsl.vert"
#include "EyeVectorInTangentSpace.glsl.vert"
#include "SkeletalAnimation.glsl.vert"

#ifdef NORMAL_MAPPING
    #define LIGHT_VECTOR
    #define TANGENT_BASIS
#endif

#ifdef SPECULAR
    #define EYE_VECTOR
    #define LIGHT_VECTOR
    #define TANGENT_BASIS
#endif

#ifdef PARALLAX_MAPPING
    #define EYE_VECTOR
    #define TANGENT_BASIS
#endif

attribute vec3 vsPosition;
attribute vec2 vsDiffuseTextureCoordinate;
varying vec2 tcTextureMap;
uniform mat4 modelViewProjection;

#ifdef LIGHT_VECTOR
    varying vec3 lightVector;
    uniform vec3 lightDirection;
#endif

#ifdef TANGENT_BASIS
    attribute vec3 vsTangent;
    attribute vec3 vsBitangent;
    attribute vec3 vsNormal;
#endif

#ifdef VERTEX_COLOR
    attribute vec4 vsColor;
    varying vec4 vertexColor;
#endif

void main()
{
    vec3 newPosition = vsPosition;

#ifdef TANGENT_BASIS
    vec3 newTangent = vsTangent;
    vec3 newBitangent = vsBitangent;
    vec3 newNormal = vsNormal;

    skeletalAnimation(newPosition, newTangent, newBitangent, newNormal);
#else
    skeletalAnimation(newPosition);
#endif

    gl_Position = modelViewProjection * vec4(newPosition, 1.0);

#ifdef VERTEX_COLOR
    vertexColor = vsColor;
#endif

#ifdef LIGHT_VECTOR
    // Rotate light vector into tangent space
    lightVector.x = dot(newTangent, -lightDirection);
    lightVector.y = dot(newBitangent, -lightDirection);
    lightVector.z = dot(newNormal, -lightDirection);
#endif

    // Pass texture coordinates through
    tcTextureMap = vsDiffuseTextureCoordinate;

#ifdef EYE_VECTOR
    eyeVectorInTangentSpace(newPosition, newTangent, newBitangent, newNormal);
#endif

    ambientOcclusion();
    decalMapping();
}
