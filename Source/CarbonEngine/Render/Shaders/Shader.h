/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Parameter.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Graphics/ShaderProgram.h"
#include "CarbonEngine/Render/Shaders/Blending.h"
#include "CarbonEngine/Render/VertexStream.h"

namespace Carbon
{

/**
 * A shader provides an implementation of an effect.
 */
class CARBON_API Shader : private Noncopyable
{
public:

    /**
     * The directory for shaders, currently "Shaders/".
     */
    static const UnicodeString ShaderDirectory;

    /**
     * Every shader has a shader type that is returned by the Shader::getShaderType() method. The shader type is used in the
     * renderer to sort and process geometry as well as ensure that the shader is supplied with the resources it needs to render
     * correctly.
     */
    enum ShaderType
    {
        /**
         * A standard geometry shader.
         */
        Normal,

        /**
         * A shader that uses blending that means it should be drawn after objects behind it, i.e. back to front.
         */
        Blended,

        /**
         * A shader that requires a reflection texture as an input.
         */
        Reflection,

        /**
         * A shader that requires a framebuffer texture as an input.
         */
        Framebuffer,

        /**
         * A shader that requires both refraction and reflection framebuffer textures as inputs. Mainly used for water effects.
         */
        RefractionReflection,

        /**
         * A post-processing shader that does shading effects on the final render of a scene.
         */
        PostProcess
    };

    /**
     * Constructs this shader with the given effect name, quality level, and an optional required shader language. The default
     * implementation of Shader::hasHardwareSupport() checks that the specified shader language has hardware support which means
     * that shader subclasses that rely on a single shader language do not need to override Shader::hasHardwareSupport().
     */
    Shader(String effectName, unsigned int quality,
           ShaderProgram::ShaderLanguage requiredShaderLanguage = ShaderProgram::NoShaderLanguage);

    virtual ~Shader();

    /**
     * Returns the name of this shader subclass. This is set by the CARBON_REGISTER_SHADER() macro and the
     * ShaderRegistry::registerShader() method.
     */
    const String& getClassName() const { return className_; }

    /**
     * The effect this shader implements, this is set in the constructor.
     */
    const String& getEffectName() const { return effectName_; }

    /**
     * Returns the quality level of this shader's implementation of the effect. A value of 100 or greater means no parts of the
     * effect are missing. This value is set in the constructor.
     */
    unsigned int getQuality() const { return quality_; }

    /**
     * Whether this shader can run on the current hardware setup. The default implementation returns whether the shader language
     * passed to the constructor is supported. Shaders that have more specific hardware requirements should override this.
     */
    virtual bool hasHardwareSupport() const
    {
        return requiredShaderLanguage_ == ShaderProgram::NoShaderLanguage ||
            graphics().isShaderLanguageSupported(requiredShaderLanguage_);
    }

    /**
     * Returns the shader type when the shader is using the specified parameters. Shaders can return different types given a
     * different set of input parameters. That default implementation returns \a Normal except when the "blend" parameter is set
     * to true in which case \a Blended is returned.
     */
    virtual ShaderType getShaderType(const ParameterArray& params, const ParameterArray& internalParams) const
    {
        return Shaders::Blending::isPresent(params) ? Blended : Normal;
    }

    /**
     * Initializes this shader for rendering if it has not yet been initialized. Returns a flag indicating whether the shader
     * has initialized successfully and can be used in rendering.
     */
    bool setup();

    /**
     * Returns whether this shader is initialized and ready for use in rendering.
     */
    bool isSetup() const { return isSetup_; }

    /**
     * Cleans up any resources allocated by this shader. After calling this Shader::isSetup() will return false.
     */
    void cleanup();

    /**
     * Causes this shader to do any precaching it can to avoid JIT processes occurring during rendering, typically this is used
     * to compile all possible shader feature combinations.
     */
    virtual void precache() {}

    /**
     * Called when this shader needs to activate itself for rendering.
     */
    virtual void enterShader() = 0;

    /**
     * Returns the number of rendering passes needed by this shader when rendering with the given \a params and \a
     * internalParams, the default implementation returns 1 indicating that this is a single pass shader, and shader
     * implementations that require multiple passes can override this as needed. Shader::setShaderParams() will be called once
     * for every rendering pass.
     */
    virtual unsigned int getPassCount(const ParameterArray& params, const ParameterArray& internalParams) const { return 1; }

