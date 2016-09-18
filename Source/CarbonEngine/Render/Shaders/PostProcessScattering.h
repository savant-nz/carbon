/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class PostProcessScatteringGLSL : public Shader
{
public:

    const Vec3 betaRayleigh = {6.9549565e-006f, 1.1761114e-005f, 2.4387846e-005f};
    const Vec3 betaDashRayleigh = {4.1509335e-007f, 7.0193977e-007f, 1.4555422e-006f};
    const Vec3 betaMie = {0.0057405969f, 0.0073996861f, 0.010514311f};
    const Vec3 betaDashMie = {0.0013337875f, 0.0017344574f, 0.0024976188f};

    class PostProcessScatteringProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* sSceneTexture = nullptr;
        ShaderConstant* sDepthTexture = nullptr;
        ShaderConstant* lightDirection = nullptr;
        ShaderConstant* nearFarPlaneDistanceConstants = nullptr;
        ShaderConstant* projectionMatrixInverse = nullptr;
        ShaderConstant* sunColor = nullptr;
        ShaderConstant* betaRayleighPlusBetaMie = nullptr;
        ShaderConstant* invBetaRayleighPlusBetaMie = nullptr;
        ShaderConstant* betaDashRayleigh = nullptr;
        ShaderConstant* betaDashMie = nullptr;
        ShaderConstant* gValues = nullptr;
        ShaderConstant* extinctionFactor = nullptr;
        ShaderConstant* inscatteringFactor = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(sSceneTexture);
            CACHE_SHADER_CONSTANT(sDepthTexture);
            CACHE_SHADER_CONSTANT(lightDirection);
            CACHE_SHADER_CONSTANT(nearFarPlaneDistanceConstants);
            CACHE_SHADER_CONSTANT(projectionMatrixInverse);
            CACHE_SHADER_CONSTANT(sunColor);
            CACHE_SHADER_CONSTANT(betaRayleighPlusBetaMie);
            CACHE_SHADER_CONSTANT(invBetaRayleighPlusBetaMie);
            CACHE_SHADER_CONSTANT(betaDashRayleigh);
            CACHE_SHADER_CONSTANT(betaDashMie);
            CACHE_SHADER_CONSTANT(gValues);
            CACHE_SHADER_CONSTANT(extinctionFactor);
            CACHE_SHADER_CONSTANT(inscatteringFactor);
        }
    };

    PostProcessScatteringProgram program;

    PostProcessScatteringGLSL() : Shader("PostProcessScattering", 100, ShaderProgram::GLSL110) {}

    Shader::ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return PostProcess;
    }

    bool initialize() override
    {
        return program.setup(ShaderProgram::GLSL110, {"UnitRectangle.glsl.vert", "PostProcessScattering.glsl.frag"});
    }

    void uninitialize() override { program.clear(); }

    void enterShader() override
    {
        States::StateCacher::push();

        program.activate();

        program.sSceneTexture->setInteger(0);
        program.sDepthTexture->setInteger(1);
    }

    // Given the direction of the sun and the atmospheric turbidity this function returns the sun color to use for
    // atmospheric simulation. The different wavelengths of light scatter differently depending on the angle of the sun,
    // which leads to effects such as red/orange sunsets.
    //
    // The equations used are based on the paper "A Practical Analytic Model for Daylight" and its accompanying
    // implementation that can be found at http://www.cs.utah.edu/vissim/papers/sunsky/code/RiSunConstants.C
    Color computeSunlightColor(const Vec3& lightDirection, float turbidity, float sunIntensity)
    {
        // Get sun's zenith angle
        auto cosTheta = -lightDirection.y;
        auto theta = acosf(cosTheta);

        // Amount of aerosols present
        auto beta = 0.04608f * turbidity - 0.04586f;

        // Ratio of small to large particle sizes
        auto alpha = 1.3f;

        // Light wavelengths in um
        auto lambda = Vec3(0.65f, 0.57f, 0.475f);

        // Approximation of the relative optical mass
        auto m = 1.0f / (cosTheta + 0.15f * powf(Math::radiansToDegrees(1.63860f - theta), -1.253f));

        auto tau = Vec3::One;

        // Rayleigh scattering
        tau *= (lambda.pow(-4.08f) * 0.008735f * -m).exp();

        // Aerosol attenuation
        tau *= (lambda.pow(-alpha) * beta * -m).exp();

        // Scale by sun intensity;
        tau *= 100.0f * sunIntensity;

        // Return sunlight color
        return {tau.x, tau.y, tau.z, 1.0f};
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                         const ParameterArray& internalParams, unsigned int pass, unsigned int sortKey) override
    {
        program.setVertexAttributeArrayConfiguration(geometryChunk);

        auto invCameraOrientation = Matrix3();
        renderer().getCamera().getOrientation().getInverse(invCameraOrientation);
        program.lightDirection->setFloat3(invCameraOrientation * renderer().getDirectionalLightDirection());

        setTexture(0, internalParams[Parameter::inputTexture]);
        setTexture(1, internalParams[Parameter::depthTexture]);

        auto zNear = renderer().getCamera().getNearPlaneDistance();
        auto zFar = renderer().getCamera().getFarPlaneDistance();
        program.nearFarPlaneDistanceConstants->setFloat3(zNear * zFar, zFar, zFar - zNear);

        program.projectionMatrixInverse->setMatrix4Inverse(renderer().getCamera().getProjectionMatrix());

        program.sunColor->setFloat3(computeSunlightColor(renderer().getDirectionalLightDirection(),
                                                         params[Parameter::turbidity].getFloat(),
                                                         renderer().getDirectionalLightColor().a));

        // The rayleigh and mie factors are used to scale the relevant values passed to the shader
        auto rayleighFactor = params[Parameter::rayleighCoefficient].getFloat();
        auto mieFactor = params[Parameter::mieCoefficient].getFloat();
        auto g = params[Parameter::g].getFloat();

        program.betaRayleighPlusBetaMie->setFloat3(betaRayleigh * rayleighFactor + betaMie * mieFactor);
        program.invBetaRayleighPlusBetaMie->setFloat3((betaRayleigh * rayleighFactor + betaMie * mieFactor).pow(-1.0f));
        program.betaDashRayleigh->setFloat3(betaDashRayleigh * rayleighFactor);
        program.betaDashMie->setFloat3(betaDashMie * mieFactor);
        program.gValues->setFloat3(1.0f - g * g, 1.0f + g * g, 2.0f * g);
        program.extinctionFactor->setFloat(params);
        program.inscatteringFactor->setFloat(params);
    }

    void exitShader() override { States::StateCacher::pop(); }
};

CARBON_REGISTER_SHADER(PostProcessScatteringGLSL, OpenGLBase)

}

}
