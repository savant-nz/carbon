/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class InternalSpriteGLSL : public Shader
{
public:

    class InternalSpriteProgram : public ManagedShaderProgram
    {
    public:

        InternalSpriteProgram(unsigned int lightCount_, unsigned int spotLightCount_)
            : lightCount(lightCount_),
              spotLightCount(spotLightCount_),
              lightPosition(lightCount),
              lightColor(lightCount),
              lightRadius(lightCount),
              spotConstants(spotLightCount),
              lightDirection(spotLightCount)
        {
            program_ = graphics().createShaderProgram(ShaderProgram::GLSL110);
        }

        const unsigned int lightCount;
        const unsigned int spotLightCount;

        ShaderConstant* modelViewProjection = nullptr;
        ShaderConstant* scaleAndOffset = nullptr;
        ShaderConstant* sDiffuseMap = nullptr;
        ShaderConstant* sNormalMap = nullptr;
        ShaderConstant* diffuseColor = nullptr;
        ShaderConstant* lightAmbient = nullptr;
        ShaderConstant* currentScale = nullptr;

        Vector<ShaderConstant*> lightPosition;
        Vector<ShaderConstant*> lightColor;
        Vector<ShaderConstant*> lightRadius;
        Vector<ShaderConstant*> spotConstants;
        Vector<ShaderConstant*> lightDirection;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(modelViewProjection);
            CACHE_SHADER_CONSTANT(scaleAndOffset);
            CACHE_SHADER_CONSTANT(sDiffuseMap);
            CACHE_SHADER_CONSTANT(diffuseColor);
            CACHE_SHADER_CONSTANT(lightAmbient);

            if (lightCount)
            {
                CACHE_SHADER_CONSTANT(sNormalMap);
                CACHE_SHADER_CONSTANT(currentScale);
            }

            for (auto i = 0U; i < lightCount; i++)
            {
                lightPosition[i] = program_->getConstant(String() + "lightPosition" + i, String::Empty);
                lightColor[i] = program_->getConstant(String() + "lightColor" + i, String::Empty);
                lightRadius[i] = program_->getConstant(String() + "lightRadius" + i, String::Empty);
            }

            for (auto i = 0U; i < spotLightCount; i++)
            {
                spotConstants[i] = program_->getConstant(String() + "spotConstants" + i, String::Empty);
                lightDirection[i] = program_->getConstant(String() + "lightDirection" + i, String::Empty);
            }
        }
    };

    // A separate program is generated for each (light count, spot light count) pair
    std::map<std::pair<unsigned int, unsigned int>, std::unique_ptr<InternalSpriteProgram>> programs;
    InternalSpriteProgram* currentProgram = nullptr;

    InternalSpriteGLSL() : Shader("InternalSprite", 100, ShaderProgram::GLSL110) {}

    ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return Blended;
    }

    void uninitialize() override { programs.clear(); }

    void precache() override
    {
        // Precache commonly used variants: 0, 1 and 2 point lights each with 0 and 1 spot lights
        for (auto i = 0U; i < 3; i++)
        {
            for (auto j = 0U; j < 2; j++)
                updateCurrentProgram(i + j, j);
        }

        currentProgram = nullptr;
    }

    void enterShader() override
    {
        States::StateCacher::push();

        currentProgram = nullptr;
    }

    bool updateCurrentProgram(unsigned int lightCount, unsigned int spotLightCount)
    {
        if (currentProgram && lightCount == currentProgram->lightCount &&
            spotLightCount != currentProgram->spotLightCount)
            return true;

        auto lightConfiguration = std::make_pair(lightCount, spotLightCount);

        auto it = programs.find(lightConfiguration);

        if (it == programs.end())
        {
            // Create a new shader program for this light count
            programs[lightConfiguration] = createProgram(lightCount, spotLightCount);
            it = programs.find(lightConfiguration);
        }

        currentProgram = it->second.get();

        if (!currentProgram)
            return false;

        currentProgram->activate();
        currentProgram->sDiffuseMap->setInteger(0);
        if (currentProgram->sNormalMap)
            currentProgram->sNormalMap->setInteger(1);

        return true;
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                         const ParameterArray& internalParams, unsigned int pass, unsigned int sortKey) override
    {
        auto lights = Vector<const Renderer::Light*>();
        auto lightAmbient = Color();

        if (params[Parameter::isLightingAllowed].getBoolean() &&
            renderer().gatherLights(geometryChunk.getAABB(), lights))
            lightAmbient = renderer().getAmbientLightColor();
        else
            lightAmbient = Color::White;

        auto spotLightCount = 0U;
        for (auto light : lights)
        {
            if (light->isSpotLight())
                spotLightCount++;
        }

        if (!updateCurrentProgram(lights.size(), spotLightCount))
            return;

        currentProgram->setVertexAttributeArrayConfiguration(geometryChunk);

        // Set constants
        currentProgram->modelViewProjection->setMatrix4(renderer().getModelViewProjectionMatrix());
        currentProgram->diffuseColor->setFloat4(params[Parameter::diffuseColor].getColor());

        auto scaleAndOffset = params[Parameter::scaleAndOffset].getFloat4();
        currentProgram->scaleAndOffset->setFloat4(scaleAndOffset[0], scaleAndOffset[1], scaleAndOffset[2],
                                                  scaleAndOffset[3]);
        if (currentProgram->currentScale)
        {
            currentProgram->currentScale->setFloat3(
                renderer().getCurrentScale() *
                Vec3(Math::getSign(scaleAndOffset[0]), Math::getSign(scaleAndOffset[1]), 1.0f));
        }

        // Loop over lights and set their corresponding shader constants
        for (auto i = 0U; i < lights.size(); i++)
        {
            auto light = lights[i];

            currentProgram->lightColor[i]->setFloat3(light->getColor());

            if (light->isPointLight() || light->isSpotLight())
            {
                currentProgram->lightPosition[i]->setFloat4(
                    renderer().getCurrentTransformInverseMatrix() * light->getLightTransform().getPosition(), 1.0f);
                currentProgram->lightRadius[i]->setFloat(light->getRadius() * light->getRadius());

                // If this is a spotlight then set the additional spotlight constants
                if (light->isSpotLight())
                {
                    currentProgram->spotConstants[lights.size() - i - 1]->setFloat2(
                        cosf(light->getMinimumConeAngle()),
                        1.0f / (cosf(light->getMaximumConeAngle()) - cosf(light->getMinimumConeAngle())));
                    currentProgram->lightDirection[lights.size() - i - 1]->setFloat3(
                        renderer().getCurrentOrientationInverseMatrix() * light->getLightTransform().getDirection() *
                        Vec3(-1.0f, 1.0f, 1.0f));
                }
            }
            else if (light->isDirectionalLight())
            {
                currentProgram->lightPosition[i]->setFloat4(
                    renderer().getCurrentOrientationInverseMatrix() * -light->getLightTransform().getDirection(), 0.0f);
                currentProgram->lightRadius[i]->setFloat(1000000.0f);
            }
        }

        // Set the ambient light
        currentProgram->lightAmbient->setFloat3(lightAmbient);

        // Set textures
        setTexture(0, params[Parameter::diffuseMap], renderer().getErrorTexture());
        if (currentProgram->sNormalMap)
        {
            if (lights.size() && params.has(Parameter::normalMap))
                setTexture(1, params[Parameter::normalMap], renderer().getFlatNormalMap());
            else
                setTexture(1, renderer().getFlatNormalMap());
        }

        Blending::setShaderParams(params);
    }

    void exitShader() override { States::StateCacher::pop(); }

    std::unique_ptr<InternalSpriteProgram> createProgram(unsigned int lightCount, unsigned int spotLightCount)
    {
        auto vp = String("attribute vec3 vsPosition;\n"
                         "attribute vec2 vsDiffuseTextureCoordinate;\n"
                         "attribute vec3 vsTangent;\n"
                         "attribute vec3 vsBitangent;\n"
                         "attribute vec3 vsNormal;\n"
                         "uniform mat4 modelViewProjection;\n"
                         "uniform vec4 scaleAndOffset;\n"
                         "uniform vec3 currentScale;\n"
                         "varying vec2 tcTextureMap;\n");

        for (auto i = 0U; i < lightCount; i++)
        {
            vp << "uniform vec4 lightPosition" << i << ";\n";
            vp << "varying vec3 lightVector" << i << ";\n";
        }

        vp << "void main()\n";
        vp << "{\n";
        vp << "    gl_Position = modelViewProjection * vec4(vsPosition, 1.0);\n";
        vp << "    tcTextureMap = scaleAndOffset.xy * vsDiffuseTextureCoordinate + scaleAndOffset.zw;\n";
        if (lightCount)
            vp << "    vec3 lightDirection;\n";

        // Calculate all the light direction vectors, the light position w coordinate is zero for directional lights
        for (auto i = 0U; i < lightCount; i++)
        {
            vp << "    lightDirection = vsPosition * lightPosition" << i << ".w - lightPosition" << i << ".xyz;\n";
            vp << "    lightVector" << i << ".x = dot(vsTangent, -lightDirection);\n";
            vp << "    lightVector" << i << ".y = dot(vsBitangent, -lightDirection);\n";
            vp << "    lightVector" << i << ".z = dot(vsNormal, -lightDirection);\n";
            vp << "    lightVector" << i << " *= currentScale;\n";
        }

        vp << "}\n";

        auto fp = String("varying vec2 tcTextureMap;\n"
                         "uniform sampler2D sDiffuseMap;\n"
                         "uniform sampler2D sNormalMap;\n"
                         "uniform vec4 diffuseColor;\n"
                         "uniform vec3 lightAmbient;\n");

        for (auto i = 0U; i < lightCount; i++)
        {
            fp << "varying vec3 lightVector" << i << ";\n";
            fp << "uniform vec3 lightColor" << i << ";\n";
            fp << "uniform float lightRadius" << i << ";\n";
        }

        for (auto i = 0U; i < spotLightCount; i++)
        {
            fp << "uniform vec3 lightDirection" << i << ";\n";
            fp << "uniform vec2 spotConstants" << i << ";\n";
        }

        fp << "void main()\n";
        fp << "{\n";
        fp << "    vec4 surfaceColor = texture2D(sDiffuseMap, tcTextureMap) * diffuseColor;\n";
        fp << "    vec3 L = lightAmbient;\n";

        if (lightCount)
        {
            fp << "    vec3 normal = vec3(texture2D(sNormalMap, tcTextureMap)) * 2.0 - 1.0;\n";
            fp << "    float distance, attenuation, nDotL;\n";
        }

        // Accumulate lighting contribution from all lights
        for (auto i = 0U; i < lightCount; i++)
        {
            fp << "    distance = length(lightVector" << i << ");\n";
            fp << "    attenuation = max(0.0, 1.0 - distance * distance / lightRadius" << i << ");\n";

            // Spot attenuation, no dot(N, L) term is done on spotlights at the moment
            if (lightCount - i <= spotLightCount)
            {
                fp << "    nDotL = 1.0;\n";
                fp << "    attenuation *= clamp(1.0 - (dot(lightDirection" << (lightCount - i - 1) << ".xy, lightVector"
                   << i << ".xy / distance) - spotConstants" << (lightCount - i - 1) << ".x) * spotConstants"
                   << (lightCount - i - 1) << ".y, 0.0, 1.0);\n";
            }
            else
                fp << "    nDotL = max(dot(normal, lightVector" << i << " / distance), 0.0);\n";

            fp << "    L += attenuation * nDotL * lightColor" << i << ";\n";
        }

        fp << "    gl_FragColor = surfaceColor * vec4(L, 1.0);\n";
        fp << "}\n";

        auto newProgram = std::make_unique<InternalSpriteProgram>(lightCount, spotLightCount);
        newProgram->getProgram()->addSource(vp, "InternalSprite.glsl.vert");
        newProgram->getProgram()->addSource(fp, "InternalSprite.glsl.frag");

        try
        {
            if (!newProgram->getProgram()->link() || !newProgram->mapVertexAttributes())
                throw Exception();

            newProgram->cache();
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;
            return nullptr;
        }

        return newProgram;
    }
};

CARBON_REGISTER_SHADER(InternalSpriteGLSL, OpenGLBase)

}

}
