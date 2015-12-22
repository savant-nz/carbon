/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

namespace Shaders
{

class PostProcessBlurGLSL : public Shader
{
public:

    static const auto FilterTapCount = 13U;

    class PostProcessBlurProgram : public ManagedShaderProgram
    {
    public:

        ShaderConstant* sInputTexture = nullptr;
        ShaderConstant* blurScale = nullptr;
        ShaderConstant* offsetsAndWeights = nullptr;
        ShaderConstant* color = nullptr;

        void cache() override
        {
            CACHE_SHADER_CONSTANT(sInputTexture);
            CACHE_SHADER_CONSTANT(blurScale);
            CACHE_SHADER_CONSTANT(offsetsAndWeights);
            CACHE_SHADER_CONSTANT(color);
        }
    };

    PostProcessBlurProgram program;

    PostProcessBlurGLSL() : Shader("PostProcessBlur", 100, ShaderProgram::GLSL110) {}

    Shader::ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const override
    {
        return PostProcess;
    }

    bool initialize() override
    {
        return program.setup(ShaderProgram::GLSL110, {"UnitRectangle.glsl.vert", "PostProcessBlur.glsl.frag"});
    }

    void uninitialize() override { program.clear(); }

    void enterShader() override
    {
        States::StateCacher::push();

        program.activate();

        program.sInputTexture->setInteger(0);
    }

    void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params, const ParameterArray& internalParams,
                         unsigned int pass, unsigned int sortKey) override
    {
        // Get the input texture to blur
        auto t = static_cast<Texture2D*>(internalParams[Parameter::inputTexture].getPointer<Texture>());
        if (!t)
            return;

        program.blurScale->setFloat(params);
        program.color->setFloat4(params.get(Parameter::color, Color::White).getColor());

        // Get the type of blur to do
        enum
        {
            BlurHorizontal,
            BlurVertical,
            Blur2D
        } blurType;

        auto& type = params[Parameter::blurType].getString();
        if (type == "horizontal")
            blurType = BlurHorizontal;
        else if (type == "vertical")
            blurType = BlurVertical;
        else if (type == "2D")
            blurType = Blur2D;
        else
            return;

        auto standardDeviation = params[Parameter::blurStandardDeviation].getFloat();

        // Calculate the texture offsets and weights needed for the blur
        auto weights = std::array<float, FilterTapCount>();
        auto offsetsAndWeights = std::array<std::array<float, 3>, FilterTapCount>();

        if (blurType == Blur2D)
        {
            auto offsets = std::array<Vec2, FilterTapCount>();
            getSampleOffsets2D(t->getWidth(), t->getHeight(), offsets, weights, standardDeviation);
            for (auto i = 0U; i < FilterTapCount; i++)
            {
                offsetsAndWeights[i][0] = offsets[i].x;
                offsetsAndWeights[i][1] = offsets[i].y;
            }
        }
        else
        {
            auto offsets = std::array<float, FilterTapCount>();

            if (blurType == BlurHorizontal)
                getSampleOffsets1D(t->getWidth(), offsets, weights, standardDeviation);
            else
                getSampleOffsets1D(t->getHeight(), offsets, weights, standardDeviation);

            for (auto i = 0U; i < FilterTapCount; i++)
            {
                if (blurType == BlurHorizontal)
                {
                    offsetsAndWeights[i][0] = offsets[i];
                    offsetsAndWeights[i][1] = 0.0f;
                }
                else
                {
                    offsetsAndWeights[i][0] = 0.0f;
                    offsetsAndWeights[i][1] = offsets[i];
                }
            }
        }

        // Copy weights
        for (auto i = 0U; i < FilterTapCount; i++)
            offsetsAndWeights[i][2] = weights[i];

        // Upload the texture offsets and weights as an array of vec3's
        program.offsetsAndWeights->setArray(3, FilterTapCount, &offsetsAndWeights[0][0]);

        program.setVertexAttributeArrayConfiguration(geometryChunk);

        setTexture(0, t);
    }

    void exitShader() override { States::StateCacher::pop(); }

private:

    // Calculates offsets and weights for a 13-tap 1D gaussian blur.
    void getSampleOffsets1D(unsigned int textureSize, std::array<float, FilterTapCount>& offsets,
                            std::array<float, FilterTapCount>& weights, float standardDeviation)
    {
        auto dt = 1.0f / float(textureSize);

        auto totalWeight = 0.0f;

        for (auto i = 0U; i < FilterTapCount; i++)
        {
            auto texel = float(int(i) - int(FilterTapCount) / 2);

            offsets[i] = texel * dt;
            weights[i] = Math::normalDistribution(texel, 0.0f, standardDeviation);

            totalWeight += weights[i];
        }

        // Normalize so the weights sum to one
        for (auto& weight : weights)
            weight /= totalWeight;
    }

    // Calculates offsets and weights for a 13-tap 2D gaussian blur.
    void getSampleOffsets2D(unsigned int textureWidth, unsigned int textureHeight, std::array<Vec2, FilterTapCount>& offsets,
                            std::array<float, FilterTapCount>& weights, float standardDeviation)
    {
        static const auto samplePoints = std::array<Vec2, FilterTapCount>{{{0.0f, 0.0f},
                                                                           {-0.32621f, -0.40581f},
                                                                           {-0.84014f, -0.07358f},
                                                                           {-0.69591f, 0.45714f},
                                                                           {-0.20335f, 0.62072f},
                                                                           {0.96234f, -0.19498f},
                                                                           {0.47343f, -0.48003f},
                                                                           {0.51946f, 0.76702f},
                                                                           {0.18546f, -0.89312f},
                                                                           {0.50743f, 0.06443f},
                                                                           {0.89642f, 0.41246f},
                                                                           {-0.32194f, -0.93262f},
                                                                           {-0.79156f, -0.59771f}}};

        auto dx = 1.0f / float(textureWidth);
        auto dy = 1.0f / float(textureHeight);

        auto totalWeight = 0.0f;

        for (auto i = 0U; i < FilterTapCount; i++)
        {
            offsets[i].x = samplePoints[i].x * dx;
            offsets[i].y = samplePoints[i].y * dy;

            weights[i] = Math::normalDistribution(samplePoints[i].length(), 0.0f, standardDeviation);

            totalWeight += weights[i];
        }

        // Normalize so the weights sum to one
        for (auto& weight : weights)
            weight /= totalWeight;
    }
};

CARBON_REGISTER_SHADER(PostProcessBlurGLSL, OpenGLBase)

}

}
