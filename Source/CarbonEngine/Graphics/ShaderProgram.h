/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Provides a common interface for handling shader programs that is independent of the underlying shader language being
 * used. This class is subclassed for each individual shader language that is supported, and instances of these
 * subclasses are allocated through the GraphicsInterface. Functionality common to all shader programming languages is
 * handled by this class, including accessing and assignment of shader constant values (through
 * ShaderProgram::getConstant() and the ShaderConstant class), constructing a shader program from multiple source files,
 * and enumerating and mapping named vertex attributes.
 */
class CARBON_API ShaderProgram : private Noncopyable
{
public:

    /**
     * Supported shader languages.
     */
    enum ShaderLanguage
    {
        /**
         * Unspecified shader language.
         */
        NoShaderLanguage,

        /**
         * The OpenGL Shading Language version 1.10, this is aliased with the OpenGL ES Shading Language version 1.00
         * and only the subset of functionality shared by both languages should be used unless additional functionality
         * provided by an implementation or extension is explicitly verified to be present.
         */
        GLSL110,

        /**
         * The OpenGL Shading Language version 4.10, intended for use in tandem with OpenGL 4.1 Core Profile.
         */
        GLSL410
    };

    /**
     * Constructs this shader program and specifies which language it is implementing. The language is set by the
     * specific subclass that is implementing ShaderProgram for that language.
     */
    ShaderProgram(ShaderLanguage language) : language_(language) {}

    virtual ~ShaderProgram() { clear(); }

    /**
     * Returns the shader language that this shader program is using.
     */
    ShaderLanguage getLanguage() const { return language_; }

    /**
     * Clears this shader program.
     */
    virtual void clear();

    /**
     * Returns the source code for any preprocessor defines that has been set by a call to
     * ShaderProgram::setPreprocessorDefines().
     */
    const String& getPreprocessorDefines() const { return preprocessorDefines_; }

    /**
     * Sets the source code that will be inserted at the start of all shader source added with
     * ShaderProgram::addSource(), this is used to set preprocessor defines for this shader program.
     */
    void setPreprocessorDefines(const String& preprocessorDefines) { preprocessorDefines_ = preprocessorDefines; }

    /**
     * Adds the given shader source code to this shader program. This method can be called multiple times to add all
     * required source code. The ShaderProgram::link() method must be called before this shader program can be used in
     * rendering. Returns success flag. The \a filename parameter indicates either the name of the shader file that the
     * source was loaded from, or if the source has been generated at runtime then it should be a sensible filename that
     * the source could have been loaded from. The filename extension may be used by a ShaderProgram subclass to
     * determine the type of shader.
     */
    virtual bool addSource(const String& source, const String& filename) = 0;

    /**
     * Links together all the shader program source added via ShaderProgram::addSource() into a final program usable in
     * rendering. Returns success flag.
     */
    virtual bool link() = 0;

    /**
     * Once this program has been linked this method returns a vector of all the vertex attributes it uses.
     */
    virtual Vector<String> getVertexAttributes() const = 0;

    /**
     * If this shader program supports vertex attributes then this method will return the index for the vertex attribute
     * of the given name. Otherwise -1 is returned.
     */
    virtual int getVertexAttributeIndex(const String& name) = 0;

    /**
     * Returns a ShaderConstant instance that can be used to set the constant with the given name. Returns null if no
     * constant with the given name exists in this shader program.
     */
    ShaderConstant* getConstant(const String& name, const String& parameterName);

protected:

    /**
     * Applies a C-style preprocessor to the passed shader source code. This supports \#include, \#define, \#undef,
     * \#ifdef, \#else, and \#endif. All other preprocessor tokens are silently ignored. Returns success flag.
     */
    static bool preprocessShaderCode(const String& filename, Vector<String>& lines);

    /**
     * Logs the passed shader code to a collapsing section with line numbers in the main logfile.
     */
    static void logShaderCode(const String& filename, const String& shaderCode);

private:

    ShaderLanguage language_;

    String preprocessorDefines_;

    // Caching of shader constants
    struct CachedShaderConstant
    {
        String name;
        ShaderConstant* constant = nullptr;

        CachedShaderConstant() {}
        CachedShaderConstant(String name_, ShaderConstant* constant_) : name(std::move(name_)), constant(constant_) {}
    };
    Vector<CachedShaderConstant> constants_;
    virtual ShaderConstant* getConstantUncached(const String& name, const String& parameterName) = 0;
};

}
