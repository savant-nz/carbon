/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Render/Effect.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/RenderEvents.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Shaders/ShaderRegistry.h"
#include "CarbonEngine/Render/Texture/Texture.h"

namespace Carbon
{

const UnicodeString Effect::EffectDirectory = "Effects/";
const UnicodeString Effect::EffectExtension = ".effect";

void Effect::clear()
{
    name_.clear();
    description_.clear();
    vertexStreams_.clear();
    parameters_.clear();

    quality_ = 0;
    clearActiveShader();
}

const Effect::EffectParameter& Effect::getParameter(const String& name) const
{
    static const auto empty = EffectParameter();

    return parameters_.detect([&](const EffectParameter& param) { return param.name == name; }, empty);
}

bool Effect::hasParameter(const String& name) const
{
    return parameters_.findBy([&](const EffectParameter& param) { return param.name == name; }) != -1;
}

bool Effect::isTextureParameter(const String& parameterName)
{
    return parameterName.endsWith("Map") || parameterName.endsWith("Texture");
}

bool Effect::load(const UnicodeString& filename)
{
    try
    {
        clear();

        name_ = A(FileSystem::getResourceName(filename, EffectDirectory, EffectExtension));

        // Open this effect file
        auto lineTokens = Vector<Vector<String>>();
        if (!fileSystem().readTextFile(filename, lineTokens))
            throw Exception("Failed reading effect file");

        // Read effect definition
        for (const auto& line : lineTokens)
        {
            if (line[0].asLower() == "description")
            {
                // Read "Description <...>"

                if (line.size() != 2)
                    throw Exception("Invalid description");

                description_ = line[1];
            }
            else if (line[0].asLower() == "vertexstream")
            {
                // Read "VertexStream <name>[<component count>]"

                if (line.size() != 2)
                    throw Exception("Invalid vertex stream");

                auto componentCount = line[1].getIndexInBrackets();
                if (componentCount < 1 || componentCount > 4)
                    throw Exception() << "Invalid vertex stream component count: " << componentCount;

                vertexStreams_.emplace(VertexStream::streamNameToType(line[1].withoutIndexInBrackets()), uint(componentCount),
                                       TypeNone);
            }
            else if (line[0].at(0) == '$')
            {
                // Read "$<name> <type> [<texture type> <texture group> [optional|internal]]"

                if (line[0].length() == 1 || line.size() < 2)
                    throw Exception("Invalid effect parameter");

                // Check for invalid characters
                if (line[0].has('.'))
                    throw Exception() << "Invalid effect parameter name: " << line[0].substr(1);

                auto effectParameter = Effect::EffectParameter();
                effectParameter.name = line[0].substr(1);

                if (line[1].asLower().startsWith("texture"))
                {
                    // Texture parameter

                    if (line.size() != 3 && line.size() != 4)
                        throw Exception("Invalid texture parameter");

                    if (!isTextureParameter(effectParameter.name))
                        throw Exception() << "Invalid texture parameter name: " << effectParameter.name;

                    effectParameter.textureType = Texture::convertStringToTextureType(line[1].substr(7));
                    if (effectParameter.textureType == GraphicsInterface::TextureNone)
                        throw Exception() << "Invalid texture type: " << line[1];

                    effectParameter.textureGroup = line[2];

                    // Check to see if this parameter is marked as optional or internal
                    if (line.size() == 4)
                    {
                        if (line[3].asLower() == "optional")
                            effectParameter.isOptional = true;
                        else
                            throw Exception() << "Unexpected token: " << line[3];
                    }
                }
                else
                {
                    // Standard parameter

                    if (line.size() > 3)
                        throw Exception("Invalid parameter definition");

                    if (isTextureParameter(effectParameter.name))
                        throw Exception() << "Normal parameters must use names reserved for texture parameters: "
                                          << effectParameter.name;

                    // Check type is valid
                    effectParameter.type = Parameter::getTypeFromString(line[1]);
                    if (effectParameter.type == Parameter::NullParameter)
                        throw Exception() << "Invalid parameter type: " << line[1];

                    // Check to see if this parameter is marked as optional or internal
                    if (line.size() == 3)
                    {
                        if (line[2].asLower() == "optional")
                            effectParameter.isOptional = true;
                        else
                            throw Exception() << "Unexpected token: " << line[2];
                    }
                }

                parameters_.append(effectParameter);
            }
            else
                throw Exception() << "Unexpected token: " << line[0];
        }

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name_ << "' - " << e;

        return false;
    }
}

Vector<Shader*> Effect::getAllShaders() const
{
    return ShaderRegistry::getShadersForEffect(name_);
}

void Effect::clearActiveShader()
{
    if (activeShader_)
    {
        activeShader_->cleanup();
        activeShader_ = nullptr;
    }
}

void Effect::updateActiveShader(unsigned int quality)
{
    auto newShader = pointer_to<Shader>::type();

    for (auto shader : getAllShaders())
    {
        // Skip shaders that don't have hardware support
        if (!shader->hasHardwareSupport())
            continue;

        // Find the actual shader to link with based on the quality level
        if (!newShader || ((shader->getQuality() > newShader->getQuality() && (shader->getQuality() <= quality)) ||
                           (shader->getQuality() < newShader->getQuality() && newShader->getQuality() > quality)))
            newShader = shader;
    }

    quality_ = quality;

    // If the shader hasn't changed then there's no need do anything
    if (newShader == activeShader_)
        return;

    // Clean up previous shader
    auto oldShader = activeShader_;
    auto oldShaderWasSetup = oldShader ? oldShader->isSetup() : false;
    if (oldShaderWasSetup)
        oldShader->cleanup();

    activeShader_ = newShader;

    // Setup new shader if previous one was setup
    if (oldShaderWasSetup && activeShader_)
        activeShader_->setup();

    // Send a ShaderChangeEvent
    events().dispatchEvent(ShaderChangeEvent(name_, oldShader, newShader));
}

bool Effect::isActiveShaderReady()
{
    return getActiveShader() && getActiveShader()->setup();
}

}
