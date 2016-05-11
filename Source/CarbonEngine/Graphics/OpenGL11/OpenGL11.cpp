/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGL11

#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Graphics/OpenGL11/OpenGL11.h"
#include "CarbonEngine/Graphics/OpenGL11/OpenGL11ShaderProgram.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

using namespace OpenGL11Extensions;

bool OpenGL11::setup()
{
    // Log details about the OpenGL drivers and hardware being used
    logString(GL_VENDOR, "OpenGL Vendor: ");
    logString(GL_RENDERER, "OpenGL Renderer: ");
    logString(GL_VERSION, "OpenGL Version: ");
    logString(GL_SHADING_LANGUAGE_VERSION_ARB, "OpenGL Shading Language Version: ");

    // Get OpenGL extensions
    auto extensions = Vector<UnicodeString>();
    auto glExtensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    CARBON_CHECK_OPENGL_ERROR(glGetString);
    if (glExtensions)
        extensions = UnicodeString(glExtensions).getTokens().sorted();

    // Write out the available extensions to the logfile
    Logfile::get().writeCollapsibleSection("OpenGL Extensions", extensions);

    // Support for GLSL is required
    if (!extensions.has("GL_ARB_vertex_shader") || !extensions.has("GL_ARB_fragment_shader") ||
        !extensions.has("GL_ARB_shading_language_100") || !extensions.has("GL_ARB_shader_objects"))
    {
        LOG_ERROR << "This hardware does not support the OpenGL Shading Language";
        return false;
    }

    // Support for VBOs is required
    if (!extensions.has("GL_ARB_vertex_buffer_object"))
    {
        LOG_ERROR << "This hardware does not support vertex buffer objects";
        return false;
    }

    // Support for cube maps is required
    if (!extensions.has("GL_ARB_texture_cube_map"))
    {
        LOG_ERROR << "This hardware does not support cube map textures";
        return false;
    }

    // Support for edge clamping is required
    if (!extensions.has("GL_EXT_texture_edge_clamp") && !extensions.has("GL_SGIS_texture_edge_clamp"))
    {
        LOG_ERROR << "This hardware does not support texture edge clamping";
        return false;
    }

    // Map all extension functions
    mapFunctions();

    // Check for extension availability
    extensions_.ARB_depth_clamp = extensions.has("GL_ARB_depth_clamp");
    extensions_.ARB_depth_texture = extensions.has("GL_ARB_depth_texture");
    extensions_.ARB_draw_buffers = extensions.has("GL_ARB_draw_buffers");
    extensions_.ARB_half_float_pixel = extensions.has("GL_ARB_half_float_pixel");
    extensions_.ARB_shadow = extensions.has("GL_ARB_shadow");
    extensions_.ARB_texture_float = extensions.has("GL_ARB_texture_float");
    extensions_.ARB_texture_non_power_of_two = extensions.has("GL_ARB_texture_non_power_of_two");
    extensions_.ARB_texture_rg = extensions.has("GL_ARB_texture_rg");
    extensions_.ARB_vertex_array_object = extensions.has("GL_ARB_vertex_array_object");
    extensions_.EXT_abgr = extensions.has("GL_EXT_abgr");
    extensions_.EXT_bgra = extensions.has("GL_EXT_bgra");
    extensions_.EXT_draw_range_elements = extensions.has("GL_EXT_draw_range_elements");
    extensions_.EXT_framebuffer_object = extensions.has("GL_EXT_framebuffer_object");
    extensions_.EXT_packed_depth_stencil = extensions.has("GL_EXT_packed_depth_stencil");
    extensions_.EXT_stencil_two_side = extensions.has("GL_EXT_stencil_two_side");
    extensions_.EXT_stencil_wrap = extensions.has("GL_EXT_stencil_wrap");
    extensions_.EXT_texture_3D = extensions.has("GL_EXT_texture_3D");
    extensions_.EXT_texture_compression_s3tc = extensions.has("GL_EXT_texture_compression_s3tc");
    extensions_.EXT_texture_filter_anisotropic = extensions.has("GL_EXT_texture_filter_anisotropic");
    extensions_.EXT_texture_sRGB = extensions.has("GL_EXT_texture_sRGB");
    extensions_.SGIS_texture_lod = extensions.has("GL_SGIS_texture_lod");

    // Treat GL_APPLE_vertex_array_object as if it is GL_ARB_vertex_array_object
    if (!extensions_.ARB_vertex_array_object && extensions.has("GL_APPLE_vertex_array_object"))
    {
        extensions_.ARB_vertex_array_object = true;

        glBindVertexArray = glBindVertexArrayAPPLE;
        glDeleteVertexArrays = glDeleteVertexArraysAPPLE;
        glGenVertexArrays = glGenVertexArraysAPPLE;
        glIsVertexArray = glIsVertexArrayAPPLE;
    }

    // Store a few hardware limits
    textureUnitCount_ = glGetUnsignedInteger(GL_MAX_TEXTURE_IMAGE_UNITS_ARB);
    vertexAttributeCount_ = glGetUnsignedInteger(GL_MAX_VERTEX_ATTRIBS_ARB);
    vertexTextureUnitCount_ = glGetUnsignedInteger(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB);
    maximumDrawBuffers_ = extensions_.ARB_draw_buffers ? glGetUnsignedInteger(GL_MAX_DRAW_BUFFERS_ARB) : 1;

    // Flush active texture unit
    glActiveTextureARB(GL_TEXTURE0_ARB);
    CARBON_CHECK_OPENGL_ERROR(glActiveTextureARB);

    return OpenGLShared::setup();
}

