/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#ifndef APIENTRY
    #define APIENTRY
#endif

namespace Carbon
{

namespace OpenGL11Extensions
{

/**
 * Maps all the OpenGL extension functions defined in this namespace.
 */
extern void mapFunctions();

// Data types added in OpenGL 1.2
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366

// GL_APPLE_vertex_array_object
#define GL_VERTEX_ARRAY_BINDING_APPLE 0x85B5
typedef void(APIENTRY* PFnglBindVertexArrayAPPLE)(GLuint array);
typedef void(APIENTRY* PFnglDeleteVertexArraysAPPLE)(GLsizei n, const GLuint* arrays);
typedef void(APIENTRY* PFnglGenVertexArraysAPPLE)(GLsizei n, GLuint* arrays);
typedef void(APIENTRY* PFnglIsVertexArrayAPPLE)(GLuint array);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glBindVertexArrayAPPLE);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glDeleteVertexArraysAPPLE);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGenVertexArraysAPPLE);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glIsVertexArrayAPPLE);

// GL_ARB_color_buffer_float
#define GL_RGBA_FLOAT_MODE_ARB 0x8820
#define GL_CLAMP_VERTEX_COLOR_ARB 0x891A
#define GL_CLAMP_FRAGMENT_COLOR_ARB 0x891B
#define GL_CLAMP_READ_COLOR_ARB 0x891C
#define GL_FIXED_ONLY_ARB 0x891D
#define WGL_TYPE_RGBA_FLOAT_ARB 0x21A0
#define GLX_RGBA_FLOAT_TYPE_ARB 0x20B9
typedef void(APIENTRY* PFnglClampColorARB)(GLenum target, GLenum clamp);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glClampColorARB);

// GL_ARB_depth_clamp
#define GL_DEPTH_CLAMP_ARB 0x864F

// GL_ARB_draw_buffers
#define GL_MAX_DRAW_BUFFERS_ARB 0x8824
#define GL_DRAW_BUFFER0_ARB 0x8825
#define GL_DRAW_BUFFER1_ARB 0x8826
#define GL_DRAW_BUFFER2_ARB 0x8827
#define GL_DRAW_BUFFER3_ARB 0x8828
#define GL_DRAW_BUFFER4_ARB 0x8829
#define GL_DRAW_BUFFER5_ARB 0x882A
#define GL_DRAW_BUFFER6_ARB 0x882B
#define GL_DRAW_BUFFER7_ARB 0x882C
#define GL_DRAW_BUFFER8_ARB 0x882D
#define GL_DRAW_BUFFER9_ARB 0x882E
#define GL_DRAW_BUFFER10_ARB 0x882F
#define GL_DRAW_BUFFER11_ARB 0x8830
#define GL_DRAW_BUFFER12_ARB 0x8831
#define GL_DRAW_BUFFER13_ARB 0x8832
#define GL_DRAW_BUFFER14_ARB 0x8833
#define GL_DRAW_BUFFER15_ARB 0x8834
typedef void(APIENTRY* PFnglDrawBuffersARB)(GLsizei n, const GLenum* bufs);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glDrawBuffersARB);

// GL_ARB_fragment_shader
#define GL_FRAGMENT_SHADER_ARB 0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB 0x8B49
#define GL_MAX_TEXTURE_COORDS_ARB 0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB 0x8872
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB 0x8B8B

// GL_ARB_framebuffer_srgb
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20B2
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20A9
#define GL_FRAMEBUFFER_SRGB_ARB 0x8DB9

// GL_ARB_half_float_pixel
#define GL_HALF_FLOAT_ARB 0x140B

// GL_ARB_multisample promoted to core in OpenGL 1.3
#define GL_MULTISAMPLE_ARB 0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB 0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_ARB 0x809F
#define GL_SAMPLE_COVERAGE_ARB 0x80A0
#define GL_MULTISAMPLE_BIT_ARB 0x20000000
#define GL_SAMPLE_BUFFERS_ARB 0x80A8
#define GL_SAMPLES_ARB 0x80A9
#define GL_SAMPLE_COVERAGE_VALUE_ARB 0x80AA
#define GL_SAMPLE_COVERAGE_INVERT_ARB 0x80AB
typedef void(APIENTRY* PFnglSampleCoverageARB)(GLclampf value, GLboolean invert);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glSampleCoverageARB);

