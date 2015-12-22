/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

varying vec2 tcScreen;

uniform sampler2D sSceneTexture;
uniform sampler2D sDepthTexture;

uniform mat4 projectionMatrixInverse;

uniform vec3 nearFarPlaneDistanceConstants; // near * far, far, far - near

uniform vec3 sunColor;
uniform vec3 lightDirection;

uniform vec3 betaRayleighPlusBetaMie;
uniform vec3 invBetaRayleighPlusBetaMie;
uniform vec3 betaDashRayleigh;
uniform vec3 betaDashMie;

uniform vec3 gValues; // 1.0 - g*g, 1.0 + g*g, 2.0 * g

uniform float extinctionFactor;
uniform float inscatteringFactor;

float rayleighPhase(float cosTheta)
{
    return 1.0 + cosTheta * cosTheta;
}

float miePhase(float cosTheta)
{
    return pow(gValues.y - gValues.z * cosTheta, -1.5) * gValues.x;
}

void main()
{
    // Distance to the surface
    float distance = nearFarPlaneDistanceConstants.x / (nearFarPlaneDistanceConstants.y -
                     texture2D(sDepthTexture, tcScreen).r * nearFarPlaneDistanceConstants.z);

    // Direction to the surface in camera space
    vec4 eyeVector = projectionMatrixInverse[0] * (tcScreen.x * 2.0 - 1.0) +
                     projectionMatrixInverse[1] * (tcScreen.y * 2.0 - 1.0) +
                     projectionMatrixInverse[3];

    // Angle between the eye and the light direction at the surface
    float cosTheta = max(0.0, -dot(normalize(eyeVector.xyz), lightDirection));

    // Extinction
    vec3 E1 = -betaRayleighPlusBetaMie * distance;
    E1.x = exp(E1.x);
    E1.y = exp(E1.y);
    E1.z = exp(E1.z);
    vec3 extinction = E1 * extinctionFactor * vec3(0.0138, 0.0113, 0.008);

    // Inscattering, combine rayleigh and mie values
    vec3 inscattering = betaDashRayleigh * rayleighPhase(cosTheta) + betaDashMie * miePhase(cosTheta);
    inscattering *= invBetaRayleighPlusBetaMie;

    // Scale according to extinction and final inscattering factor
    inscattering *= 1.0 - E1;
    inscattering *= inscatteringFactor;

    // Multiply by sun color
    inscattering *= sunColor;
    extinction   *= sunColor;

    // Put together the extinction and inscattering
    gl_FragColor = texture2D(sSceneTexture, tcScreen) * vec4(extinction, 1.0) + vec4(inscattering, 0.0);
}
