/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGL11

#include "CarbonEngine/Graphics/OpenGL11/OpenGL11.h"
#include "CarbonEngine/Graphics/States/States.h"

namespace Carbon
{

using namespace OpenGL11Extensions;

GraphicsInterface::DataBufferObject OpenGL11::createDataBuffer()
{
    auto glBuffer = GLuint();
    glGenBuffersARB(1, &glBuffer);
    CARBON_CHECK_OPENGL_ERROR(glGenBuffersARB);

    return new DataBuffer(glBuffer);
}

void OpenGL11::deleteDataBuffer(DataBufferObject dataBufferObject)
{
    if (!dataBufferObject)
        return;

    auto dataBuffer = reinterpret_cast<DataBuffer*>(dataBufferObject);

    // Flush the data buffer out of the cache
    if (activeVertexDataBuffer_ == dataBuffer)
        activeVertexDataBuffer_ = nullptr;

    for (auto& activeIndexDataBuffer : activeIndexDataBuffer_)
    {
        if (activeIndexDataBuffer.second == dataBuffer)
            activeIndexDataBuffer.second = nullptr;
    }

    glDeleteBuffersARB(1, &dataBuffer->glBuffer);
    CARBON_CHECK_OPENGL_ERROR(glDeleteBuffersARB);

    delete dataBuffer;
    dataBuffer = nullptr;
}

bool OpenGL11::uploadStaticDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                      const byte_t* data)
{
    auto dataBuffer = reinterpret_cast<DataBuffer*>(dataBufferObject);

    dataBuffer->size = size;
    dataBuffer->isDynamic = false;

    return updateDataBuffer(dataBufferObject, type, data);
}

bool OpenGL11::uploadDynamicDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                       const byte_t* data)
{
    auto dataBuffer = reinterpret_cast<DataBuffer*>(dataBufferObject);

    dataBuffer->size = size;
    dataBuffer->isDynamic = true;

    return updateDataBuffer(dataBufferObject, type, data);
}

bool OpenGL11::updateDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, const byte_t* data)
{
    auto dataBuffer = reinterpret_cast<DataBuffer*>(dataBufferObject);

    if (type == VertexDataBuffer)
        setVertexDataBuffer(dataBuffer);
    else if (type == IndexDataBuffer)
        setIndexDataBuffer(dataBuffer);

    // Put in the new data, replacing the whole buffer is generally faster than using glBufferSubDataARB() or
    // glMapBufferARB()
    glBufferDataARB(glBufferTypeEnum[type], dataBuffer->size, data,
                    dataBuffer->isDynamic ? GL_STREAM_DRAW_ARB : GL_STATIC_DRAW_ARB);
    CARBON_CHECK_OPENGL_ERROR(glBufferDataARB);

    return true;
}

void OpenGL11::setVertexDataBuffer(const DataBuffer* dataBuffer)
{
    if (activeVertexDataBuffer_ == dataBuffer)
        return;

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, dataBuffer ? dataBuffer->glBuffer : 0);
    CARBON_CHECK_OPENGL_ERROR(glBindBufferARB);

    activeVertexDataBuffer_ = dataBuffer;
}

void OpenGL11::setIndexDataBuffer(const DataBuffer* dataBuffer)
{
    if (activeIndexDataBuffer_[States::VertexAttributeArrayConfiguration.getCurrentGraphicsInterfaceValue()] ==
        dataBuffer)
        return;

    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, dataBuffer ? dataBuffer->glBuffer : 0);
    CARBON_CHECK_OPENGL_ERROR(glBindBufferARB);

    activeIndexDataBuffer_[States::VertexAttributeArrayConfiguration.getCurrentGraphicsInterfaceValue()] = dataBuffer;
}

unsigned int OpenGL11::getVertexAttributeArrayCount() const
{
    return vertexAttributeCount_;
}

bool OpenGL11::setVertexAttributeArrayEnabled(unsigned int attributeIndex, bool enabled)
{
    if (extensions_.ARB_vertex_array_object)
        return true;

    States::VertexAttributeArrayConfiguration.pushSetFlushPop(nullptr);

    if (enabled)
    {
        glEnableVertexAttribArrayARB(attributeIndex);
        CARBON_CHECK_OPENGL_ERROR(glEnableVertexAttribArrayARB);
    }
    else
    {
        glDisableVertexAttribArrayARB(attributeIndex);
        CARBON_CHECK_OPENGL_ERROR(glDisableVertexAttribArrayARB);
    }

    return true;
}

bool OpenGL11::setVertexAttributeArraySource(unsigned int attributeIndex, const ArraySource& source)
{
    if (extensions_.ARB_vertex_array_object)
        return true;

    if (!States::VertexAttributeArrayEnabled[attributeIndex])
        return false;

    States::VertexAttributeArrayConfiguration.pushSetFlushPop(nullptr);

    setVertexDataBuffer(reinterpret_cast<DataBuffer*>(source.getDataBufferObject()));

    glVertexAttribPointerARB(attributeIndex, source.getComponentCount(), glDataTypeEnum[source.getDataType()],
                             source.getNormalizeFixedPoint(), source.getStride(),
                             reinterpret_cast<void*>(source.getOffset()));
    CARBON_CHECK_OPENGL_ERROR(glVertexAttribPointerARB);

    return true;
}

bool OpenGL11::isVertexAttribtuteArrayConfigurationSupported() const
{
    return extensions_.ARB_vertex_array_object;
}

GraphicsInterface::VertexAttributeArrayConfigurationObject
    OpenGL11::createVertexAttributeArrayConfiguration(const Vector<ArraySource>& sources)
{
    if (!extensions_.ARB_vertex_array_object)
        return nullptr;

    // Create a new VAO
    auto glVertexArray = GLuint();
    glGenVertexArrays(1, &glVertexArray);
    CARBON_CHECK_OPENGL_ERROR(glGenVertexArrays);

    auto configuration = VertexAttributeArrayConfigurationObject(uintptr_t(glVertexArray));

    // Bind the new VAO
    States::VertexAttributeArrayConfiguration.pushSetFlushPop(configuration);

    // Setup the new VAO's state
    for (auto i = 0U; i < sources.size(); i++)
    {
        auto& source = sources[i];

        if (!source.isValid())
            continue;

        setVertexDataBuffer(reinterpret_cast<DataBuffer*>(source.getDataBufferObject()));

        glEnableVertexAttribArrayARB(i);
        CARBON_CHECK_OPENGL_ERROR(glEnableVertexAttribArrayARB);

        glVertexAttribPointerARB(i, source.getComponentCount(), glDataTypeEnum[source.getDataType()],
                                 source.getNormalizeFixedPoint(), source.getStride(),
                                 reinterpret_cast<void*>(source.getOffset()));
        CARBON_CHECK_OPENGL_ERROR(glVertexAttribPointerARB);
    }

    return configuration;
}

void OpenGL11::deleteVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration)
{
    if (!extensions_.ARB_vertex_array_object)
        return;

    auto glVertexArray = GLuint(uintptr_t(configuration));

    glDeleteVertexArrays(1, &glVertexArray);
    CARBON_CHECK_OPENGL_ERROR(glDeleteVertexArrays);

    States::VertexAttributeArrayConfiguration.onGraphicsInterfaceObjectDelete(configuration);

    activeIndexDataBuffer_[configuration] = nullptr;
}

void OpenGL11::setVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration)
{
    if (!extensions_.ARB_vertex_array_object)
        return;

    glBindVertexArray(GLuint(uintptr_t(configuration)));
    CARBON_CHECK_OPENGL_ERROR(glBindVertexArray);
}

}

#endif