void OpenGL11::shutdown()
{
#ifdef CARBON_INCLUDE_OCULUSRIFT
    auto session = platform().getPlatformSpecificValue<ovrSession>(PlatformInterface::OculusRiftSession);

    for (auto& eye : oculusRiftEyes_)
    {
        if (eye.swapTextureSet)
        {
            ovr_DestroySwapTextureSet(session, eye.swapTextureSet);
            eye.swapTextureSet = nullptr;
        }

        if (eye.depthTexture)
        {
            if (eye.renderTarget)
                setRenderTargetDepthBufferTexture(eye.renderTarget, nullptr);

            deleteTexture(eye.depthTexture);
            eye.depthTexture = nullptr;
        }

        if (eye.renderTarget)
        {
            deleteRenderTarget(eye.renderTarget);
            eye.renderTarget = nullptr;
        }
    }
#endif

    if (renderTargets_.size())
        LOG_WARNING << "There are unreleased render targets, this may cause a memory leak";
}

void OpenGL11::setDepthClearValue(float clearValue)
{
    glClearDepth(GLclampd(clearValue));
    CARBON_CHECK_OPENGL_ERROR(glClearDepth);
}

void OpenGL11::setMultisampleEnabled(bool enabled)
{
    setEnabled(GL_MULTISAMPLE_ARB, enabled);
}

bool OpenGL11::isStencilBufferSupported() const
{
    // At present stencil is only advertised as supported if the hardware supports separate stencil modes for front and back
    // faces as well as stencil wrapping. More hardware could be supported if these requirements were lifted but there is no
    // need at present. Doing this would require additional hardware support methods on GraphicsInterface and users of
    // stenciling to be updated to check for the relevant hardware support.

    return extensions_.EXT_stencil_two_side && extensions_.EXT_stencil_wrap;
}

void OpenGL11::setStencilTestEnabled(bool enabled)
{
    if (!isStencilBufferSupported())
        return;

    setEnabled(GL_STENCIL_TEST, enabled);
    setEnabled(GL_STENCIL_TEST_TWO_SIDE_EXT, enabled);
}

void OpenGL11::setStencilTestFunction(const States::StencilTestSetup& function)
{
    if (!isStencilBufferSupported())
        return;

    glActiveStencilFaceEXT(GL_BACK);
    CARBON_CHECK_OPENGL_ERROR(glActiveStencilFaceEXT);
    OpenGLShared::setStencilTestFunction(function);

    glActiveStencilFaceEXT(GL_FRONT);
    CARBON_CHECK_OPENGL_ERROR(glActiveStencilFaceEXT);
    OpenGLShared::setStencilTestFunction(function);
}

void OpenGL11::setStencilWriteEnabled(bool enabled)
{
    if (!isStencilBufferSupported())
        return;

    glActiveStencilFaceEXT(GL_BACK);
    CARBON_CHECK_OPENGL_ERROR(glActiveStencilFaceEXT);
    OpenGLShared::setStencilWriteEnabled(enabled);

    glActiveStencilFaceEXT(GL_FRONT);
    CARBON_CHECK_OPENGL_ERROR(glActiveStencilFaceEXT);
    OpenGLShared::setStencilWriteEnabled(enabled);
}