// GL_ARB_multitexture, promoted to core in OpenGL 1.3 Immediate mode multitexture functions are not defined
#define GL_TEXTURE0_ARB 0x84C0
#define GL_TEXTURE1_ARB 0x84C1
#define GL_TEXTURE2_ARB 0x84C2
#define GL_TEXTURE3_ARB 0x84C3
#define GL_TEXTURE4_ARB 0x84C4
#define GL_TEXTURE5_ARB 0x84C5
#define GL_TEXTURE6_ARB 0x84C6
#define GL_TEXTURE7_ARB 0x84C7
#define GL_TEXTURE8_ARB 0x84C8
#define GL_TEXTURE9_ARB 0x84C9
#define GL_TEXTURE10_ARB 0x84CA
#define GL_TEXTURE11_ARB 0x84CB
#define GL_TEXTURE12_ARB 0x84CC
#define GL_TEXTURE13_ARB 0x84CD
#define GL_TEXTURE14_ARB 0x84CE
#define GL_TEXTURE15_ARB 0x84CF
#define GL_TEXTURE16_ARB 0x84D0
#define GL_TEXTURE17_ARB 0x84D1
#define GL_TEXTURE18_ARB 0x84D2
#define GL_TEXTURE19_ARB 0x84D3
#define GL_TEXTURE20_ARB 0x84D4
#define GL_TEXTURE21_ARB 0x84D5
#define GL_TEXTURE22_ARB 0x84D6
#define GL_TEXTURE23_ARB 0x84D7
#define GL_TEXTURE24_ARB 0x84D8
#define GL_TEXTURE25_ARB 0x84D9
#define GL_TEXTURE26_ARB 0x84DA
#define GL_TEXTURE27_ARB 0x84DB
#define GL_TEXTURE28_ARB 0x84DC
#define GL_TEXTURE29_ARB 0x84DD
#define GL_TEXTURE30_ARB 0x84DE
#define GL_TEXTURE31_ARB 0x84DF
#define GL_ACTIVE_TEXTURE_ARB 0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB 0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB 0x84E2
typedef void(APIENTRY* PFnglActiveTextureARB)(GLenum texture);
typedef void(APIENTRY* PFnglClientActiveTextureARB)(GLenum texture);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glActiveTextureARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glClientActiveTextureARB);

// GL_ARB_shader_objects
#define GL_PROGRAM_OBJECT_ARB 0x8B40
#define GL_OBJECT_TYPE_ARB 0x8B4E
#define GL_OBJECT_SUBTYPE_ARB 0x8B4F
#define GL_OBJECT_DELETE_STATUS_ARB 0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB 0x8B81
#define GL_OBJECT_LINK_STATUS_ARB 0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB 0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB 0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB 0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB 0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB 0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB 0x8B88
#define GL_SHADER_OBJECT_ARB 0x8B48
#define GL_FLOAT_VEC2_ARB 0x8B50
#define GL_FLOAT_VEC3_ARB 0x8B51
#define GL_FLOAT_VEC4_ARB 0x8B52
#define GL_INT_VEC2_ARB 0x8B53
#define GL_INT_VEC3_ARB 0x8B54
#define GL_INT_VEC4_ARB 0x8B55
#define GL_BOOL_ARB 0x8B56
#define GL_BOOL_VEC2_ARB 0x8B57
#define GL_BOOL_VEC3_ARB 0x8B58
#define GL_BOOL_VEC4_ARB 0x8B59
#define GL_FLOAT_MAT2_ARB 0x8B5A
#define GL_FLOAT_MAT3_ARB 0x8B5B
#define GL_FLOAT_MAT4_ARB 0x8B5C
#define GL_SAMPLER_1D_ARB 0x8B5D
#define GL_SAMPLER_2D_ARB 0x8B5E
#define GL_SAMPLER_3D_ARB 0x8B5F
#define GL_SAMPLER_CUBE_ARB 0x8B60
#define GL_SAMPLER_1D_SHADOW_ARB 0x8B61
#define GL_SAMPLER_2D_SHADOW_ARB 0x8B62
#define GL_SAMPLER_2D_RECT_ARB 0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB 0x8B64
typedef char GLcharARB;
typedef unsigned int GLhandleARB;
typedef void(APIENTRY* PFnglDeleteObjectARB)(GLhandleARB obj);
typedef GLhandleARB(APIENTRY* PFnglGetHandleARB)(GLenum pname);
typedef void(APIENTRY* PFnglDetachObjectARB)(GLhandleARB containerObj, GLhandleARB attachedObj);
typedef GLhandleARB(APIENTRY* PFnglCreateShaderObjectARB)(GLenum shaderType);
typedef void(APIENTRY* PFnglShaderSourceARB)(GLhandleARB shaderObj, GLsizei count, const GLcharARB** string,
                                             const GLint* length);
