/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGLES2

#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2.h"
#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2Extensions.h"
#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2ShaderProgram.h"

namespace Carbon
{

bool OpenGLES2::setup()
{
    // Log details about the OpenGL drivers and hardware being used
    logString(GL_VENDOR, "OpenGL ES Vendor: ");
    logString(GL_RENDERER, "OpenGL ES Renderer: ");
    logString(GL_VERSION, "OpenGL ES Version: ");
    logString(GL_SHADING_LANGUAGE_VERSION, "OpenGL ES Shading Language Version: ");

    // Get OpenGL extensions
    auto extensions = Vector<UnicodeString>();
    auto glExtensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (glExtensions)
        extensions = UnicodeString(glExtensions).getTokens().sorted();

    // Write out the available extensions to the logfile
    Logfile::get().writeCollapsibleSection("OpenGL ES Extensions", extensions);

    // Map all extension functions
    OpenGLES2Extensions::mapFunctions();

    // Check for extension availability
    extensions_.APPLE_texture_format_BGRA8888 = extensions.has("GL_APPLE_texture_format_BGRA8888");
    extensions_.EXT_sRGB = extensions.has("GL_EXT_sRGB");
    extensions_.EXT_texture_compression_dxt1 = extensions.has("GL_EXT_texture_compression_dxt1");
    extensions_.IMG_texture_compression_pvrtc = extensions.has("GL_IMG_texture_compression_pvrtc");
    extensions_.OES_depth_texture = extensions.has("GL_OES_depth_texture");
    extensions_.OES_packed_depth_stencil = extensions.has("GL_OES_packed_depth_stencil");
    extensions_.OES_vertex_array_object = extensions.has("GL_OES_vertex_array_object");

    // Store a few hardware limits
    textureUnitCount_ = glGetUnsignedInteger(GL_MAX_TEXTURE_IMAGE_UNITS);
    vertexAttributeCount_ = glGetUnsignedInteger(GL_MAX_VERTEX_ATTRIBS);

    // Flush active texture unit
    glActiveTexture(GL_TEXTURE0);
    CARBON_CHECK_OPENGL_ERROR(glActiveTexture);

    return OpenGLShared::setup();
}

void OpenGLES2::disableUnusedCachedStates()
{
    States::StateCacher::disable(States::MultisampleEnabled);

    OpenGLShared::disableUnusedCachedStates();
}

void OpenGLES2::setDepthClearValue(float clearValue)
{
    glClearDepthf(clearValue);
    CARBON_CHECK_OPENGL_ERROR(glClearDepthf);
}

bool OpenGLES2::isStencilBufferSupported() const
{
    return true;
}

void OpenGLES2::setStencilOperationsForFrontFaces(const States::StencilOperations& operations)
{
    glStencilOpSeparate(GL_FRONT, glStencilOpEnum[operations.getStencilTestFailOperation()],
                        glStencilOpEnum[operations.getDepthTestFailOperation()],
                        glStencilOpEnum[operations.getBothTestsPassOperation()]);
    CARBON_CHECK_OPENGL_ERROR(glStencilOpSeparate);
}

void OpenGLES2::setStencilOperationsForBackFaces(const States::StencilOperations& operations)
{
    glStencilOpSeparate(GL_BACK, glStencilOpEnum[operations.getStencilTestFailOperation()],
                        glStencilOpEnum[operations.getDepthTestFailOperation()],
                        glStencilOpEnum[operations.getBothTestsPassOperation()]);
    CARBON_CHECK_OPENGL_ERROR(glStencilOpSeparate);
}

bool OpenGLES2::isShaderLanguageSupported(ShaderProgram::ShaderLanguage language) const
{
    return language == ShaderProgram::GLSL110;
}

ShaderProgram* OpenGLES2::createShaderProgram(ShaderProgram::ShaderLanguage language)
{
    if (language == ShaderProgram::GLSL110)
        return new OpenGLES2ShaderProgram;

    return nullptr;
}

void OpenGLES2::deleteShaderProgram(ShaderProgram* program)
{
    delete program;
}

void OpenGLES2::setShaderProgram(ShaderProgram* program)
{
    glUseProgram(program ? static_cast<OpenGLES2ShaderProgram*>(program)->getProgram() : 0);
    CARBON_CHECK_OPENGL_ERROR(glUseProgram);
}

void OpenGLES2::drawIndexedPrimitives(PrimitiveType primitiveType, unsigned int lowestIndex, unsigned int highestIndex,
                                      unsigned int indexCount, DataType indexDataType,
                                      DataBufferObject indexDataBufferObject, uintptr_t indexOffset)
{
    assert(indexDataBufferObject && "Index data buffer not specified");

    setIndexDataBuffer(reinterpret_cast<DataBuffer*>(indexDataBufferObject));

    glDrawElements(glPrimitiveType[primitiveType], indexCount, glDataTypeEnum[indexDataType],
                   reinterpret_cast<void*>(indexOffset));
    CARBON_CHECK_OPENGL_ERROR(glDrawElements);

    GraphicsInterface::drawIndexedPrimitives(primitiveType, lowestIndex, highestIndex, indexCount, indexDataType,
                                             indexDataBufferObject, indexOffset);
}

void OpenGLES2::copyBackbufferTo2DTexture(TextureObject texture, unsigned int mipmapLevel, const Rect& rect)
{
    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);
    States::RenderTarget.flush();

    glCopyTexSubImage2D(GL_TEXTURE_2D, mipmapLevel, 0, 0, GLint(rect.getLeft()), GLint(rect.getBottom()),
                        GLsizei(rect.getWidth()), GLsizei(rect.getHeight()));
    CARBON_CHECK_OPENGL_ERROR(glCopyTexSubImage2D);
}

}

#endif
