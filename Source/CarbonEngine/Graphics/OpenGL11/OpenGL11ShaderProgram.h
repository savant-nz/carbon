/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/OpenGL11/OpenGL11ShaderConstant.h"
#include "CarbonEngine/Graphics/OpenGLShared/OpenGLShaderProgramShared.h"

namespace Carbon
{

using namespace OpenGL11Extensions;

/**
 * Implementation of ShaderProgram for the OpenGL Shading Language version 1.10 operating under OpenGL 1.1 with the
 * relevant extensions.
 */
class OpenGL11ShaderProgram : public OpenGLShaderProgramShared
{
public:

    OpenGL11ShaderProgram() : OpenGLShaderProgramShared(GLSL110) {}

    ~OpenGL11ShaderProgram() override { clear(); }

    Vector<String> getVertexAttributes() const override
    {
        auto attributes = Vector<String>();

        if (!getProgram())
            return attributes;

        // Get the number of attributes and the length of the longest attribute name
        auto attributeCount = 0;
        auto maxLength = 0;
        glGetObjectParameterivARB(getProgram(), GL_OBJECT_ACTIVE_ATTRIBUTES_ARB, &attributeCount);
        CARBON_CHECK_OPENGL_ERROR(glGetObjectParameterivARB);
        glGetObjectParameterivARB(getProgram(), GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB, &maxLength);
        CARBON_CHECK_OPENGL_ERROR(glGetObjectParameterivARB);

        attributes.resize(attributeCount);

        // Read names for all the vertex attributes in this shader program
        for (auto i = 0U; i < attributes.size(); i++)
        {
            auto name = Vector<char>(maxLength + 1);

            auto attributeSize = GLint();
            auto attributeType = GLenum();
            glGetActiveAttribARB(getProgram(), i, maxLength, nullptr, &attributeSize, &attributeType, name.getData());
            CARBON_CHECK_OPENGL_ERROR(glGetActiveAttribARB);

            attributes[i] = name.getData();
        }

        return attributes;
    }

    int getVertexAttributeIndex(const String& name) override
    {
        auto location = glGetAttribLocationARB(getProgram(), name.cStr());
        CARBON_CHECK_OPENGL_ERROR(glGetAttribLocationARB);

        if (location == -1)
            LOG_WARNING << "Vertex attribute doesn't exist in this shader program: " << name;

        return location;
    }

protected:

    // The shader type is determined by the file extension
    GLenum getOpenGLShaderType(const String& filename) const override
    {
        if (filename.endsWith(".vert"))
            return GL_VERTEX_SHADER_ARB;

        if (filename.endsWith(".frag"))
            return GL_FRAGMENT_SHADER_ARB;

        return OpenGLShaderProgramShared::getOpenGLShaderType(filename);
    }

    ShaderConstant* getConstantUncached(const String& name, const String& parameterName) override
    {
        auto location = glGetUniformLocationARB(getProgram(), name.cStr());
        CARBON_CHECK_OPENGL_ERROR(glGetUniformLocationARB);

        return location != -1 ? new OpenGL11ShaderConstant(parameterName, location) : nullptr;
    }

    GLuint createProgram() override
    {
        auto program = glCreateProgramObjectARB();
        CARBON_CHECK_OPENGL_ERROR(glCreateProgramObjectARB);

        return program;
    }

    void deleteProgram() override
    {
        glDeleteObjectARB(getProgram());
        CARBON_CHECK_OPENGL_ERROR(glDeleteObjectARB);
    }

    String getSourcePrefix(GLenum glShaderType) override { return "#version 110\n"; }

    bool linkProgram() override
    {
        auto isLinked = GLint(GL_FALSE);

        glLinkProgramARB(getProgram());
        CARBON_CHECK_OPENGL_ERROR(glLinkProgramARB);
        glGetObjectParameterivARB(getProgram(), GL_OBJECT_LINK_STATUS_ARB, &isLinked);
        CARBON_CHECK_OPENGL_ERROR(glGetObjectParameterivARB);

        return isLinked == GL_TRUE;
    }

    String getLinkerOutput() override { return getObjectInfoLog(getProgram()); }

    GLuint createShader(GLenum glShaderType) override
    {
        auto glShader = glCreateShaderObjectARB(glShaderType);
        CARBON_CHECK_OPENGL_ERROR(glCreateShaderObjectARB);

        return glShader;
    }

    void deleteShader(GLuint glShader) override
    {
        glDeleteObjectARB(glShader);
        CARBON_CHECK_OPENGL_ERROR(glDeleteObjectARB);
    }

    bool compileShader(GLuint glShader, const String& source) override
    {
        auto glSource = reinterpret_cast<const char*>(source.cStr());
        auto glLength = GLint(source.length());
        glShaderSourceARB(glShader, 1, &glSource, &glLength);
        CARBON_CHECK_OPENGL_ERROR(glShaderSourceARB);

        glCompileShaderARB(glShader);
        CARBON_CHECK_OPENGL_ERROR(glCompileShaderARB);

        auto isCompiled = GLint(GL_FALSE);
        glGetObjectParameterivARB(glShader, GL_OBJECT_COMPILE_STATUS_ARB, &isCompiled);
        CARBON_CHECK_OPENGL_ERROR(glGetObjectParameterivARB);

        return isCompiled == GL_TRUE;
    }

    String getCompilerOutput(GLuint glShader) override { return getObjectInfoLog(glShader); }

    void attachShader(GLuint glShader) override
    {
        glAttachObjectARB(getProgram(), glShader);
        CARBON_CHECK_OPENGL_ERROR(glAttachObjectARB);
    }

    String getObjectInfoLog(GLuint glObject)
    {
        auto length = 0;
        glGetObjectParameterivARB(glObject, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
        CARBON_CHECK_OPENGL_ERROR(glGetObjectParameterivARB);

        if (length > 1)
        {
            auto output = Vector<GLcharARB>(length);
            glGetInfoLogARB(glObject, length, nullptr, output.getData());
            CARBON_CHECK_OPENGL_ERROR(glGetInfoLogARB);

            return output.as<char>();
        }

        return {};
    }
};

}