    /**
     * Called for each geometry chunk prior to it being rendered, this is called once for every rendering pass. By default there
     * is only one rendering pass done but multipass shaders can increase this number by overriding Shader::getPassCount(). The
     * \a pass parameter indicates which pass is currently being rendered. The \a sortKey parameter will be set to the sort key
     * that was returned for this set of parameters when it was passed to Shader::getSortKey().
     */
    virtual void setShaderParams(const GeometryChunk& geometryChunk, const ParameterArray& params,
                                 const ParameterArray& internalParams, unsigned int pass, unsigned int sortKey) = 0;

    /**
     * Does any cleanup and state resetting for enterShader.
     */
    virtual void exitShader() = 0;

    /**
     * Returns the sorting key for this shader given the specified parameters. This is used to sort rendering order to avoid
     * excessive state changes. The sort key is passed back to the shader in calls to Shader::setShaderParams() so it can use
     * the sort key to store information that is then used in rendering.
     */
    virtual unsigned int getSortKey(const ParameterArray& params, const ParameterArray& internalParams) const { return 0; }

    /**
     * The standard behavior for multi-pass post-process shaders is for them to output to the relevant output texture during
     * their final pass, with other previous passes being done to other offscreen buffers. However, if a multi-pass post-process
     * shader wants to start writing to its output texture in an earlier pass then it can return true from this method and all
     * remaining rendering passes from it will be directed into the relevant output texture.
     */
    virtual bool isPostProcessShaderReadyToOutput(unsigned int pass) const { return false; }

    /**
     * This takes a set of effect parameters, and for each parameter that has a name that is a texture parameter for this
     * shader's effect, a new hidden parameter is added that contains a pointer to the loaded texture. This pointer is then used
     * in the shader to get the correct texture instance to use. Any new texture references taken are appended to the \a
     * textureReferences vector. The caller is responsible for managing and releasing these references properly. This method may
     * also create new hidden parameters in the given parameter array which means that the parameter array passed to
     * Shader::setShaderParams() must have first been passed through this method.
     */
    bool prepareParameters(ParameterArray& parameters, Vector<const Texture*>& textureReferences) const;

    /**
     * The ManagedShaderProgram class is provided for shader subclasses to use in order to make it easier to use a ShaderProgram
     * instance that has been provided by the graphics interface. This class must be subclassed in order to be used. It handles
     * setup, cleanup and has a ManagedShaderProgram::cache() method that should be implemented by subclasses to cache the
     * necessary ShaderConstant instances that will be used during rendering.
     */
    class ManagedShaderProgram : private Noncopyable
    {
    public:

        virtual ~ManagedShaderProgram() { clear(); }

        /**
         * Sets up a shader program from the set of source files specified.
         */
        bool setup(ShaderProgram::ShaderLanguage language, const Vector<String>& sourceFiles,
                   const String& preprocessorDefines = String::Empty);

        /**
         * Returns the ShaderProgram instance being managed by this class.
         */
        ShaderProgram* getProgram() { return program_; }

        /**
         * Erases the program object.
         */
        void clear();

        /**
         * Activates a managed shader program ready for rendering. This method is intended to be used inside
         * Shader::enterShader() or Shader::setShaderParams() implementations.
         */
        void activate();

        /**
         * Maps available vertex attributes so that they can be later be activated for rendering using
         * ManagedShaderProgram::setVertexAttributeArrayConfiguration(). The "vs" prefix on vertex stream names is removed
         * automatically. This method is automatically called as part of ManagedShaderProgram::setup().
         */
        bool mapVertexAttributes();

        /**
         * Enables all the known vertex attributes in this shader program.
         */
        void setVertexAttributeArrayConfiguration(const GeometryChunk& geometryChunk);

        /**
         * \copydoc ShaderProgram::getConstant(const String &, const String &)
         */
        ShaderConstant* getConstant(const String& name, const String& parameterName)
        {
            return program_->getConstant(name, parameterName);
        }

    protected:

        /**
         * Should be implemented by subclasses to cache any ShaderConstant instances that will be needed during rendering. This
         * method is automatically called as part of ManagedShaderProgram::setup(). If any errors occur then an Exception should
         * be raised and this will cause ManagedShaderProgram::setup() call to clean up and then return false. The
         * CACHE_SHADER_CONSTANT() macro is provided to make implementing this method less verbose.
         */
        virtual void cache() = 0;

        /**
         * The ShaderProgram instance being managed.
         */
        ShaderProgram* program_ = nullptr;

    private:

        struct MappedVertexAttribute
        {
            unsigned int vertexStreamType = 0;
            int index = -1;

            MappedVertexAttribute() {}
            MappedVertexAttribute(unsigned int streamType_, int index_) : vertexStreamType(streamType_), index(index_) {}
        };
        Vector<MappedVertexAttribute> mappedVertexAttributes_;
    };

