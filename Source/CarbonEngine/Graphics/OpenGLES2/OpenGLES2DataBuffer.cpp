/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGLES2

#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2.h"
#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2Extensions.h"
#include "CarbonEngine/Graphics/States/States.h"

namespace Carbon
{

using namespace OpenGLES2Extensions;

GraphicsInterface::DataBufferObject OpenGLES2::createDataBuffer()
{
    auto glBuffer = GLuint();
    glGenBuffers(1, &glBuffer);
    CARBON_CHECK_OPENGL_ERROR(glGenBuffers);

    return new DataBuffer(glBuffer);
}

void OpenGLES2::deleteDataBuffer(DataBufferObject dataBufferObject)
{
    if (!dataBufferObject)
        return;

    auto dataBuffer = reinterpret_cast<DataBuffer*>(dataBufferObject);

    // Flush the data buffer out of the cache
    if (activeVertexDataBuffer_ == dataBuffer)
        setVertexDataBuffer(nullptr);

    for (auto& activeIndexDataBuffer : activeIndexDataBuffer_)
    {
        if (activeIndexDataBuffer.second == dataBuffer)
            activeIndexDataBuffer.second = nullptr;
    }

    glDeleteBuffers(1, &dataBuffer->glBuffer);
    CARBON_CHECK_OPENGL_ERROR(glDeleteBuffers);

    delete dataBuffer;
    dataBuffer = nullptr;
}

bool OpenGLES2::uploadStaticDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                       const byte_t* data)
{
    auto dataBuffer = reinterpret_cast<DataBuffer*>(dataBufferObject);

    dataBuffer->size = size;
    dataBuffer->isDynamic = false;

    return updateDataBuffer(dataBufferObject, type, data);
}

bool OpenGLES2::uploadDynamicDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                        const byte_t* data)
{
    auto dataBuffer = reinterpret_cast<DataBuffer*>(dataBufferObject);

    dataBuffer->size = size;
    dataBuffer->isDynamic = true;

    return updateDataBuffer(dataBufferObject, type, data);
}

bool OpenGLES2::updateDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, const byte_t* data)
{
    auto dataBuffer = reinterpret_cast<DataBuffer*>(dataBufferObject);

    if (type == VertexDataBuffer)
        setVertexDataBuffer(dataBuffer);
    else if (type == IndexDataBuffer)
        setIndexDataBuffer(dataBuffer);

    // Put in the new data, replacing the whole buffer is generally faster than using glBufferSubData() or glMapBuffer()
    glBufferData(glBufferTypeEnum[type], dataBuffer->size, data, dataBuffer->isDynamic ? GL_STREAM_DRAW : GL_STATIC_DRAW);
    CARBON_CHECK_OPENGL_ERROR(glBufferData);

    return true;
}

void OpenGLES2::setVertexDataBuffer(const DataBuffer* dataBuffer)
{
    if (activeVertexDataBuffer_ == dataBuffer)
        return;

    glBindBuffer(GL_ARRAY_BUFFER, dataBuffer ? dataBuffer->glBuffer : 0);
    CARBON_CHECK_OPENGL_ERROR(glBindBuffer);

    activeVertexDataBuffer_ = dataBuffer;
}

void OpenGLES2::setIndexDataBuffer(const DataBuffer* dataBuffer)
{
    if (activeIndexDataBuffer_[States::VertexAttributeArrayConfiguration.getCurrentGraphicsInterfaceValue()] == dataBuffer)
        return;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dataBuffer ? dataBuffer->glBuffer : 0);
    CARBON_CHECK_OPENGL_ERROR(glBindBuffer);

    activeIndexDataBuffer_[States::VertexAttributeArrayConfiguration.getCurrentGraphicsInterfaceValue()] = dataBuffer;
}

unsigned int OpenGLES2::getVertexAttributeArrayCount() const
{
    return vertexAttributeCount_;
}

bool OpenGLES2::setVertexAttributeArrayEnabled(unsigned int attributeIndex, bool enabled)
{
    if (extensions_.OES_vertex_array_object)
        return true;

    States::VertexAttributeArrayConfiguration.pushSetFlushPop(nullptr);

    if (enabled)
    {
        glEnableVertexAttribArray(attributeIndex);
        CARBON_CHECK_OPENGL_ERROR(glEnableVertexAttribArray);
    }
    else
    {
        glDisableVertexAttribArray(attributeIndex);
        CARBON_CHECK_OPENGL_ERROR(glDisableVertexAttribArray);
    }

    return true;
}

bool OpenGLES2::setVertexAttributeArraySource(unsigned int attributeIndex, const ArraySource& source)
{
    if (extensions_.OES_vertex_array_object)
        return true;

    if (!States::VertexAttributeArrayEnabled[attributeIndex])
        return false;

    States::VertexAttributeArrayConfiguration.pushSetFlushPop(nullptr);

    setVertexDataBuffer(reinterpret_cast<DataBuffer*>(source.getDataBufferObject()));

    glVertexAttribPointer(attributeIndex, source.getComponentCount(), glDataTypeEnum[source.getDataType()],
                          source.getNormalizeFixedPoint(), source.getStride(), reinterpret_cast<void*>(source.getOffset()));
    CARBON_CHECK_OPENGL_ERROR(glVertexAttribPointer);

    return true;
}

bool OpenGLES2::isVertexAttribtuteArrayConfigurationSupported() const
{
    return extensions_.OES_vertex_array_object;
}

GraphicsInterface::VertexAttributeArrayConfigurationObject
    OpenGLES2::createVertexAttributeArrayConfiguration(const Vector<ArraySource>& sources)
{
    if (!extensions_.OES_vertex_array_object)
        return nullptr;

    // Create a new VAO
    auto glVertexArray = GLuint();
    glGenVertexArraysOES(1, &glVertexArray);
    CARBON_CHECK_OPENGL_ERROR(glGenVertexArraysOES);

    auto configuration = VertexAttributeArrayConfigurationObject(uintptr_t(glVertexArray));

    // Bind the new VAO
    States::VertexAttributeArrayConfiguration.pushSetFlushPop(configuration);

    // Setup the new VAO's state
    for (auto i = 0U; i < sources.size(); i++)
    {
        const auto& source = sources[i];

        if (!source.isValid())
            continue;

        setVertexDataBuffer(reinterpret_cast<DataBuffer*>(source.getDataBufferObject()));

        glEnableVertexAttribArray(i);
        CARBON_CHECK_OPENGL_ERROR(glEnableVertexAttribArray);

        glVertexAttribPointer(i, source.getComponentCount(), glDataTypeEnum[source.getDataType()],
                              source.getNormalizeFixedPoint(), source.getStride(), reinterpret_cast<void*>(source.getOffset()));
        CARBON_CHECK_OPENGL_ERROR(glVertexAttribPointer);
    }

    return configuration;
}

void OpenGLES2::deleteVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration)
{
    if (!extensions_.OES_vertex_array_object)
        return;

    auto glVertexArray = GLuint(uintptr_t(configuration));

    glDeleteVertexArraysOES(1, &glVertexArray);
    CARBON_CHECK_OPENGL_ERROR(glDeleteVertexArraysOES);

    States::VertexAttributeArrayConfiguration.onGraphicsInterfaceObjectDelete(configuration);

    activeIndexDataBuffer_[configuration] = nullptr;
}

void OpenGLES2::setVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration)
{
    if (!extensions_.OES_vertex_array_object)
        return;

    glBindVertexArrayOES(GLuint(uintptr_t(configuration)));
    CARBON_CHECK_OPENGL_ERROR(glBindVertexArrayOES);
}

}

#endif
