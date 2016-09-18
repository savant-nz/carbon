/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Parameter.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Render/VertexStream.h"

namespace Carbon
{

/**
 * This class holds the name, description and parameter information on a single effect. An effect is defined in a text
 * file and describes at a high level the manner in which an object using this effect will be drawn. Shaders then
 * provide as many implementations of the effect as are required. The decision about which shader to use for each effect
 * at runtime is done in Effect::updateActiveShader().
 */
class CARBON_API Effect : private Noncopyable
{
public:

    /**
     * The directory for effects, currently "Effects/".
     */
    static const UnicodeString EffectDirectory;

    /**
     * The extension for effects, currently ".effect".
     */
    static const UnicodeString EffectExtension;

    /**
     * Holds details on an effect parameter.
     */
    class EffectParameter
    {
    public:

        /**
         * The name of this effect parameter.
         */
        String name;

        /**
         * Whether this effect parameter is optional.
         */
        bool isOptional = false;

        /**
         * For texture parameters, the type of texture this effect parameter describes.
         */
        GraphicsInterface::TextureType textureType = GraphicsInterface::TextureNone;

        /**
         * For texture parameters, the texture group for the texture described by this effect parameter.
         */
        String textureGroup;

        /**
         * For non-texture parameters, the parameter type of this effect.
         */
        Parameter::Type type = Parameter::NullParameter;

        /**
         * Whether this effect parameter describes a texture
         */
        bool isTexture() const { return Effect::isTextureParameter(name); }
    };

    ~Effect() { clearActiveShader(); }

    /**
     * Clears the contents of this effect.
     */
    void clear();

    /**
     * Returns the name of this effect.
     */
    const String& getName() const { return name_; }

    /**
     * Returns the description of this effect.
     */
    const String& getDescription() const { return description_; }

    /**
     * Returns the vertex streams this effect requires.
     */
    const Vector<VertexStream>& getVertexStreams() const { return vertexStreams_; }

    /**
     * Returns the effect parameters this effect requires. The parameter values here aren't used.
     */
    const Vector<EffectParameter>& getParameters() const { return parameters_; }

    /**
     * Returns the parameter with the specified name. If no parameter with the given name exists on this effect then an
     * empty effect parameter definition is returned.
     */
    const EffectParameter& getParameter(const String& name) const;

    /**
     * Returns whether this effect has a parameter with the specified name.
     */
    bool hasParameter(const String& name) const;

    /**
     * Returns whether the given parameter is a texture parameter. Texture parameters must end with either 'Map' or
     * 'Texture'.
     */
    static bool isTextureParameter(const String& parameterName);

    /**
     * Loads this effect from the given effect file. Returns success flag.
     */
    bool load(const UnicodeString& filename);

    /**
     * Returns all the available shader implementations for this effect that are compatible with the active graphics
     * interface.
     */
    Vector<Shader*> getAllShaders() const;

    /**
     * Returns the shader implementation currently being used to render this effect. If there is no available shader
     * implementation then null is returned.
     */
    Shader* getActiveShader() const { return activeShader_; }

    /**
     * Clears the currently active shader, this will also cause the active shader to release any graphics interface
     * resources it is holding.
     */
    void clearActiveShader();

    /**
     * Updates the active shader being used to render this effect. This will obey the given shader quality setting when
     * choosing the active shader. Shaders with a quality level above the specified value will not be used unless there
     * are no alternative shaders at the lower quality setting.
     */
    void updateActiveShader(unsigned int quality);

    /**
     * Returns true if this effect has a valid shader that is setup and ready for rendering. If the active shader hasn't
     * been setup yet then this method will attempt to do so.
     */
    bool isActiveShaderReady();

    /**
     * Returns the quality setting that was used to determine the currently active shader.
     */
    unsigned int getQuality() const { return quality_; }

    /**
     * Low shader quality value. See Effect::updateActiveShader() for details.
     */
    static const auto LowShaderQuality = 10U;

    /**
     * Medium shader quality value. See Effect::updateActiveShader() for details.
     */
    static const auto MediumShaderQuality = 50U;

    /**
     * High shader quality value. See Effect::updateActiveShader() for details.
     */
    static const auto HighShaderQuality = 100U;

    /**
     * Maximum shader quality value. See Effect::updateActiveShader() for details.
     */
    static const auto MaximumShaderQuality = UINT_MAX;

private:

    String name_;
    String description_;

    Vector<VertexStream> vertexStreams_;
    Vector<EffectParameter> parameters_;

    unsigned int quality_ = 0;

    Shader* activeShader_ = nullptr;
};

}
