/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OPENGL41

#include "CarbonEngine/Graphics/OpenGL41/OpenGL41.h"
#include "CarbonEngine/Graphics/States/States.h"

namespace Carbon
{

GraphicsInterface::DataBufferObject OpenGL41::createDataBuffer()
{
    auto glBuffer = GLuint();
    glGenBuffers(1, &glBuffer);
    CARBON_CHECK_OPENGL_ERROR(glGenBuffers);

    return new DataBuffer(glBuffer);
}

void OpenGL41::deleteDataBuffer(DataBufferObject dataBufferObject)
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

    glDeleteBuffers(1, &dataBuffer->glBuffer);
    CARBON_CHECK_OPENGL_ERROR(glDeleteBuffers);

    delete dataBuffer;
    dataBuffer = nullptr;
}

bool OpenGL41::uploadStaticDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                      const byte_t* data)
{
    auto dataBuffer = reinterpret_cast<DataBuffer*>(dataBufferObject);

    dataBuffer->size = size;
    dataBuffer->isDynamic = false;

    return updateDataBuffer(dataBufferObject, type, data);
}

bool OpenGL41::uploadDynamicDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, unsigned int size,
                                       const byte_t* data)
{
    auto dataBuffer = reinterpret_cast<DataBuffer*>(dataBufferObject);

    dataBuffer->size = size;
    dataBuffer->isDynamic = true;

    return updateDataBuffer(dataBufferObject, type, data);
}

bool OpenGL41::updateDataBuffer(DataBufferObject dataBufferObject, DataBufferType type, const byte_t* data)
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

void OpenGL41::setVertexDataBuffer(const DataBuffer* dataBuffer)
{
    if (activeVertexDataBuffer_ == dataBuffer)
        return;

    glBindBuffer(GL_ARRAY_BUFFER, dataBuffer ? dataBuffer->glBuffer : 0);
    CARBON_CHECK_OPENGL_ERROR(glBindBuffer);

    activeVertexDataBuffer_ = dataBuffer;
}

void OpenGL41::setIndexDataBuffer(const DataBuffer* dataBuffer)
{
    if (activeIndexDataBuffer_[States::VertexAttributeArrayConfiguration.getCurrentGraphicsInterfaceValue()] == dataBuffer)
        return;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dataBuffer ? dataBuffer->glBuffer : 0);
    CARBON_CHECK_OPENGL_ERROR(glBindBuffer);

    activeIndexDataBuffer_[States::VertexAttributeArrayConfiguration.getCurrentGraphicsInterfaceValue()] = dataBuffer;
}

unsigned int OpenGL41::getVertexAttributeArrayCount() const
{
    return vertexAttributeCount_;
}

bool OpenGL41::isVertexAttribtuteArrayConfigurationSupported() const
{
    return true;
}

GraphicsInterface::VertexAttributeArrayConfigurationObject
    OpenGL41::createVertexAttributeArrayConfiguration(const Vector<ArraySource>& sources)
{
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

void OpenGL41::deleteVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration)
{
    auto glVertexArray = GLuint(uintptr_t(configuration));

    glDeleteVertexArrays(1, &glVertexArray);
    CARBON_CHECK_OPENGL_ERROR(glDeleteVertexArrays);

    States::VertexAttributeArrayConfiguration.onGraphicsInterfaceObjectDelete(configuration);

    activeIndexDataBuffer_[configuration] = nullptr;
}

void OpenGL41::setVertexAttributeArrayConfiguration(VertexAttributeArrayConfigurationObject configuration)
{
    glBindVertexArray(GLuint(uintptr_t(configuration)));
    CARBON_CHECK_OPENGL_ERROR(glBindVertexArray);
}

}

#endif