typedef void(APIENTRY* PFnglCompileShaderARB)(GLhandleARB shaderObj);
typedef GLhandleARB(APIENTRY* PFnglCreateProgramObjectARB)(void);
typedef void(APIENTRY* PFnglAttachObjectARB)(GLhandleARB containerObj, GLhandleARB obj);
typedef void(APIENTRY* PFnglLinkProgramARB)(GLhandleARB programObj);
typedef void(APIENTRY* PFnglUseProgramObjectARB)(GLhandleARB programObj);
typedef void(APIENTRY* PFnglValidateProgramARB)(GLhandleARB programObj);
typedef void(APIENTRY* PFnglUniform1fARB)(GLint location, GLfloat v0);
typedef void(APIENTRY* PFnglUniform2fARB)(GLint location, GLfloat v0, GLfloat v1);
typedef void(APIENTRY* PFnglUniform3fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void(APIENTRY* PFnglUniform4fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void(APIENTRY* PFnglUniform1iARB)(GLint location, GLint v0);
typedef void(APIENTRY* PFnglUniform2iARB)(GLint location, GLint v0, GLint v1);
typedef void(APIENTRY* PFnglUniform3iARB)(GLint location, GLint v0, GLint v1, GLint v2);
typedef void(APIENTRY* PFnglUniform4iARB)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void(APIENTRY* PFnglUniform1fvARB)(GLint location, GLsizei count, const GLfloat* value);
typedef void(APIENTRY* PFnglUniform2fvARB)(GLint location, GLsizei count, const GLfloat* value);
typedef void(APIENTRY* PFnglUniform3fvARB)(GLint location, GLsizei count, const GLfloat* value);
typedef void(APIENTRY* PFnglUniform4fvARB)(GLint location, GLsizei count, const GLfloat* value);
typedef void(APIENTRY* PFnglUniform1ivARB)(GLint location, GLsizei count, const GLint* value);
typedef void(APIENTRY* PFnglUniform2ivARB)(GLint location, GLsizei count, const GLint* value);
typedef void(APIENTRY* PFnglUniform3ivARB)(GLint location, GLsizei count, const GLint* value);
typedef void(APIENTRY* PFnglUniform4ivARB)(GLint location, GLsizei count, const GLint* value);
typedef void(APIENTRY* PFnglUniformMatrix2fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void(APIENTRY* PFnglUniformMatrix3fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void(APIENTRY* PFnglUniformMatrix4fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void(APIENTRY* PFnglGetObjectParameterfvARB)(GLhandleARB obj, GLenum pname, GLfloat* params);
typedef void(APIENTRY* PFnglGetObjectParameterivARB)(GLhandleARB obj, GLenum pname, GLint* params);
typedef void(APIENTRY* PFnglGetInfoLogARB)(GLhandleARB obj, GLsizei maxLength, GLsizei* length, GLcharARB* infoLog);
typedef void(APIENTRY* PFnglGetAttachedObjectsARB)(GLhandleARB containerObj, GLsizei maxCount, GLsizei* count,
                                                   GLhandleARB* obj);
typedef GLint(APIENTRY* PFnglGetUniformLocationARB)(GLhandleARB programObj, const GLcharARB* name);
typedef void(APIENTRY* PFnglGetActiveUniformARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei* length,
                                                 GLint* size, GLenum* type, GLcharARB* name);
typedef void(APIENTRY* PFnglGetUniformfvARB)(GLhandleARB programObj, GLint location, GLfloat* params);
typedef void(APIENTRY* PFnglGetUniformivARB)(GLhandleARB programObj, GLint location, GLint* params);
typedef void(APIENTRY* PFnglGetShaderSourceARB)(GLhandleARB obj, GLsizei maxLength, GLsizei* length, GLcharARB* source);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glDeleteObjectARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetHandleARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glDetachObjectARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glCreateShaderObjectARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glShaderSourceARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glCompileShaderARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glCreateProgramObjectARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glAttachObjectARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glLinkProgramARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUseProgramObjectARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glValidateProgramARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform1fARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform2fARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform3fARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform4fARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform1iARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform2iARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform3iARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform4iARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform1fvARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform2fvARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform3fvARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform4fvARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform1ivARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform2ivARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform3ivARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniform4ivARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniformMatrix2fvARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniformMatrix3fvARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUniformMatrix4fvARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetObjectParameterfvARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetObjectParameterivARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetInfoLogARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetAttachedObjectsARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetUniformLocationARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetActiveUniformARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetUniformfvARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetUniformivARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetShaderSourceARB);

// GL_ARB_shading_language_100
#define GL_SHADING_LANGUAGE_VERSION_ARB 0x8B8C

// GL_ARB_shadow
#define GL_TEXTURE_COMPARE_MODE_ARB 0x884C
#define GL_TEXTURE_COMPARE_FUNC_ARB 0x884D
#define GL_COMPARE_R_TO_TEXTURE_ARB 0x884E

// GL_ARB_texture_compression
#define GL_COMPRESSED_ALPHA_ARB 0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB 0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB 0x84EB
#define GL_COMPRESSED_INTENSITY_ARB 0x84EC
#define GL_COMPRESSED_RGB_ARB 0x84ED
#define GL_COMPRESSED_RGBA_ARB 0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB 0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB 0x86A0
#define GL_TEXTURE_COMPRESSED_ARB 0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A3
typedef void(APIENTRY* PFnglCompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                                     GLint border, GLsizei imageSize, const GLvoid* data);
typedef void(APIENTRY* PFnglCompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                                     GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
typedef void(APIENTRY* PFnglCompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width,
                                                     GLsizei height, GLsizei depth, GLint border, GLsizei imageSize,
                                                     const GLvoid* data);
typedef void(APIENTRY* PFnglCompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format,
                                                        GLsizei imageSize, const GLvoid* data);
typedef void(APIENTRY* PFnglCompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
                                                        GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
typedef void(APIENTRY* PFnglCompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                                        GLsizei width, GLsizei height, GLsizei depth, GLenum format,
                                                        GLsizei imageSize, const GLvoid* data);
typedef void(APIENTRY* PFnglGetCompressedTexImageARB)(GLenum target, GLint level, GLvoid* img);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glCompressedTexImage1DARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glCompressedTexImage2DARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glCompressedTexImage3DARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glCompressedTexSubImage1DARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glCompressedTexSubImage2DARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glCompressedTexSubImage3DARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetCompressedTexImageARB);

// GL_ARB_texture_cube_map
#define GL_NORMAL_MAP_ARB 0x8511
#define GL_REFLECTION_MAP_ARB 0x8512
#define GL_TEXTURE_CUBE_MAP_ARB 0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB 0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB 0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB 0x851C

// GL_ARB_texture_float
#define GL_TEXTURE_RED_TYPE_ARB 0x8C10
#define GL_TEXTURE_GREEN_TYPE_ARB 0x8C11
#define GL_TEXTURE_BLUE_TYPE_ARB 0x8C12
#define GL_TEXTURE_ALPHA_TYPE_ARB 0x8C13
#define GL_TEXTURE_LUMINANCE_TYPE_ARB 0x8C14
#define GL_TEXTURE_INTENSITY_TYPE_ARB 0x8C15
#define GL_TEXTURE_DEPTH_TYPE_ARB 0x8C16
#define GL_UNSIGNED_NORMALIZED_ARB 0x8C17
#define GL_RGBA32F_ARB 0x8814
#define GL_RGB32F_ARB 0x8815
#define GL_ALPHA32F_ARB 0x8816
#define GL_INTENSITY32F_ARB 0x8817
#define GL_LUMINANCE32F_ARB 0x8818
#define GL_LUMINANCE_ALPHA32F_ARB 0x8819
#define GL_RGBA16F_ARB 0x881A
#define GL_RGB16F_ARB 0x881B
#define GL_ALPHA16F_ARB 0x881C
#define GL_INTENSITY16F_ARB 0x881D
#define GL_LUMINANCE16F_ARB 0x881E
#define GL_LUMINANCE_ALPHA16F_ARB 0x881F

// GL_ARB_texture_rg
#define GL_R8_ARB 0x8229
#define GL_R16_ARB 0x822A
#define GL_RG8_ARB 0x822B
#define GL_RG16_ARB 0x822C
#define GL_R16F_ARB 0x822D
#define GL_R32F_ARB 0x822E
#define GL_RG16F_ARB 0x822F
#define GL_RG32F_ARB 0x8230
#define GL_R8I_ARB 0x8231
#define GL_R8UI_ARB 0x8232
#define GL_R16I_ARB 0x8233
#define GL_R16UI_ARB 0x8234
#define GL_R32I_ARB 0x8235
#define GL_R32UI_ARB 0x8236
#define GL_RG8I_ARB 0x8237
#define GL_RG8UI_ARB 0x8238
#define GL_RG16I_ARB 0x8239
#define GL_RG16UI_ARB 0x823A
#define GL_RG32I_ARB 0x823B
#define GL_RG32UI_ARB 0x823C
#define GL_RED_ARB 0x1903
#define GL_RG_ARB 0x8227
#define GL_COMPRESSED_RED_ARB 0x8225
#define GL_COMPRESSED_RG_ARB 0x8226
#define GL_RG_INTEGER_ARB 0x8228

// GL_ARB_vertex_array_object
#define GL_VERTEX_ARRAY_BINDING 0x85B5
typedef void(APIENTRY* PFnglBindVertexArray)(GLuint array);
typedef void(APIENTRY* PFnglDeleteVertexArrays)(GLsizei n, const GLuint* arrays);
typedef void(APIENTRY* PFnglGenVertexArrays)(GLsizei n, GLuint* arrays);
typedef void(APIENTRY* PFnglIsVertexArray)(GLuint array);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glBindVertexArray);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glDeleteVertexArrays);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGenVertexArrays);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glIsVertexArray);

// GL_ARB_vertex_buffer_object, promoted to core in OpenGL 1.5
#define GL_ARRAY_BUFFER_ARB 0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB 0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB 0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB 0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB 0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB 0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB 0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB 0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB 0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB 0x889F
#define GL_STREAM_DRAW_ARB 0x88E0
#define GL_STREAM_READ_ARB 0x88E1
#define GL_STREAM_COPY_ARB 0x88E2
#define GL_STATIC_DRAW_ARB 0x88E4
#define GL_STATIC_READ_ARB 0x88E5
#define GL_STATIC_COPY_ARB 0x88E6
#define GL_DYNAMIC_DRAW_ARB 0x88E8
#define GL_DYNAMIC_READ_ARB 0x88E9
#define GL_DYNAMIC_COPY_ARB 0x88EA
#define GL_READ_ONLY_ARB 0x88B8
#define GL_WRITE_ONLY_ARB 0x88B9
#define GL_READ_WRITE_ARB 0x88BA
#define GL_BUFFER_SIZE_ARB 0x8764
#define GL_BUFFER_USAGE_ARB 0x8765
#define GL_BUFFER_ACCESS_ARB 0x88BB
#define GL_BUFFER_MAPPED_ARB 0x88BC
#define GL_BUFFER_MAP_POINTER_ARB 0x88BD
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
typedef void(APIENTRY* PFnglBindBufferARB)(GLenum target, GLuint buffer);
typedef void(APIENTRY* PFnglDeleteBuffersARB)(GLsizei n, const GLuint* buffers);
typedef void(APIENTRY* PFnglGenBuffersARB)(GLsizei n, GLuint* buffers);
typedef GLboolean(APIENTRY* PFnglIsBufferARB)(GLuint buffer);
typedef void(APIENTRY* PFnglBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid* data, GLenum usage);
typedef void(APIENTRY* PFnglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid* data);
typedef void(APIENTRY* PFnglGetBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid* data);
typedef GLvoid*(APIENTRY* PFnglMapBufferARB)(GLenum target, GLenum access);
typedef GLboolean(APIENTRY* PFnglUnmapBufferARB)(GLenum target);
typedef void(APIENTRY* PFnglGetBufferParameterivARB)(GLenum target, GLenum pname, GLint* params);
typedef void(APIENTRY* PFnglGetBufferPointervARB)(GLenum target, GLenum pname, GLvoid** params);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glBindBufferARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glDeleteBuffersARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGenBuffersARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glIsBufferARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glBufferDataARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glBufferSubDataARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetBufferSubDataARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glMapBufferARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glUnmapBufferARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetBufferParameterivARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetBufferPointervARB);

