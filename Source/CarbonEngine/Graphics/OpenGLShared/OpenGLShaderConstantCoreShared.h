/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/ShaderConstant.h"

namespace Carbon
{

/**
 * Implementation of ShaderConstant for OpenGL and GLSL that assumes core OpenGL functions are available for use. This
 * class is used by OpenGLShaderProgramCoreShared and so gets used by both the OpenGLES2ShaderProgram class and
 * OpenGL41ShaderProgram class.
 */
class OpenGLShaderConstantCoreShared : public ShaderConstant
{
public:

    OpenGLShaderConstantCoreShared(const String& parameterName, GLint uniformLocation)
        : ShaderConstant(parameterName), uniformLocation_(uniformLocation)
    {
    }

    void setArray(unsigned int componentCount, unsigned int itemCount, const float* f) override
    {
        if (componentCount == 1)
        {
            glUniform1fv(uniformLocation_, itemCount, f);
            CARBON_CHECK_OPENGL_ERROR(glUniform1fv);
        }
        else if (componentCount == 2)
        {
            glUniform2fv(uniformLocation_, itemCount, f);
            CARBON_CHECK_OPENGL_ERROR(glUniform2fv);
        }
        else if (componentCount == 3)
        {
            glUniform3fv(uniformLocation_, itemCount, f);
            CARBON_CHECK_OPENGL_ERROR(glUniform3fv);
        }
        else if (componentCount == 4)
        {
            glUniform4fv(uniformLocation_, itemCount, f);
            CARBON_CHECK_OPENGL_ERROR(glUniform4fv);
        }
    }

private:

    const GLint uniformLocation_ = 0;

    void setFloatUncached(float f0) override
    {
        glUniform1f(uniformLocation_, f0);
        CARBON_CHECK_OPENGL_ERROR(glUniform1f);
    }

    void setFloat2Uncached(float f0, float f1) override
    {
        glUniform2f(uniformLocation_, f0, f1);
        CARBON_CHECK_OPENGL_ERROR(glUniform2f);
    }

    void setFloat3Uncached(float f0, float f1, float f2) override
    {
        glUniform3f(uniformLocation_, f0, f1, f2);
        CARBON_CHECK_OPENGL_ERROR(glUniform3f);
    }

    void setFloat4Uncached(float f0, float f1, float f2, float f3) override
    {
        glUniform4f(uniformLocation_, f0, f1, f2, f3);
        CARBON_CHECK_OPENGL_ERROR(glUniform1f);
    }

    void setIntegerUncached(int i0) override
    {
        glUniform1i(uniformLocation_, i0);
        CARBON_CHECK_OPENGL_ERROR(glUniform1i);
    }

    void setMatrix3Uncached(const Matrix3& m) override
    {
        glUniformMatrix3fv(uniformLocation_, 1, false, m.asArray());
        CARBON_CHECK_OPENGL_ERROR(glUniformMatrix3fv);
    }

    void setMatrix4Uncached(const Matrix4& m) override
    {
        glUniformMatrix4fv(uniformLocation_, 1, false, m.asArray());
        CARBON_CHECK_OPENGL_ERROR(glUniformMatrix4fv);
    }
};

}