void OpenGL11::setStencilOperationsForFrontFaces(const States::StencilOperations& operations)
{
    if (!isStencilBufferSupported())
        return;

    glActiveStencilFaceEXT(GL_FRONT);
    CARBON_CHECK_OPENGL_ERROR(glActiveStencilFaceEXT);

    glStencilOp(glStencilOpEnum[operations.getStencilTestFailOperation()],
                glStencilOpEnum[operations.getDepthTestFailOperation()],
                glStencilOpEnum[operations.getBothTestsPassOperation()]);
    CARBON_CHECK_OPENGL_ERROR(glStencilOp);
}

void OpenGL11::setStencilOperationsForBackFaces(const States::StencilOperations& operations)
{
    if (!isStencilBufferSupported())
        return;

    glActiveStencilFaceEXT(GL_BACK);
    CARBON_CHECK_OPENGL_ERROR(glActiveStencilFaceEXT);

    glStencilOp(glStencilOpEnum[operations.getStencilTestFailOperation()],
                glStencilOpEnum[operations.getDepthTestFailOperation()],
                glStencilOpEnum[operations.getBothTestsPassOperation()]);
    CARBON_CHECK_OPENGL_ERROR(glStencilOp);
}

bool OpenGL11::isDepthClampSupported() const
{
    return extensions_.ARB_depth_clamp;
}

void OpenGL11::setDepthClampEnabled(bool enabled)
{
    if (!extensions_.ARB_depth_clamp)
        return;

    setEnabled(GL_DEPTH_CLAMP_ARB, enabled);
}

bool OpenGL11::isShaderLanguageSupported(ShaderProgram::ShaderLanguage language) const
{
    return language == ShaderProgram::GLSL110;
}

unsigned int OpenGL11::getVertexShaderTextureUnitCount(ShaderProgram::ShaderLanguage language) const
{
    if (language == ShaderProgram::GLSL110)
        return vertexTextureUnitCount_;

    return 0;
}

ShaderProgram* OpenGL11::createShaderProgram(ShaderProgram::ShaderLanguage language)
{
    return language == ShaderProgram::GLSL110 ? new OpenGL11ShaderProgram : nullptr;
}

void OpenGL11::deleteShaderProgram(ShaderProgram* program)
{
    delete program;
}

void OpenGL11::setShaderProgram(ShaderProgram* program)
{
    if (!program)
        glUseProgramObjectARB(0);
    else
        glUseProgramObjectARB(static_cast<OpenGL11ShaderProgram*>(program)->getProgram());

    CARBON_CHECK_OPENGL_ERROR(glUseProgramObjectARB);
}

bool OpenGL11::isPrimitiveTypeSupported(PrimitiveType primitiveType) const
{
    return primitiveType == LineList || primitiveType == LineStrip || primitiveType == TriangleList ||
        primitiveType == TriangleStrip;
}

void OpenGL11::drawIndexedPrimitives(PrimitiveType primitiveType, unsigned int lowestIndex, unsigned int highestIndex,
                                     unsigned int indexCount, DataType indexDataType, DataBufferObject indexDataBufferObject,
                                     uintptr_t indexOffset)
{
    assert(indexDataBufferObject && "Index data buffer not specified");

    setIndexDataBuffer(reinterpret_cast<DataBuffer*>(indexDataBufferObject));

    if (extensions_.EXT_draw_range_elements)
    {
        glDrawRangeElementsEXT(glPrimitiveType[primitiveType], lowestIndex, highestIndex, indexCount,
                               glDataTypeEnum[indexDataType], reinterpret_cast<void*>(indexOffset));
        CARBON_CHECK_OPENGL_ERROR(glDrawRangeElementsEXT);
    }
    else
    {
        glDrawElements(glPrimitiveType[primitiveType], indexCount, glDataTypeEnum[indexDataType],
                       reinterpret_cast<void*>(indexOffset));
        CARBON_CHECK_OPENGL_ERROR(glDrawElements);
    }

    GraphicsInterface::drawIndexedPrimitives(primitiveType, lowestIndex, highestIndex, indexCount, indexDataType,
                                             indexDataBufferObject, indexOffset);
}

void OpenGL11::copyBackbufferTo2DTexture(TextureObject texture, unsigned int mipmapLevel, const Rect& rect)
{
    States::Texture[activeTextureUnit_].pushSetFlushPop(texture);
    States::RenderTarget.flush();

    glCopyTexSubImage2D(GL_TEXTURE_2D, mipmapLevel, 0, 0, GLint(rect.getLeft()), GLint(rect.getBottom()),
                        GLsizei(rect.getWidth()), GLsizei(rect.getHeight()));
    CARBON_CHECK_OPENGL_ERROR(glCopyTexSubImage2D);
}

}

#endif