// GL_ARB_vertex_shader
#define GL_VERTEX_SHADER_ARB 0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB 0x8B4A
#define GL_MAX_VARYING_FLOATS_ARB 0x8B4B
#define GL_MAX_VERTEX_ATTRIBS_ARB 0x8869
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB 0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB 0x8B4D
#define GL_MAX_TEXTURE_COORDS_ARB 0x8871
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB 0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB 0x8643
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB 0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB 0x8B8A
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB 0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB 0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB 0x886A
#define GL_CURRENT_VERTEX_ATTRIB_ARB 0x8626
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB 0x8645
#define GL_FLOAT_VEC2_ARB 0x8B50
#define GL_FLOAT_VEC3_ARB 0x8B51
#define GL_FLOAT_VEC4_ARB 0x8B52
#define GL_FLOAT_MAT2_ARB 0x8B5A
#define GL_FLOAT_MAT3_ARB 0x8B5B
#define GL_FLOAT_MAT4_ARB 0x8B5C
typedef void(APIENTRY* PFnglVertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
                                                    const GLvoid* pointer);
typedef void(APIENTRY* PFnglEnableVertexAttribArrayARB)(GLuint index);
typedef void(APIENTRY* PFnglDisableVertexAttribArrayARB)(GLuint index);
typedef void(APIENTRY* PFnglBindAttribLocationARB)(GLhandleARB programObj, GLuint index, const GLcharARB* name);
typedef void(APIENTRY* PFnglGetActiveAttribARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei* length,
                                                GLint* size, GLenum* type, GLcharARB* name);
