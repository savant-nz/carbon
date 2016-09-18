/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/OpenGLShared/OpenGLShaderProgramCoreShared.h"

namespace Carbon
{

/**
 * Implementation of ShaderProgram for the OpenGL Shading Language version 1.10 and version 4.10 operating under OpenGL
 * 4.1 Core Profile. Note that a number of preprocessor macros are added by OpenGL41ShaderProgram::getSourcePrefix() in
 * order to make GLSL 1.10 syntax compile as GLSL 4.10 code, this is needed because GLSL 4.10 is the only version
 * supported by OpenGL 4.1 Core Profile.
 */
class OpenGL41ShaderProgram : public OpenGLShaderProgramCoreShared
{
public:

    OpenGL41ShaderProgram(ShaderLanguage language) : OpenGLShaderProgramCoreShared(language) {}

protected:

    // Recognize geometry shader files
    GLenum getOpenGLShaderType(const String& filename) const override
    {
        if (filename.endsWith(".geom"))
            return GL_GEOMETRY_SHADER;

        return OpenGLShaderProgramCoreShared::getOpenGLShaderType(filename);
    }

    // Define macros so that GLSL 1.10 code will compile
    String getSourcePrefix(GLenum glShaderType) override
    {
        auto prefix = String("#version 410\n");

        if (getLanguage() == GLSL110)
        {
            prefix << "#define texture2D texture\n"
                      "#define texture2DLod textureLod\n"
                      "#define texture3D texture\n"
                      "#define texture3DLod textureLod\n"
                      "#define textureCube texture\n"
                      "#define textureCubeLod textureLod\n";

            if (glShaderType == GL_VERTEX_SHADER)
            {
                prefix << "#define attribute in\n"
                          "#define varying out\n";
            }
            else if (glShaderType == GL_FRAGMENT_SHADER)
            {
                prefix << "#define varying in\n"
                          "#define gl_FragColor fragColor\n"
                          "out vec4 fragColor;\n";
            }
        }

        return prefix;
    }
};

}
