/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/ShaderProgram.h"
#include "CarbonEngine/Graphics/OpenGLShared/OpenGLShaderProgramCoreShared.h"

namespace Carbon
{

/**
 * Implementation of ShaderProgram for the OpenGL ES Shading Language version 1.00 operating under OpenGL ES 2.0.
 */
class OpenGLES2ShaderProgram : public OpenGLShaderProgramCoreShared
{
public:

    OpenGLES2ShaderProgram() : OpenGLShaderProgramCoreShared(GLSL110) {}

protected:

    String getSourcePrefix(GLenum glShaderType) override
    {
        auto prefix = String("#version 100\n");

        // Set the fragment shader precision to medium
        if (glShaderType == GL_FRAGMENT_SHADER)
            prefix += "precision mediump float;\n";

        return prefix;
    }
};

}
