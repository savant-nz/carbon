/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGL41

#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Graphics/OpenGL41/OpenGL41.h"
#include "CarbonEngine/Graphics/OpenGL41/OpenGL41ShaderProgram.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Math/MathCommon.h"

#ifdef MACOSX
    #include <OpenGL/OpenGL.h>
#endif

namespace Carbon
{

bool OpenGL41::isSupported() const
{
#ifdef MACOSX
    // Check whether this OpenGL context was created with an OpenGL 4.1 core profile
    auto attributes = std::array<int, 14>{{kCGLPFAMinimumPolicy, kCGLPFAAccelerated, kCGLPFADoubleBuffer,
                                           kCGLPFAColorSize, 24, kCGLPFAAlphaSize, 8, kCGLPFADepthSize, 24,
                                           kCGLPFAStencilSize, 8, kCGLPFAOpenGLProfile, 0x4100, 0}};

    // Try and choose an OpenGL 4.1 pixel format
    auto pixelFormat = CGLPixelFormatObj();
    auto pixelFormatCount = GLint();
    CGLChoosePixelFormat(reinterpret_cast<const CGLPixelFormatAttribute*>(attributes.data()), &pixelFormat,
                         &pixelFormatCount);

    // If there is a pixel format available then OpenGL 4.1 is supported
    if (pixelFormat)
        return true;
#endif

    return false;
}

bool OpenGL41::setup()
{
    // Log details about the OpenGL drivers and hardware being used
    logString(GL_VENDOR, "OpenGL Vendor: ");
    logString(GL_RENDERER, "OpenGL Renderer: ");
    logString(GL_VERSION, "OpenGL Version: ");
    logString(GL_SHADING_LANGUAGE_VERSION, "OpenGL Shading Language Version: ");

    // Get OpenGL extensions
    auto extensions = Vector<UnicodeString>(glGetUnsignedInteger(GL_NUM_EXTENSIONS));
    for (auto i = 0U; i < extensions.size(); i++)
    {
        extensions[i] = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        CARBON_CHECK_OPENGL_ERROR(glGetStringi);
    }
    extensions.sort();

    // Write out the available extensions to the logfile
    Logfile::get().writeCollapsibleSection("OpenGL Extensions", extensions);

    // Check for extension availability
    extensions_.EXT_texture_compression_s3tc = extensions.has("GL_EXT_texture_compression_s3tc");
    extensions_.EXT_texture_filter_anisotropic = extensions.has("GL_EXT_texture_filter_anisotropic");

    // Store a few hardware limits
    textureUnitCount_ = glGetUnsignedInteger(GL_MAX_TEXTURE_IMAGE_UNITS);
    vertexAttributeCount_ = glGetUnsignedInteger(GL_MAX_VERTEX_ATTRIBS);
    vertexTextureUnitCount_ = glGetUnsignedInteger(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);
    maximumDrawBuffers_ = glGetUnsignedInteger(GL_MAX_DRAW_BUFFERS);

    // Flush active texture unit
    glActiveTexture(GL_TEXTURE0);
    CARBON_CHECK_OPENGL_ERROR(glActiveTexture);

    return OpenGLShared::setup();
}

void OpenGL41::shutdown()
{
    if (renderTargets_.size())
        LOG_WARNING << "There are unreleased render targets, this may cause a memory leak";
}

void OpenGL41::setDepthClearValue(float clearValue)
{
    glClearDepth(GLclampd(clearValue));
    CARBON_CHECK_OPENGL_ERROR(glClearDepth);
}

void OpenGL41::setMultisampleEnabled(bool enabled)
{
    setEnabled(GL_MULTISAMPLE, enabled);
}

bool OpenGL41::isStencilBufferSupported() const
{
    return true;
}

void OpenGL41::setStencilOperationsForFrontFaces(const States::StencilOperations& operations)
{
    glStencilOpSeparate(GL_FRONT, glStencilOpEnum[operations.getStencilTestFailOperation()],
                        glStencilOpEnum[operations.getDepthTestFailOperation()],
                        glStencilOpEnum[operations.getBothTestsPassOperation()]);
    CARBON_CHECK_OPENGL_ERROR(glStencilOpSeparate);
}

void OpenGL41::setStencilOperationsForBackFaces(const States::StencilOperations& operations)
{
    glStencilOpSeparate(GL_BACK, glStencilOpEnum[operations.getStencilTestFailOperation()],
                        glStencilOpEnum[operations.getDepthTestFailOperation()],
                        glStencilOpEnum[operations.getBothTestsPassOperation()]);
    CARBON_CHECK_OPENGL_ERROR(glStencilOpSeparate);
}

bool OpenGL41::isDepthClampSupported() const
{
    return true;
}

void OpenGL41::setDepthClampEnabled(bool enabled)
{
    setEnabled(GL_DEPTH_CLAMP, enabled);
}

bool OpenGL41::isShaderLanguageSupported(ShaderProgram::ShaderLanguage language) const
{
    return language == ShaderProgram::GLSL110 || language == ShaderProgram::GLSL410;
}

bool OpenGL41::isGeometryProgrammingSupported(ShaderProgram::ShaderLanguage language) const
{
    return language == ShaderProgram::GLSL410;
}

unsigned int OpenGL41::getVertexShaderTextureUnitCount(ShaderProgram::ShaderLanguage language) const
{
    if (language == ShaderProgram::GLSL110 || language == ShaderProgram::GLSL410)
        return vertexTextureUnitCount_;

    return 0;
}

ShaderProgram* OpenGL41::createShaderProgram(ShaderProgram::ShaderLanguage language)
{
    if (language == ShaderProgram::GLSL110 || language == ShaderProgram::GLSL410)
        return new OpenGL41ShaderProgram(language);

    return nullptr;
}

void OpenGL41::deleteShaderProgram(ShaderProgram* program)
{
    delete program;
}

void OpenGL41::setShaderProgram(ShaderProgram* program)
{
    if (program)
        glUseProgram(static_cast<OpenGL41ShaderProgram*>(program)->getProgram());
    else
        glUseProgram(0);

    CARBON_CHECK_OPENGL_ERROR(glUseProgram);
}

bool OpenGL41::isPrimitiveTypeSupported(PrimitiveType primitiveType) const
{
    return primitiveType == LineList || primitiveType == LineStrip || primitiveType == TriangleList ||
        primitiveType == TriangleStrip || primitiveType == TriangleListWithAdjacency ||
        primitiveType == TriangleStripWithAdjacency;
}

void OpenGL41::drawIndexedPrimitives(PrimitiveType primitiveType, unsigned int lowestIndex, unsigned int highestIndex,
                                     unsigned int indexCount, DataType indexDataType,
                                     DataBufferObject indexDataBufferObject, uintptr_t indexOffset)
{
    assert(indexDataBufferObject && "Index data buffer not specified");

    setIndexDataBuffer(reinterpret_cast<DataBuffer*>(indexDataBufferObject));

    glDrawRangeElements(glPrimitiveType[primitiveType], lowestIndex, highestIndex, indexCount,
                        glDataTypeEnum[indexDataType], reinterpret_cast<void*>(indexOffset));
    CARBON_CHECK_OPENGL_ERROR(glDrawRangeElements);

    GraphicsInterface::drawIndexedPrimitives(primitiveType, lowestIndex, highestIndex, indexCount, indexDataType,
                                             indexDataBufferObject, indexOffset);
}

void OpenGL41::copyBackbufferTo2DTexture(TextureObject texture, unsigned int mipmapLevel, const Rect& rect)
{
    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);
    States::RenderTarget.flush();

    glCopyTexSubImage2D(GL_TEXTURE_2D, mipmapLevel, 0, 0, GLint(rect.getLeft()), GLint(rect.getBottom()),
                        GLsizei(rect.getWidth()), GLsizei(rect.getHeight()));
    CARBON_CHECK_OPENGL_ERROR(glCopyTexSubImage2D);
}

}

#endif
