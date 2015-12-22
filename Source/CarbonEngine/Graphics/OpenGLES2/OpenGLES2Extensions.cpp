/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGLES2

#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2.h"
#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2Extensions.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

namespace OpenGLES2Extensions
{

// GL_OES_vertex_array_object
CARBON_OPENGL_DEFINE_EXTENSION_FUNCTION(glBindVertexArrayOES);
CARBON_OPENGL_DEFINE_EXTENSION_FUNCTION(glDeleteVertexArraysOES);
CARBON_OPENGL_DEFINE_EXTENSION_FUNCTION(glGenVertexArraysOES);
CARBON_OPENGL_DEFINE_EXTENSION_FUNCTION(glIsVertexArrayOES);

void mapFunctions()
{
    // GL_OES_vertex_array_object
    CARBON_OPENGL_MAP_EXTENSION_FUNCTION(glBindVertexArrayOES);
    CARBON_OPENGL_MAP_EXTENSION_FUNCTION(glDeleteVertexArraysOES);
    CARBON_OPENGL_MAP_EXTENSION_FUNCTION(glGenVertexArraysOES);
    CARBON_OPENGL_MAP_EXTENSION_FUNCTION(glIsVertexArrayOES);
}

}

}

#endif
