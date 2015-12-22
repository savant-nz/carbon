/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef SKELETAL_ANIMATION

#ifndef WEIGHTS_PER_VERTEX
    #define WEIGHTS_PER_VERTEX 4
#endif

attribute vec4 vsBones;
attribute vec4 vsWeights;

uniform vec4 boneTransforms[240]; // At most 80 bones can be referenced in a single drawcall

vec3 transformByBone(vec4 v, int boneIndex)
{
    return vec3(dot(v, boneTransforms[boneIndex]),
                dot(v, boneTransforms[boneIndex + 1]),
                dot(v, boneTransforms[boneIndex + 2]));
}

void skeletalAnimation(inout vec3 position, inout vec3 tangent, inout vec3 bitangent, inout vec3 normal)
{
    // Get bone indices as integers
    ivec4 boneIndices = ivec4(vsBones * 3.0);

    // Create 4-component versions of the inputs
    vec4 position4 = vec4(position, 1.0);
    vec4 tangent4 = vec4(tangent, 0.0);
    vec4 bitangent4 = vec4(bitangent, 0.0);
    vec4 normal4 = vec4(normal, 0.0);

    // 1st bone
    position = vsWeights[0] * transformByBone(position4, boneIndices[0]);
    tangent = vsWeights[0] * transformByBone(tangent4, boneIndices[0]);
    bitangent = vsWeights[0] * transformByBone(bitangent4, boneIndices[0]);
    normal = vsWeights[0] * transformByBone(normal4, boneIndices[0]);

#if WEIGHTS_PER_VERTEX > 1

    // 2nd bone
    position += vsWeights[1] * transformByBone(position4, boneIndices[1]);
    tangent += vsWeights[1] * transformByBone(tangent4, boneIndices[1]);
    bitangent += vsWeights[1] * transformByBone(bitangent4, boneIndices[1]);
    normal += vsWeights[1] * transformByBone(normal4, boneIndices[1]);

    #if WEIGHTS_PER_VERTEX > 2

        // 3rd bone
        position += vsWeights[2] * transformByBone(position4, boneIndices[2]);
        tangent += vsWeights[2] * transformByBone(tangent4, boneIndices[2]);
        bitangent += vsWeights[2] * transformByBone(bitangent4, boneIndices[2]);
        normal += vsWeights[2] * transformByBone(normal4, boneIndices[2]);

        #if WEIGHTS_PER_VERTEX > 3

            // 4th bone
            position += vsWeights[3] * transformByBone(position4, boneIndices[3]);
            tangent += vsWeights[3] * transformByBone(tangent4, boneIndices[3]);
            bitangent += vsWeights[3] * transformByBone(bitangent4, boneIndices[3]);
            normal += vsWeights[3] * transformByBone(normal4, boneIndices[3]);

        #endif

    #endif

    // Renormalize the animated tangent basis vectors
    tangent = normalize(tangent);
    bitangent = normalize(bitangent);
    normal = normalize(normal);

#endif
}

void skeletalAnimation(inout vec3 position)
{
    // Get bone indices as integers
    ivec4 boneIndices = ivec4(vsBones * 3.0);

    // Create 4-component version of the input
    vec4 position4 = vec4(position, 1.0);

    // 1st bone
    position = vsWeights[0] * transformByBone(position4, boneIndices[0]);

#if WEIGHTS_PER_VERTEX > 1

    // 2nd bone
    position += vsWeights[1] * transformByBone(position4, boneIndices[1]);

    #if WEIGHTS_PER_VERTEX > 2

        // 3rd bone
        position += vsWeights[2] * transformByBone(position4, boneIndices[2]);

        #if WEIGHTS_PER_VERTEX > 3

            // 4th bone
            position += vsWeights[3] * transformByBone(position4, boneIndices[3]);

        #endif

    #endif

#endif
}

#else

void skeletalAnimation(inout vec3 position, inout vec3 tangent, inout vec3 bitangent, inout vec3 normal)
{

}

void skeletalAnimation(inout vec3 position)
{

}

#endif