    /**
     * Sets up the state required for using the given texture on the given texture unit. This contains fallbacks if the texture
     * failed to load.
     */
    static void setTexture(unsigned int unit, const Texture* texture, const Texture* fallback = nullptr);

    /**
     * Converts the pointer value in the given Parameter into a Texture pointer and calls Shader::setTexture() with it.
     */
    static void setTexture(unsigned int unit, const Parameter& parameter, const Texture* fallback = nullptr)
    {
        setTexture(unit, parameter.getPointer<Texture>(), fallback);
    }

    /**
     * Uses a vertex stream from the given geometry chunk to source vertex attribute data from.
     */
    static void setVertexAttributeArray(const GeometryChunk& geometryChunk, unsigned int attributeIndex,
                                        unsigned int streamType);

protected:

    /**
     * Initializes this shader and allocates required resources.
     */
    virtual bool initialize() { return true; }

    /**
     * Releases all resources used by this shader such as shader programs, textures, and so on.
     */
    virtual void uninitialize() {}

    /**
     * For use in shaders that create shader program variants by compiling the same code with differing preprocessor defines,
     * this method compiles a single program identified by the passed index, where the index is a bitfield that indicates which
     * of the passed preprocessor defines should be present in its compilation. Returns a pointer to the successfully set up
     * program or null on failure.
     */
    template <typename ManagedShaderProgramSubclass, size_t DefineCount, typename... ShaderFiles>
    ManagedShaderProgramSubclass*
        setupProgramCombination(unsigned int programIndex, Vector<ManagedShaderProgramSubclass*>& programs,
                                const std::array<String, DefineCount>& preprocessorDefines, ShaderFiles&&... shaderFiles)
    {
        // If the program has already been done then return it if the setup was successful
        if (programs[programIndex])
            return programs[programIndex]->getProgram() ? programs[programIndex] : nullptr;

        auto defines = String();

        // Work out the enabled flags for each of the preprocessor defines for this combination
        auto enabledDefines = std::array<bool, DefineCount>();
        for (auto j = 0U; j < enabledDefines.size(); j++)
        {
            enabledDefines[j] = (programIndex & (1 << j)) != 0;
            if (enabledDefines[j])
                defines << preprocessorDefines[j] << String::Newline;
        }

        // Create a list of the shader files to link in when creating this shader program
        auto files = Vector<String>{std::forward<ShaderFiles>(shaderFiles)...};

        // Create a new shader program and tell it what preprocessor defines are enabled for it
        programs[programIndex] = new ManagedShaderProgramSubclass(enabledDefines);

        // Setup the shader program
        if (programs[programIndex]->setup(requiredShaderLanguage_, files, defines))
            return programs[programIndex];

        return nullptr;
    }

    /**
     * For use in shaders that create shader program variants using Shader::setupProgramCombination(), returns the index into
     * the array of shader programs that should be used when rendering using the given geometry chunk and parameters. This
     * method is templated with each of the shader components used by the shader and uses the static `isPresent()` method on
     * each of the shader component classes to determine whether that particular component is enabled. The returned index value
     * is computed based on which components are enabled.
     */
    template <typename ShaderComponent>
    static unsigned int getShaderProgramIndex(const ParameterArray& params, const ParameterArray& internalParams)
    {
        return ShaderComponent::isPresent(params, internalParams) ? 1 : 0;
    }

    /**
     * \copydoc Shader::getShaderProgramIndex()
     */
    template <typename FirstShaderComponent, typename SecondShaderComponent, typename... FurtherShaderComponents>
    static unsigned int getShaderProgramIndex(const ParameterArray& params, const ParameterArray& internalParams)
    {
        return getShaderProgramIndex<FirstShaderComponent>(params, internalParams) |
            (getShaderProgramIndex<SecondShaderComponent, FurtherShaderComponents...>(params, internalParams) << 1);
    }

private:

    String className_;
    String effectName_;
    unsigned int quality_ = 0;
    ShaderProgram::ShaderLanguage requiredShaderLanguage_ = ShaderProgram::NoShaderLanguage;

    bool isSetup_ = false;
    bool needsInitialize_ = true;

    friend class ShaderRegistry;
};

/**
 * For use by ManagedShaderProgram::cache() implementations, assigns a member variable with the shader constant of the same
 * name and throws an Exception on failure (i.e. if the shader constant doesn't exist).
 */
#define CACHE_SHADER_CONSTANT(Name)                                    \
    do                                                                 \
    {                                                                  \
        Name = program_->getConstant(#Name, #Name);                    \
        if (!Name)                                                     \
            throw Exception("Failed caching shader constant: " #Name); \
    } while (false)
}
