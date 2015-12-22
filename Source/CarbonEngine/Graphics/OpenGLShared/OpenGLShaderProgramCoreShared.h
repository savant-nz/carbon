/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/OpenGLShared/OpenGLShaderProgramShared.h"
#include "CarbonEngine/Graphics/OpenGLShared/OpenGLShaderConstantCoreShared.h"

namespace Carbon
{

/**
 * This is an almost complete implementation of ShaderProgram for OpenGL and GLSL that assumes core OpenGL functions are
 * available for use. This class is subclassed by the OpenGLES2ShaderProgram and OpenGL41ShaderProgram classes which each make
 * only a few small alterations.
 */
class OpenGLShaderProgramCoreShared : public OpenGLShaderProgramShared
{
public:

    OpenGLShaderProgramCoreShared(ShaderLanguage language) : OpenGLShaderProgramShared(language) {}

    ~OpenGLShaderProgramCoreShared() override { clear(); }

    Vector<String> getVertexAttributes() const override
    {
        auto attributes = Vector<String>();

        if (!getProgram())
            return attributes;

        // Get the number of attributes and the length of the longest attribute name
        auto attributeCount = GLint();
        glGetProgramiv(getProgram(), GL_ACTIVE_ATTRIBUTES, &attributeCount);
        CARBON_CHECK_OPENGL_ERROR(glGetProgramiv);

        auto maxLength = GLint();
        glGetProgramiv(getProgram(), GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLength);
        CARBON_CHECK_OPENGL_ERROR(glGetProgramiv);

        attributes.resize(attributeCount);

        // Read names for all the vertex attributes in this shader program
        for (auto i = 0U; i < attributes.size(); i++)
        {
            auto name = Vector<GLchar>(maxLength + 1, 0);

            auto attributeSize = GLint();
            auto attributeType = GLenum();
            glGetActiveAttrib(getProgram(), i, maxLength, nullptr, &attributeSize, &attributeType, name.getData());
            CARBON_CHECK_OPENGL_ERROR(glGetActiveAttrib);

            attributes[i] = name.getData();
        }

        return attributes;
    }

    int getVertexAttributeIndex(const String& name) override
    {
        auto location = glGetAttribLocation(getProgram(), name.cStr());
        CARBON_CHECK_OPENGL_ERROR(glGetAttribLocation);

        if (location == -1)
            LOG_WARNING << "Vertex attribute doesn't exist in this shader program: " << name;

        return location;
    }

protected:

    // The shader type is determined by the file extension
    GLenum getOpenGLShaderType(const String& filename) const override
    {
        if (filename.endsWith(".vert"))
            return GL_VERTEX_SHADER;

        if (filename.endsWith(".frag"))
            return GL_FRAGMENT_SHADER;

        return OpenGLShaderProgramShared::getOpenGLShaderType(filename);
    }

    ShaderConstant* getConstantUncached(const String& name, const String& parameterName) override
    {
        auto location = glGetUniformLocation(getProgram(), name.cStr());
        CARBON_CHECK_OPENGL_ERROR(glGetUniformLocation);

        return location != -1 ? new OpenGLShaderConstantCoreShared(parameterName, location) : nullptr;
    }

    GLuint createProgram() override
    {
        auto program = glCreateProgram();
        CARBON_CHECK_OPENGL_ERROR(glCreateProgram);

        return program;
    }

    void deleteProgram() override
    {
        glDeleteProgram(getProgram());
        CARBON_CHECK_OPENGL_ERROR(glDeleteProgram);
    }

    bool linkProgram() override
    {
        auto isLinked = GLint();

        glLinkProgram(getProgram());
        CARBON_CHECK_OPENGL_ERROR(glLinkProgram);
        glGetProgramiv(getProgram(), GL_LINK_STATUS, &isLinked);
        CARBON_CHECK_OPENGL_ERROR(glGetProgramiv);

        return isLinked != 0;
    }

    String getLinkerOutput() override
    {
        auto length = GLint();
        glGetProgramiv(getProgram(), GL_INFO_LOG_LENGTH, &length);
        CARBON_CHECK_OPENGL_ERROR(glGetProgramiv);

        if (length > 1)
        {
            auto output = Vector<GLchar>(length);
            glGetProgramInfoLog(getProgram(), length, nullptr, output.getData());
            CARBON_CHECK_OPENGL_ERROR(glGetProgramInfoLog);

            return output.as<char>();
        }

        return {};
    }

    GLuint createShader(GLenum glShaderType) override
    {
        auto glShader = glCreateShader(glShaderType);
        CARBON_CHECK_OPENGL_ERROR(glCreateShader);

        return glShader;
    }

    void deleteShader(GLuint glShader) override
    {
        glDeleteShader(glShader);
        CARBON_CHECK_OPENGL_ERROR(glDeleteShader);
    }

    bool compileShader(GLuint glShader, const String& source) override
    {
        auto glSource = reinterpret_cast<const GLchar*>(source.cStr());
        auto glLength = GLint(source.length());
        glShaderSource(glShader, 1, &glSource, &glLength);
        CARBON_CHECK_OPENGL_ERROR(glShaderSource);

        glCompileShader(glShader);
        CARBON_CHECK_OPENGL_ERROR(glCompileShader);

        auto isCompiled = GLint();
        glGetShaderiv(glShader, GL_COMPILE_STATUS, &isCompiled);
        CARBON_CHECK_OPENGL_ERROR(glGetShaderiv);

        return isCompiled != 0;
    }

    String getCompilerOutput(GLuint glShader) override
    {
        auto length = GLint();
        glGetShaderiv(glShader, GL_INFO_LOG_LENGTH, &length);
        CARBON_CHECK_OPENGL_ERROR(glGetShaderiv);

        if (length > 1)
        {
            auto output = Vector<GLchar>(length);
            glGetShaderInfoLog(glShader, length, nullptr, output.getData());
            CARBON_CHECK_OPENGL_ERROR(glGetShaderInfoLog);

            return output.as<char>();
        }

        return {};
    }

    void attachShader(GLuint glShader) override
    {
        glAttachShader(getProgram(), glShader);
        CARBON_CHECK_OPENGL_ERROR(glAttachShader);
    }
};

}
