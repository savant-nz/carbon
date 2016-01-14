/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Graphics/ShaderConstant.h"
#include "CarbonEngine/Graphics/States/StateCacher.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Shaders/ShaderRegistry.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/TextureCubemap.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

namespace Carbon
{

const UnicodeString Shader::ShaderDirectory = "Shaders/";

Shader::Shader(String effectName, unsigned int quality, ShaderProgram::ShaderLanguage requiredShaderLanguage)
    : effectName_(std::move(effectName)), quality_(quality), requiredShaderLanguage_(requiredShaderLanguage)
{
}

Shader::~Shader()
{
    cleanup();
}

bool Shader::setup()
{
    if (needsInitialize_)
    {
        needsInitialize_ = false;
        isSetup_ = initialize();

        if (!isSetup_)
        {
            LOG_ERROR << className_ << " - Initialization failed";
            uninitialize();
        }
    }

    return isSetup_;
}

void Shader::cleanup()
{
    if (isSetup_)
        uninitialize();

    isSetup_ = false;
    needsInitialize_ = true;
}

bool Shader::prepareParameters(ParameterArray& parameters, Vector<const Texture*>& textureReferences) const
{
    auto effect = effects().getEffect(getEffectName());
    if (!effect)
        return false;

    // Go through all parameters
    for (const auto& parameter : parameters)
    {
        const auto& effectParameter = effect->getParameter(parameter.getName());

        // If a texture parameter is found then load/reference the texture
        if (effectParameter.isTexture())
        {
            auto& textureName = parameter.getValue().getString();

            // Load/reference this texture. The built-in textures are special cased here, and there is no need to reference them
            // as they are owned by the renderer.

            auto lowerTextureName = textureName.asLower();

            auto texture = pointer_to<const Texture>::type();
            if (lowerTextureName == "white" || lowerTextureName == "white.png")
                texture = renderer().getWhiteTexture();
            else if (lowerTextureName == "black" || lowerTextureName == "black.png")
                texture = renderer().getBlackTexture();
            else if (lowerTextureName == "whitecube")
                texture = renderer().getWhiteCubemapTexture();
            else if (lowerTextureName == "blackcube")
                texture = renderer().getBlackCubemapTexture();
            else if (lowerTextureName == "flatnormalmap" || lowerTextureName == "flatnormalmap.png" ||
                     lowerTextureName == "nonormalmap" || lowerTextureName == "nonormalmap.png")
                texture = renderer().getFlatNormalMap();
            else
            {
                texture = textures().setupTexture(effectParameter.textureType, textureName, effectParameter.textureGroup);

                // Record that we have a reference to this texture
                textureReferences.append(texture);
            }

            // Put the texture parameter's corresponding texture pointer into the parameters, the Shader then uses this to
            // access the texture
            parameters.set(Parameter::getHiddenParameterName(effectParameter.name), texture);
        }
        else
        {
            // Convert blending factors from the text representation to a enum value and store the result in a hidden parameter

            static const auto blendSourceFactorParameter = ParameterArray::Lookup("blendSourceFactor");
            static const auto blendDestinationFactorParameter = ParameterArray::Lookup("blendDestinationFactor");

            if (parameter.getLookup() == blendSourceFactorParameter)
            {
                parameters.set(Parameter::blendSourceFactor,
                               States::convertBlendFactorStringToEnum(parameter.getValue().getString()));
            }
            else if (parameter.getLookup() == blendDestinationFactorParameter)
            {
                parameters.set(Parameter::blendDestinationFactor,
                               States::convertBlendFactorStringToEnum(parameter.getValue().getString()));
            }
        }
    }

    return true;
}

bool Shader::ManagedShaderProgram::setup(ShaderProgram::ShaderLanguage language, const Vector<String>& sourceFiles,
                                         const String& preprocessorDefines)
{
    clear();

    program_ = graphics().createShaderProgram(language);
    if (!program_)
        return false;

    program_->setPreprocessorDefines(preprocessorDefines);

    for (auto& sourceFile : sourceFiles)
    {
        if (sourceFile.length())
        {
            auto filename = A(ShaderDirectory) + sourceFile;

            // Read the source file contents
            auto source = String();
            if (!fileSystem().readTextFile(filename, source))
            {
                LOG_ERROR << filename << " - Failed reading shader file contents";
                clear();
                return false;
            }

            // Add the source to this shader program
            if (!program_->addSource(source, filename))
            {
                clear();
                return false;
            }
        }
    }

    try
    {
        if (!program_->link())
            throw Exception("Failed linking program");

        if (!mapVertexAttributes())
            throw Exception("Failed mapping vertex attributes");

        cache();
    }
    catch (const Exception& e)
    {
        LOG_ERROR << e;

        clear();
        return false;
    }

    return true;
}

void Shader::ManagedShaderProgram::clear()
{
    if (program_)
    {
        graphics().deleteShaderProgram(program_);
        program_ = nullptr;
    }
}

void Shader::ManagedShaderProgram::activate()
{
    States::ShaderProgram = program_;
    States::ShaderProgram.flush();
}

bool Shader::ManagedShaderProgram::mapVertexAttributes()
{
    if (!program_)
        return false;

    for (const auto& attribute : program_->getVertexAttributes())
    {
        mappedVertexAttributes_.emplace(VertexStream::streamNameToType(attribute.withoutPrefix("vs")),
                                        program_->getVertexAttributeIndex(attribute));
    }

    return true;
}

void Shader::setVertexAttributeArray(const GeometryChunk& geometryChunk, unsigned int attributeIndex, unsigned int streamType)
{
    auto source = geometryChunk.getArraySourceForVertexStream(streamType);

    // Only turn on the vertex attribute array if the array source is actually valid
    if (source.isValid())
    {
        States::VertexAttributeArrayEnabled[attributeIndex] = true;
        States::VertexAttributeArraySource[attributeIndex] = source;
    }
}

void Shader::ManagedShaderProgram::setVertexAttributeArrayConfiguration(const GeometryChunk& geometryChunk)
{
    GraphicsInterface::VertexAttributeArrayConfigurationObject configuration = nullptr;

    // Look for a vertex attribute array configuration for this shader program on the geometry chunk
    auto index = geometryChunk.shaderProgramVertexAttributeArrayConfigurations_.findBy([=](
        const GeometryChunk::ShaderProgramVertexAttributeArrayConfiguration& config) { return config.program == program_; });

    if (index != -1)
        configuration = geometryChunk.shaderProgramVertexAttributeArrayConfigurations_[index].configuration;
    else
    {
        // Create new a vertex attribute array configuration for this shader and store it on the passed geometry chunk
        auto sources = Vector<GraphicsInterface::ArraySource>(mappedVertexAttributes_.size());
        for (const auto& mappedVertexAttribute : mappedVertexAttributes_)
        {
            auto vertexStreamType = mappedVertexAttribute.vertexStreamType;

            auto source = geometryChunk.getArraySourceForVertexStream(vertexStreamType);

            if (source.isValid())
                sources[mappedVertexAttribute.index] = source;
        }

        configuration = graphics().createVertexAttributeArrayConfiguration(sources);

        geometryChunk.shaderProgramVertexAttributeArrayConfigurations_.emplace(program_, configuration);
    }

    // If there is an available vertex attribute array configuration then use it, otherwise fall back to standard vertex
    // attribute arrays
    if (configuration)
        States::VertexAttributeArrayConfiguration = configuration;
    else
    {
        for (const auto& mappedVertexAttribute : mappedVertexAttributes_)
            setVertexAttributeArray(geometryChunk, mappedVertexAttribute.index, mappedVertexAttribute.vertexStreamType);
    }
}

void Shader::setTexture(unsigned int unit, const Texture* texture, const Texture* fallback)
{
    if (!texture)
    {
        texture = fallback;
        if (!texture)
            return;
    }

    // If the image load or texture upload for this texture are still pending then run them right now, this is where all JIT
    // texture loading occurs
    if (texture->getState() != Texture::Ready)
    {
        if (texture->getState() == Texture::ImageLoadPending)
            const_cast<Texture*>(texture)->ensureImageIsLoaded();
        if (texture->getState() == Texture::UploadPending)
            const_cast<Texture*>(texture)->upload();

        // If the texture still isn't ready then use the fallback if there is one
        if (texture->getState() != Texture::Ready)
        {
            if (fallback)
                setTexture(unit, fallback);

            return;
        }
    }

    States::Texture[unit] = texture->getActiveTextureObject();
}

}

