/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/ParameterArray.h"

namespace Carbon
{

/**
 * Materials are the primary way that the appearance of a rendered object is controlled. Materials consist of an effect
 * and a set of parameters for that effect to use, where the parameters are things such as textures and colors. Most
 * materials are loaded from ".material" files, but they can also be created at runtime. Materials can store any number
 * of parameters, which means applications can store their own arbitrary data on their materials. Note that some effects
 * may have required parameters that can't be omitted.
 *
 * Materials are managed by the MaterialManager class.
 */
class CARBON_API Material : private Noncopyable
{
public:

    /**
     * The directory which materials are stored under, currently "Materials/".
     */
    static const UnicodeString MaterialDirectory;

    /**
     * The file extension for materials, currently ".material".
     */
    static const UnicodeString MaterialExtension;

    Material() { clear(); }

    /**
     * Constructs this material with the specified name.
     */
    Material(String name);

    /**
     * Copy constructor (not implemented).
     */
    Material(const Material& other);

    ~Material() { clear(); }

    /**
     * Stores details of texture animations running on this material.
     */
    class AnimatedTexture
    {
    public:

        /**
         * The name of the texture parameter to animate the texture of.
         */
        const String& getName() const { return name_; }

        /**
         * The frames-per-second to run the animation at.
         */
        unsigned int getFPS() const { return fps_; }

        /**
         * For internal use only, this holds the pointer to the animated texture.
         */
        Texture* getTexture() const { return texture_; }

        /**
         * For internal use only, this holds the current animation frame.
         */
        unsigned int getCurrentFrame() const { return currentFrame_; }

        AnimatedTexture() {}

        /**
         * Constructs this animated texture from the given name and FPS.
         */
        AnimatedTexture(String name, unsigned int fps) : name_(std::move(name)), fps_(fps) {}

    private:

        String name_;
        unsigned int fps_ = 0;
        Texture* texture_ = nullptr;
        unsigned int currentFrame_ = 0;

        friend class Material;
    };

    /**
     * Returns this material's name.
     */
    const String& getName() const { return name_; }

    /**
     * Returns this material's description.
     */
    const String& getDescription() const { return description_; }

    /**
     * Sets this material's description.
     */
    void setDescription(const String& description) { description_ = description; }

    /**
     * Returns this material's effect.
     */
    const String& getEffectName() const { return effectName_; }

    /**
     * Sets this material's effect. Returns success flag.
     */
    bool setEffect(const String& effectName);

    /**
     * Returns this material's effect, or null if it is absent or invalid.
     */
    Effect* getEffect() const { return effect_; }

    /**
     * Returns a parameter stored on this material. If there is no parameter for the given lookup on this material then
     * Parameter::Empty is returned.
     */
    const Parameter& getParameter(const ParameterArray::Lookup& lookup) const { return parameters_[lookup]; }

    /**
     * Returns a parameter stored on this material. If there is no parameter with the given name on this material then
     * Parameter::Empty is returned.
     */
    const Parameter& getParameter(const String& name) const { return parameters_[ParameterArray::Lookup(name)]; }

    /**
     * Sets the value of a boolean parameter on this material.
     */
    template <typename LookupType> void setParameter(const LookupType& lookup, bool value)
    {
        parameters_[lookup].setBoolean(value);
    }

    /**
     * Sets the value of an integer parameter on this material.
     */
    template <typename LookupType> void setParameter(const LookupType& lookup, int value)
    {
        parameters_[lookup].setInteger(value);
    }

    /**
     * Sets the value of a float parameter on this material.
     */
    template <typename LookupType> void setParameter(const LookupType& lookup, float value)
    {
        parameters_[lookup].setFloat(value);
    }

    /**
     * Sets the value of a Vec2 parameter on this material.
     */
    template <typename LookupType> void setParameter(const LookupType& lookup, const Vec2& v)
    {
        parameters_[lookup].setVec2(v);
    }

    /**
     * Sets the value of a Vec3 parameter on this material.
     */
    template <typename LookupType> void setParameter(const LookupType& lookup, const Vec3& v)
    {
        parameters_[lookup].setVec3(v);
    }

    /**
     * Sets the value of a Quaternion parameter on this material.
     */
    template <typename LookupType> void setParameter(const LookupType& lookup, const Quaternion& q)
    {
        parameters_[lookup].setQuaternion(q);
    }