typedef GLint(APIENTRY* PFnglGetAttribLocationARB)(GLhandleARB programObj, const GLcharARB* name);
typedef void(APIENTRY* PFnglGetVertexAttribdvARB)(GLuint index, GLenum pname, GLdouble* params);
typedef void(APIENTRY* PFnglGetVertexAttribfvARB)(GLuint index, GLenum pname, GLfloat* params);
typedef void(APIENTRY* PFnglGetVertexAttribivARB)(GLuint index, GLenum pname, GLint* params);
typedef void(APIENTRY* PFnglGetVertexAttribPointervARB)(GLuint index, GLenum pname, GLvoid** pointer);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glVertexAttribPointerARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glEnableVertexAttribArrayARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glDisableVertexAttribArrayARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glBindAttribLocationARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetActiveAttribARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetAttribLocationARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetVertexAttribdvARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetVertexAttribfvARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetVertexAttribivARB);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetVertexAttribPointervARB);

// GL_EXT_abgr
#define GL_ABGR_EXT 0x8000

// GL_EXT_bgra
#define GL_BGR_EXT 0x80E0
#define GL_BGRA_EXT 0x80E1

// GL_EXT_draw_range_elements
#define GL_MAX_ELEMENTS_VERTICES_EXT 0x80E8
#define GL_MAX_ELEMENTS_INDICES_EXT 0x80E9
typedef void(APIENTRY* PFnglDrawRangeElementsEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type,
                                                  const GLvoid* indices);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glDrawRangeElementsEXT);

