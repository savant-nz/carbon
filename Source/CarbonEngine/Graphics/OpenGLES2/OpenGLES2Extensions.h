/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

namespace OpenGLES2Extensions
{

/**
 * Maps all the OpenGL extension functions defined in this namespace.
 */
extern void mapFunctions();

// GL_EXT_sRGB
#define GL_SRGB_EXT 0x8C40
#define GL_SRGB_ALPHA_EXT 0x8C42
#define GL_SRGB8_ALPHA8_EXT 0x8C43
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_EXT 0x8210

// GL_EXT_texture_compression_dxt1
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1

// GL_IMG_texture_compression_pvrtc
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03

// GL_OES_vertex_array_object
#define GL_VERTEX_ARRAY_BINDING_OES 0x85B5
typedef void (*PFnglBindVertexArrayOES)(GLuint array);
typedef void (*PFnglDeleteVertexArraysOES)(GLsizei n, const GLuint* arrays);
typedef void (*PFnglGenVertexArraysOES)(GLsizei n, GLuint* arrays);
typedef GLboolean (*PFnglIsVertexArrayOES)(GLuint array);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glBindVertexArrayOES);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glDeleteVertexArraysOES);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGenVertexArraysOES);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glIsVertexArrayOES);

}

}