#include "CarbonEngine/Graphics/OpenGLShared/OpenGLBase.h"

#include "CarbonEngine/Render/Shaders/AmbientOcclusionGLSL.h"
#include "CarbonEngine/Render/Shaders/Blending.h"
#include "CarbonEngine/Render/Shaders/DecalMappingGLSL.h"
#include "CarbonEngine/Render/Shaders/NormalMapping.h"
#include "CarbonEngine/Render/Shaders/ParallaxMappingGLSL.h"
#include "CarbonEngine/Render/Shaders/ShadowMappingGLSL.h"
#include "CarbonEngine/Render/Shaders/SkeletalAnimationGLSL.h"
#include "CarbonEngine/Render/Shaders/SpecularGLSL.h"
#include "CarbonEngine/Render/Shaders/VertexColor.h"

#include "CarbonEngine/Render/Shaders/BaseColored.h"
#include "CarbonEngine/Render/Shaders/BaseSkyDome.h"
#include "CarbonEngine/Render/Shaders/BaseSurface.h"
#include "CarbonEngine/Render/Shaders/InternalDeferredLightingDirectionalLight.h"
#include "CarbonEngine/Render/Shaders/InternalDeferredLightingPointLight.h"
#include "CarbonEngine/Render/Shaders/InternalDeferredLightingSetup.h"
#include "CarbonEngine/Render/Shaders/InternalDeferredLightingSurface.h"
#include "CarbonEngine/Render/Shaders/InternalFont.h"
#include "CarbonEngine/Render/Shaders/InternalGeometryClipmapping.h"
#include "CarbonEngine/Render/Shaders/InternalShadowMapping.h"
#include "CarbonEngine/Render/Shaders/InternalSprite.h"
#include "CarbonEngine/Render/Shaders/PostProcessAdd.h"
#include "CarbonEngine/Render/Shaders/PostProcessBloom.h"
#include "CarbonEngine/Render/Shaders/PostProcessBlur.h"
#include "CarbonEngine/Render/Shaders/PostProcessBrightPass.h"
#include "CarbonEngine/Render/Shaders/PostProcessColor.h"
#include "CarbonEngine/Render/Shaders/PostProcessDepthOfField.h"
#include "CarbonEngine/Render/Shaders/PostProcessPassThrough.h"
#include "CarbonEngine/Render/Shaders/PostProcessScattering.h"
#include "CarbonEngine/Render/Shaders/PostProcessToneMapping.h"
#include "CarbonEngine/Render/Shaders/SFXBlur.h"
#include "CarbonEngine/Render/Shaders/SFXEdge.h"
#include "CarbonEngine/Render/Shaders/SFXMirror.h"
#include "CarbonEngine/Render/Shaders/SFXWater.h"