// GL_EXT_framebuffer_object
#define GL_FRAMEBUFFER_EXT 0x8D40
#define GL_RENDERBUFFER_EXT 0x8D41
#define GL_STENCIL_INDEX_EXT 0x8D45
#define GL_STENCIL_INDEX1_EXT 0x8D46
#define GL_STENCIL_INDEX4_EXT 0x8D47
#define GL_STENCIL_INDEX8_EXT 0x8D48
#define GL_STENCIL_INDEX16_EXT 0x8D49
#define GL_RENDERBUFFER_WIDTH_EXT 0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT 0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT 0x8D44
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT 0x8CD4
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT 0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT 0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT 0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT 0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT 0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT 0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT 0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT 0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT 0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT 0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT 0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT 0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT 0x8CED
#define GL_COLOR_ATTACHMENT14_EXT 0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT 0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#define GL_STENCIL_ATTACHMENT_EXT 0x8D20
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT 0x8CD8
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT 0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT 0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT 0x8CDD
#define GL_FRAMEBUFFER_STATUS_ERROR_EXT 0x8CDE
#define GL_FRAMEBUFFER_BINDING_EXT 0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT 0x8CA7
#define GL_MAX_COLOR_ATTACHMENTS_EXT 0x8CDF
#define GL_MAX_RENDERBUFFER_SIZE_EXT 0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT 0x0506
typedef GLboolean(APIENTRY* PFnglIsRenderbufferEXT)(GLuint renderbuffer);
typedef void(APIENTRY* PFnglBindRenderbufferEXT)(GLenum target, GLuint renderbuffer);
typedef void(APIENTRY* PFnglDeleteRenderbuffersEXT)(GLsizei n, const GLuint* renderbuffers);
typedef void(APIENTRY* PFnglGenRenderbuffersEXT)(GLsizei n, GLuint* renderbuffers);
typedef void(APIENTRY* PFnglRenderbufferStorageEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void(APIENTRY* PFnglGetRenderbufferParameterivEXT)(GLenum target, GLenum pname, GLint* params);
typedef GLboolean(APIENTRY* PFnglIsFramebufferEXT)(GLuint framebuffer);
typedef void(APIENTRY* PFnglBindFramebufferEXT)(GLenum target, GLuint framebuffer);
typedef void(APIENTRY* PFnglDeleteFramebuffersEXT)(GLsizei n, const GLuint* framebuffers);
typedef void(APIENTRY* PFnglGenFramebuffersEXT)(GLsizei n, GLuint* framebuffers);
typedef GLenum(APIENTRY* PFnglCheckFramebufferStatusEXT)(GLenum target);
typedef void(APIENTRY* PFnglFramebufferTexture1DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
                                                     GLint level);
typedef void(APIENTRY* PFnglFramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
                                                     GLint level);
typedef void(APIENTRY* PFnglFramebufferTexture3DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
                                                     GLint level, GLint zoffset);
typedef void(APIENTRY* PFnglFramebufferRenderbufferEXT)(GLenum target, GLenum attachment, GLenum renderbuffertarget,
                                                        GLuint renderbuffer);
typedef void(APIENTRY* PFnglGetFramebufferAttachmentParameterivEXT)(GLenum target, GLenum attachment, GLenum pname,
                                                                    GLint* params);
typedef void(APIENTRY* PFnglGenerateMipmapEXT)(GLenum target);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glIsRenderbufferEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glBindRenderbufferEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glDeleteRenderbuffersEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGenRenderbuffersEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glRenderbufferStorageEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetRenderbufferParameterivEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glIsFramebufferEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glBindFramebufferEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glDeleteFramebuffersEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGenFramebuffersEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glCheckFramebufferStatusEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glFramebufferTexture1DEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glFramebufferTexture2DEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glFramebufferTexture3DEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glFramebufferRenderbufferEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGetFramebufferAttachmentParameterivEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glGenerateMipmapEXT);

