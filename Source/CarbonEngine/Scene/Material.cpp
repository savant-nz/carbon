/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Render/EffectQueue.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"

namespace Carbon
{

const UnicodeString Material::MaterialDirectory = "Materials/";
const UnicodeString Material::MaterialExtension = ".material";

Material::Material(String name)
{
    clear();
    name_ = std::move(name);
}

void Material::clear()
{
    name_.clear();
    description_.clear();
    effect_ = nullptr;
    effectName_.clear();
    parameters_.clear();
    isLoaded_ = false;
    isLoadedFromFile_ = false;
    unloadTextures();
}

bool Material::setEffect(const String& effectName)
{
    auto newEffect = effects().getEffect(effectName);
    if (!newEffect)
    {
        LOG_ERROR << "Unknown effect: " << effectName;
        return false;
    }

    // When updating effects we need to make sure the texture references held by this material are updated correctly

    auto wereTexturesLoaded = areTexturesLoaded_;

    if (wereTexturesLoaded)
    {
        textures().disableTextureDeletion();
        unloadTextures();
    }

    effectName_ = effectName;
    effect_ = newEffect;

    if (wereTexturesLoaded)
    {
        loadTextures();
        textures().enableTextureDeletion();
    }

    isLoaded_ = true;

    // The $diffuseColor parameter always defaults to white
    if (effect_->hasParameter("diffuseColor") && !hasParameter(Parameter::diffuseColor))
        setParameter(Parameter::diffuseColor, Color::White);

    return true;
}

bool Material::setParameter(const ParameterArray::Lookup& lookup, const Parameter& parameter)
{
    if (!effect_)
        return false;

    auto isTextureParameter = Effect::isTextureParameter(lookup.getName());

    if (isTextureParameter && !effect_->getParameter(lookup.getName()).isTexture())
    {
        LOG_ERROR << "Texture parameter is not in this material's effect";
        return false;
    }

    // When updating texture parameters we need to make sure the texture references held by this material are updated
    // correctly

    auto wereTexturesLoaded = isTextureParameter && areTexturesLoaded_;

    if (wereTexturesLoaded)
    {
        textures().disableTextureDeletion();
        unloadTextures();
    }

    auto isRemoveOperation =
        (&parameter == &Parameter::Empty) || (isTextureParameter && !parameter.getString().length());

    // Set the new parameter value
    if (isRemoveOperation)
    {
        parameters_.remove(lookup);
        if (isTextureParameter)
            parameters_.remove(Parameter::getHiddenParameterName(lookup.getName()));
    }
    else
        parameters_[lookup] = parameter;

    if (wereTexturesLoaded)
    {
        loadTextures();
        textures().enableTextureDeletion();
    }

    return true;
}

Texture* Material::getTextureForParameter(const ParameterArray::Lookup& lookup)
{
    if (!effect_ || !effect_->getParameter(lookup.getName()).isTexture())
        return nullptr;

    loadTextures();

    auto texture = textures().getTexture(parameters_[lookup].getString());
    texture->ensureImageIsLoaded();

    return texture;
}

bool Material::setAnimatedTextureFPS(const String& name, unsigned int fps)
{
    if (!effect_)
        return false;

    // Look for existing animated textures to directly update the FPS on
    for (auto& animatedTexture : animatedTextures_)
    {
        if (animatedTexture.getName() == name)
        {
            animatedTexture.fps_ = fps;
            return true;
        }
    }

    // Search for a texture parameter for this material that isn't currently animated. If one is found then add it to
    // the list of textures to animate on this material
    for (auto& effectParameter : effect_->getParameters())
    {
        if (effectParameter.isTexture() && effectParameter.name == name)
        {
            // Create a new animated texture description
            animatedTextures_.emplace(name, fps);

            // Cache the texture pointer if it is available
            auto search = Parameter::getHiddenParameterName(name);
            if (parameters_.has(search))
                animatedTextures_.back().texture_ = parameters_[search].getPointer<Texture>();

            return true;
        }
    }

    LOG_ERROR << "Unknown texture parameter: " << name;

    return false;
}

void Material::setupEffectQueue(EffectQueue* queue) const
{
    queue->useParams(parameters_);

    for (auto& animatedTexture : animatedTextures_)
    {
        if (animatedTexture.getTexture())
            queue->addTextureAnimation(animatedTexture.getTexture(), animatedTexture.getCurrentFrame());
    }
}

bool Material::save(const UnicodeString& name) const
{
    try
    {
        auto filename = name.length() ? name : name_;

        // Check this material has both a name and a valid effect
        if (!filename.length() || !effect_)
            throw Exception("Can't save material without a name and an effect");

        filename = FileSystem::getResourceFilename(filename, MaterialDirectory, MaterialExtension);

        // Open material file
        auto file = FileWriter();
        fileSystem().open(filename, file);

        // Alignment spacing
        auto alignment = 32U;

        // Write description
        if (description_.length())
            file.writeText(String("Description ").padToLength(alignment) + description_.quoteIfHasSpaces(), 2);

        // Write effect
        if (effect_->getName() != "BaseSurface")
            file.writeText(String("Effect ").padToLength(alignment) + effect_->getName().quoteIfHasSpaces(), 2);

        // Write animated texture frame rates
        for (auto& animatedTexture : animatedTextures_)
        {
            file.writeText(String("AnimationFPS ").padToLength(alignment) + "$" + animatedTexture.getName() + " " +
                           animatedTexture.getFPS());
        }

        // Sort parameters so that texture parameters appear first
        auto parameterNames = parameters_.getParameterNames().sorted();
        auto textureParameterNames = Vector<String>();

        for (auto i = 0U; i < parameterNames.size(); i++)
        {
            if (Effect::isTextureParameter(parameterNames[i]))
            {
                textureParameterNames.append(parameterNames[i]);
                parameterNames.erase(i--);
            }

            // Remove hidden parameters
            else if (parameterNames[i].at(0) == '.')
                parameterNames.erase(i--);

            // Don't write out white diffuse colors as this is the default
            else if (parameterNames[i] == "diffuseColor" &&
                     parameters_.get(parameterNames[i]).getColor() == Color::White &&
                     effect_->getName() != "BaseColored")
                parameterNames.erase(i--);
        }

        // Construct a sorted parameter list with texture parameters first followed by all other parameters
        auto sortedParameterNames = textureParameterNames;
        if (sortedParameterNames.size() && parameterNames.size())
            sortedParameterNames.append(String::Empty);
        sortedParameterNames.append(parameterNames);

        // Write out parameters
        for (const auto& parameterName : sortedParameterNames)
        {
            if (!parameterName.length())
            {
                file.writeText("");
                continue;
            }

            // Write parameter name
            file.writeText(("$" + parameterName).padToLength(alignment), 0);

            auto& parameterValue = parameters_.get(parameterName);

            // See if this parameter is for the effect
            auto& ep = effect_->getParameter(parameterName);
            if (ep.name.length())
            {
                if (ep.isTexture())
                {
                    // Write texture parameters as quoted strings
                    file.writeText(parameterValue.getString().quoteIfHasSpaces());
                }
                else
                {
                    // Write this parameter in the correct format for its type
                    switch (ep.type)
                    {
                        case Parameter::BooleanParameter:
                            file.writeText(parameterValue.getBoolean());
                            break;
                        case Parameter::IntegerParameter:
                            file.writeText(parameterValue.getInteger());
                            break;
                        case Parameter::FloatParameter:
                            file.writeText(parameterValue.getFloat());
                            break;
                        case Parameter::Vec2Parameter:
                            file.writeText(parameterValue.getVec2());
                            break;
                        case Parameter::Vec3Parameter:
                            file.writeText(parameterValue.getVec3());
                            break;
                        case Parameter::QuaternionParameter:
                            file.writeText(parameterValue.getQuaternion());
                            break;
                        case Parameter::ColorParameter:
                            if (parameterValue.getColor().a != 1.0f)
                                file.writeText(parameterValue.getColor());
                            else
                                file.writeText(parameterValue.getVec3());
                            break;
                        case Parameter::StringParameter:
                            file.writeText(parameterValue.getString().quoteIfHasSpaces());
                            break;
                        default:
                            file.writeText(parameterValue.getString());
                            break;
                    }
                }
            }
            else
            {
                // Non-effect parameters are written as is
                file.writeText(parameterValue.getString());
            }
        }

        file.close();

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << name << " - " << e;

        return false;
    }
}

bool Material::load(const UnicodeString& name)
{
    try
    {
        clear();

        name_ = A(FileSystem::getResourceName(name, MaterialDirectory, MaterialExtension));

        // Read this material file's contents
        auto lineTokens = Vector<Vector<String>>();
        if (!fileSystem().readTextFile(FileSystem::getResourceFilename(name, MaterialDirectory, MaterialExtension),
                                       lineTokens))
            throw Exception("Failed opening file");

        // Read material definition
        auto effectSpecified = false;
        for (const auto& line : lineTokens)
        {
            if (line[0] == "Description")
            {
                // Read "Description <...>"

                if (line.size() != 2)
                    throw Exception("Invalid description");

                description_ = line[1];
            }
            else if (line[0] == "Effect")
            {
                // Read "Effect <name>"

                if (line.size() != 2)
                    throw Exception("Invalid effect");

                if (effectSpecified)
                    throw Exception("Effect already specified");

                auto& effectName = line[1];

                if (!effects().getEffect(effectName) || !setEffect(effectName))
                    throw Exception() << "Unknown effect: " << effectName;

                effectSpecified = true;
            }
            else if (line[0].at(0) == '$')
            {
                // Read "$<name> <value>"

                // Default to the BaseSurface effect
                if (!effectSpecified)
                {
                    if (!setEffect("BaseSurface"))
                        throw Exception("Failed setting default BaseSurface effect");

                    effectSpecified = true;
                }

                if (line[0].length() == 1)
                    throw Exception("No parameter name");

                if (line.size() == 1)
                    throw Exception() << "No parameter value for " << line[0];

                auto parameterName = line[0].substr(1);

                // Check parameter is valid
                auto& ep = effect_->getParameter(parameterName);
                if (ep.name.length())
                {
                    if (!ep.isTexture())
                    {
                        auto expectedTokenCount = 0U;
                        if (ep.type == Parameter::BooleanParameter)
                        {
                            expectedTokenCount = 2;
                            if (line.size() < 2 || !line[1].isBoolean())
                                throw Exception() << "Invalid boolean parameter: " << ep.name;
                        }
                        else if (ep.type == Parameter::IntegerParameter)
                        {
                            expectedTokenCount = 2;
                            if (line.size() < 2 || !line[1].isInteger())
                                throw Exception() << "Invalid integer parameter: " << ep.name;
                        }
                        else if (ep.type == Parameter::FloatParameter)
                        {
                            expectedTokenCount = 2;
                            if (line.size() < 2 || !line[1].isFloat())
                                throw Exception() << "Invalid float parameter: " << ep.name;
                        }
                        else if (ep.type == Parameter::Vec2Parameter)
                        {
                            expectedTokenCount = 3;
                            if (line.size() < 3 || !line[1].isFloat() || !line[2].isFloat())
                                throw Exception() << "Invalid Vec2 parameter: " << ep.name;
                        }
                        else if (ep.type == Parameter::Vec3Parameter)
                        {
                            expectedTokenCount = 4;
                            if (line.size() < 4 || !line[1].isFloat() || !line[2].isFloat() || !line[3].isFloat())
                                throw Exception() << "Invalid Vec3 parameter: " << ep.name;
                        }
                        else if (ep.type == Parameter::ColorParameter)
                        {
                            if (line.size() < 4 || !line[1].isFloat() || !line[2].isFloat() || !line[3].isFloat())
                                throw Exception() << "Invalid color parameter: " << ep.name;

                            if (line.size() == 4)
                                expectedTokenCount = 4;
                            else
                            {
                                expectedTokenCount = 5;
                                if (!line[4].isFloat())
                                    throw Exception() << "Invalid color parameter: " << ep.name;
                            }
                        }
                        else if (ep.type == Parameter::Float4Parameter)
                        {
                            expectedTokenCount = 5;
                            if (line.size() < 5 || !line[1].isFloat() || !line[2].isFloat() || !line[3].isFloat() ||
                                !line[4].isFloat())
                                throw Exception() << "Invalid Float4 parameter: " << ep.name;
                        }
                        else if (ep.type == Parameter::StringParameter)
                        {
                            expectedTokenCount = 2;
                            if (line.size() < 2)
                                throw Exception() << "Invalid string parameter: " << ep.name;
                        }

                        // Warn about any ignored tokens
                        if (line.size() > expectedTokenCount)
                            LOG_WARNING << "'" << name << "' - extra tokens ignored for parameter: " << line[0];
                    }
                }
                else
                {
                    if (Effect::isTextureParameter(parameterName))
                        LOG_WARNING << "'" << name
                                    << "' - Texture parameter is not in this material's effect: " << parameterName;
                }

                // Concatenate remaining tokens to get the value for this parameter
                parameters_.set(parameterName, String(line, " ", 1));
            }
            else if (line[0] == "AnimationFPS")
            {
                // Read "AnimationFPS $<texture name> <fps>"

                if (line.size() != 3)
                    throw Exception("Invalid AnimationFPS");

                // Check FPS value is valid
                if (!line[2].isInteger() || line[2].asInteger() < 0)
                    throw Exception() << "Invalid AnimationFPS: " << line[2];

                // Store animation details
                animatedTextures_.append(Material::AnimatedTexture());
                animatedTextures_.back().name_ = line[1].substr(1);
                animatedTextures_.back().fps_ = line[2].asInteger();
            }
            else
                throw Exception() << "Unexpected token: " << line[0];
        }

        // Check all required effect parameters were specified
        verifyRequiredEffectParameters();

        isLoaded_ = true;
        isLoadedFromFile_ = true;

        LOG_INFO << "Material loaded - '" << name_ << "'";

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name_ << "' - " << e;

        auto originalName = name_;
        clear();
        name_ = originalName;

        isLoadedFromFile_ = true;

        return false;
    }
}

void Material::loadTextures()
{
    if (areTexturesLoaded_ || !effect_)
        return;

    auto shader = effect_->getActiveShader();
    if (!shader)
        return;

    shader->prepareParameters(parameters_, textureReferences_);

    // Update all the texture pointers in the animatedTextures_ array
    for (auto& animatedTexture : animatedTextures_)
    {
        animatedTexture.texture_ = nullptr;

        auto search = Parameter::getHiddenParameterName(animatedTexture.getName());
        if (parameters_.has(search))
            animatedTexture.texture_ = parameters_[search].getPointer<Texture>();
    }

    areTexturesLoaded_ = true;
}

void Material::unloadTextures()
{
    for (auto texture : textureReferences_)
        textures().releaseTexture(texture);

    textureReferences_.clear();

    // Remove all hidden texture parameters because the texture references they imply are now defunct
    for (auto parameter : parameters_)
    {
        if (Parameter::isHiddenParameterName(parameter.getName()))
            parameters_.remove(parameter.getName());
    }

    areTexturesLoaded_ = false;
}

void Material::update()
{
    // Update texture animations
    for (auto& animatedTexture : animatedTextures_)
    {
        if (animatedTexture.getFPS() == 0)
            animatedTexture.currentFrame_ = 0;
        else
            animatedTexture.currentFrame_ = platform().getTime() / TimeValue(1.0f / animatedTexture.getFPS());
    }

    // Ensure this material's textures are loaded
    loadTextures();
}

void Material::precache()
{
    if (!effect_)
        return;

    loadTextures();

    auto shader = effect_->getActiveShader();
    if (shader)
        shader->setup();
}

bool Material::sampleTexture(const String& parameterName, float u, float v, Color& result)
{
    if (!effect_ || !parameters_.has(parameterName))
        return false;

    loadTextures();

    auto texture = parameters_[Parameter::getHiddenParameterName(parameterName)].getPointer<Texture>();
    if (!texture || texture->getTextureType() != GraphicsInterface::Texture2D)
        return false;

    auto frame = 0U;
    for (auto& animatedTexture : animatedTextures_)
    {
        if (animatedTexture.getName() == parameterName)
        {
            frame = animatedTexture.getCurrentFrame();
            break;
        }
    }

    result = static_cast<Texture2D*>(texture)->sampleNearestTexel(u, v, frame);

    return true;
}

void Material::verifyRequiredEffectParameters()
{
    if (!effect_)
        return;

    for (auto& effectParameter : effect_->getParameters())
    {
        if (!parameters_.has(effectParameter.name) && !effectParameter.isOptional)
            throw Exception() << "Missing effect parameter: " << effectParameter.name;
    }
}

}
