/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec3 eyeVector;

uniform vec3 cameraPosition;

void eyeVectorInTangentSpace(vec3 position, vec3 tangent, vec3 bitangent, vec3 normal)
{
    // Calculate vertex to eye vector
    vec3 eyeDirection = cameraPosition - position;

    // Rotate eye vector into tangent space
    eyeVector.x = dot(tangent, eyeDirection);
    eyeVector.y = dot(bitangent, eyeDirection);
    eyeVector.z = dot(normal, eyeDirection);
}