// GL_EXT_packed_depth_stencil
#define GL_DEPTH_STENCIL_EXT 0x84F9
#define GL_UNSIGNED_INT_24_8_EXT 0x84FA
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT 0x88F1

// GL_EXT_stencil_two_side
#define GL_STENCIL_TEST_TWO_SIDE_EXT 0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT 0x8911
typedef void(APIENTRY* PFnglActiveStencilFaceEXT)(GLenum face);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glActiveStencilFaceEXT);

// GL_EXT_stencil_wrap
#define GL_INCR_WRAP_EXT 0x8507
#define GL_DECR_WRAP_EXT 0x8508

// GL_EXT_texture_sRGB
#define GL_SRGB_EXT 0x8C40
#define GL_SRGB8_EXT 0x8C41
#define GL_SRGB_ALPHA_EXT 0x8C42
#define GL_SRGB8_ALPHA8_EXT 0x8C43
#define GL_SLUMINANCE_ALPHA_EXT 0x8C44
#define GL_SLUMINANCE8_ALPHA8_EXT 0x8C45
#define GL_SLUMINANCE_EXT 0x8C46
#define GL_SLUMINANCE8_EXT 0x8C47
#define GL_COMPRESSED_SRGB_EXT 0x8C48
#define GL_COMPRESSED_SRGB_ALPHA_EXT 0x8C49
#define GL_COMPRESSED_SLUMINANCE_EXT 0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA_EXT 0x8C4B
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F

