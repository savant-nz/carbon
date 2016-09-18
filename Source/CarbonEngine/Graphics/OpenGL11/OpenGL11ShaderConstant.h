/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/OpenGL11/OpenGL11.h"
#include "CarbonEngine/Graphics/ShaderConstant.h"

namespace Carbon
{

using namespace OpenGL11Extensions;

/**
 * Implementation of ShaderConstant for GLSL under OpenGL 1.1 that supports all types of constants including floats,
 * integers, arrays and matrices.
 */
class OpenGL11ShaderConstant : public ShaderConstant
{
public:

    OpenGL11ShaderConstant(const String& parameterName, GLint uniformLocation)
        : ShaderConstant(parameterName), uniformLocation_(uniformLocation)
    {
    }

    void setArray(unsigned int componentCount, unsigned int itemCount, const float* f) override
    {
        if (componentCount == 1)
        {
            glUniform1fvARB(uniformLocation_, itemCount, f);
            CARBON_CHECK_OPENGL_ERROR(glUniform1fvARB);
        }
        else if (componentCount == 2)
        {
            glUniform2fvARB(uniformLocation_, itemCount, f);
            CARBON_CHECK_OPENGL_ERROR(glUniform2fvARB);
        }
        else if (componentCount == 3)
        {
            glUniform3fvARB(uniformLocation_, itemCount, f);
            CARBON_CHECK_OPENGL_ERROR(glUniform3fvARB);
        }
        else if (componentCount == 4)
        {
            glUniform4fvARB(uniformLocation_, itemCount, f);
            CARBON_CHECK_OPENGL_ERROR(glUniform4fvARB);
        }
    }

private:

    const GLint uniformLocation_ = 0;

    void setFloatUncached(float f0) override
    {
        glUniform1fARB(uniformLocation_, f0);
        CARBON_CHECK_OPENGL_ERROR(glUniform1fARB);
    }

    void setFloat2Uncached(float f0, float f1) override
    {
        glUniform2fARB(uniformLocation_, f0, f1);
        CARBON_CHECK_OPENGL_ERROR(glUniform2fARB);
    }

    void setFloat3Uncached(float f0, float f1, float f2) override
    {
        glUniform3fARB(uniformLocation_, f0, f1, f2);
        CARBON_CHECK_OPENGL_ERROR(glUniform3fARB);
    }

    void setFloat4Uncached(float f0, float f1, float f2, float f3) override
    {
        glUniform4fARB(uniformLocation_, f0, f1, f2, f3);
        CARBON_CHECK_OPENGL_ERROR(glUniform4fARB);
    }

    void setIntegerUncached(int i0) override
    {
        glUniform1iARB(uniformLocation_, i0);
        CARBON_CHECK_OPENGL_ERROR(glUniform1iARB);
    }

    void setMatrix3Uncached(const Matrix3& m) override
    {
        glUniformMatrix3fvARB(uniformLocation_, 1, false, m.asArray());
        CARBON_CHECK_OPENGL_ERROR(glUniformMatrix3fvARB);
    }

    void setMatrix4Uncached(const Matrix4& m) override
    {
        glUniformMatrix4fvARB(uniformLocation_, 1, false, m.asArray());
        CARBON_CHECK_OPENGL_ERROR(glUniformMatrix4fvARB);
    }
};

}