    /**
     * Sets the value of a Color parameter on this material.
     */
    template <typename LookupType> void setParameter(const LookupType& lookup, const Color& color)
    {
        parameters_[lookup].setColor(color);
    }

    /**
     * Sets the value of a Float4 parameter on this material.
     */
    template <typename LookupType> void setParameter(const LookupType& lookup, float f0, float f1, float f2, float f3)
    {
        parameters_[lookup].setFloat4(f0, f1, f2, f3);
    }

    /**
     * Sets the value of a string parameter on this material.
     */
    template <typename LookupType> void setParameter(const LookupType& lookup, const char* value)
    {
        setParameter(lookup, Parameter(value));
    }

    /**
     * Sets the value of a parameter on this material. This can be used to change or add parameters to a material at
     * runtime, the change takes effect immediately. Returns success flag.
     */
    bool setParameter(const ParameterArray::Lookup& lookup, const Parameter& parameter);

    /**
     * Sets the value of a parameter on this material. This can be used to change or add parameters to a material at
     * runtime, the change takes effect immediately. Returns success flag.
     */
    bool setParameter(const String& name, const Parameter& parameter)
    {
        return setParameter(ParameterArray::Lookup(name), parameter);
    }

    /**
     * Returns the parameters on this material.
     */
    const ParameterArray& getParameters() const { return parameters_; }

    /**
     * Returns whether the specified parameter is set on this material.
     */
    template <typename LookupType> bool hasParameter(const LookupType& lookup) const { return parameters_.has(lookup); }

    /**
     * Returns a pointer to the texture currently in use for the given texture parameter on this material. The returned
     * texture will have its image data loaded, and so this method may cause a JIT texture load to occur on the main
     * thread. If the parameter is not a valid texture parameter as specified by this material's effect then null is
     * returned.
     */
    Texture* getTextureForParameter(const ParameterArray::Lookup& lookup);

    /**
     * \copydoc getTextureForParameter(const ParameterArray::Lookup &)
     */
    Texture* getTextureForParameter(const String& name) { return getTextureForParameter(ParameterArray::Lookup(name)); }

    /**
     * Returns details of the texture animations on this material.
     */
    const Vector<AnimatedTexture>& getAnimatedTextures() const { return animatedTextures_; }

    /**
     * Sets the frame rate of the given texture used by this material. The \a name parameter must be the name of the
     * texture parameter and not the name of a texture. This will only affect textures that have multiple animation
     * frames, standard textures are unaffected. Returns success flag.
     */
    bool setAnimatedTextureFPS(const String& name, unsigned int fps);

    /**
     * Sets up the specified EffectQueue for rendering the contents of this material, this sets any animated texture
     * updates needed for this material using EffectQueue::addTextureAnimation() and calls EffectQueue::useParams() with
     * this material's main ParameterArray.
     */
    void setupEffectQueue(EffectQueue* queue) const;

    /**
     * Returns whether this material contains a valid loaded material definition.
     */
    bool isLoaded() const { return isLoaded_; }

    /**
     * Returns whether this material was loaded from a file.
     */
    bool isLoadedFromFile() const { return isLoadedFromFile_; }

    /**
     * Clears the contents of this material and releases all texture references.
     */
    void clear();

    /**
     * Saves this material to a file. Returns success flag.
     */
    bool save(const UnicodeString& name = UnicodeString::Empty) const;

    /**
     * Loads this material from the given material file. Returns success flag.
     */
    bool load(const UnicodeString& name);

    /**
     * Returns whether this material's textures are loaded.
     */
    bool areTexturesLoaded() const { return areTexturesLoaded_; }

    /**
     * Updates this material's texture animations and ensures its textures are loaded.
     */
    void update();

    /**
     * Ensures this material's textures and effect are precached for rendering.
     */
    void precache();

    /**
     * Samples the given 2D texture on this material. Returns success flag.
     */
    bool sampleTexture(const String& parameterName, float u, float v, Color& result);

private:

    friend class MaterialManager;

    String name_;
    String description_;

    String effectName_;
    Effect* effect_ = nullptr;

    ParameterArray parameters_;
    void verifyRequiredEffectParameters();

    // Texture references for this material
    bool areTexturesLoaded_ = false;
    Vector<const Texture*> textureReferences_;
    void loadTextures();
    void unloadTextures();

    // Animated textures in this material
    Vector<AnimatedTexture> animatedTextures_;

    bool isLoaded_ = false;
    bool isLoadedFromFile_ = false;
};

}