// GL_EXT_texture3D
#define GL_PACK_SKIP_IMAGES_EXT 0x806B
#define GL_PACK_IMAGE_HEIGHT_EXT 0x806C
#define GL_UNPACK_SKIP_IMAGES_EXT 0x806D
#define GL_UNPACK_IMAGE_HEIGHT_EXT 0x806E
#define GL_TEXTURE_3D_EXT 0x806F
#define GL_PROXY_TEXTURE_3D_EXT 0x8070
#define GL_TEXTURE_DEPTH_EXT 0x8071
#define GL_TEXTURE_WRAP_R_EXT 0x8072
#define GL_MAX_3D_TEXTURE_SIZE_EXT 0x8073
typedef void(APIENTRY* PFnglTexImage3DEXT)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height,
                                           GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
typedef void(APIENTRY* PFnglTexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                              GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
                                              const GLvoid* pixels);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glTexImage3DEXT);
CARBON_OPENGL_DECLARE_EXTENSION_FUNCTION(glTexSubImage3DEXT);

// GL_EXT_texture_compression_s3tc
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3

// GL_EXT_texture_edge_clamp
#define GL_CLAMP_TO_EDGE_EXT 0x812F

// GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

// GL_SGIS_texture_lod
#define GL_TEXTURE_MIN_LOD_SGIS 0x813A
#define GL_TEXTURE_MAX_LOD_SGIS 0x813B
#define GL_TEXTURE_BASE_LEVEL_SGIS 0x813C
#define GL_TEXTURE_MAX_LEVEL_SGIS 0x813D

}

}
