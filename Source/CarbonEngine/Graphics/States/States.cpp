/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Graphics/States/States.h"

namespace Carbon
{

namespace States
{

#define DEFINE_SIMPLE_CACHED_STATE(Name, Type, GraphicsInterfaceSetterParameterType, DefaultValue)     \
    CARBON_API SimpleCachedState<Type, GraphicsInterfaceSetterParameterType> Name(#Name, DefaultValue, \
                                                                                  &GraphicsInterface::set##Name);

#define DEFINE_INDEXED_CACHED_STATE(Name, Type, GraphicsInterfaceSetterParameterType, DefaultValue)     \
    CARBON_API IndexedCachedState<Type, GraphicsInterfaceSetterParameterType> Name(#Name, DefaultValue, \
                                                                                   &GraphicsInterface::set##Name);

DEFINE_SIMPLE_CACHED_STATE(BlendEnabled, bool, bool, false)
DEFINE_SIMPLE_CACHED_STATE(BlendFunction, BlendFunctionSetup, const BlendFunctionSetup&, BlendFunctionSetup())
DEFINE_SIMPLE_CACHED_STATE(ClearColor, Color, const Color&, Color(0.0f, 0.0f, 0.0f, 0.0f))
DEFINE_SIMPLE_CACHED_STATE(ColorWriteEnabled, bool, bool, true)
DEFINE_SIMPLE_CACHED_STATE(CullMode, CullingMode, CullingMode, CullBackFaces)
DEFINE_SIMPLE_CACHED_STATE(DepthClampEnabled, bool, bool, false)
DEFINE_SIMPLE_CACHED_STATE(DepthClearValue, float, float, 1.0f)
DEFINE_SIMPLE_CACHED_STATE(DepthCompareFunction, CompareFunction, CompareFunction, CompareLessEqual)
DEFINE_SIMPLE_CACHED_STATE(DepthTestEnabled, bool, bool, true)
DEFINE_SIMPLE_CACHED_STATE(DepthWriteEnabled, bool, bool, true)
DEFINE_SIMPLE_CACHED_STATE(MultisampleEnabled, bool, bool, false)
DEFINE_SIMPLE_CACHED_STATE(RenderTarget, GraphicsInterface::RenderTargetObject, GraphicsInterface::RenderTargetObject, nullptr)
DEFINE_SIMPLE_CACHED_STATE(ScissorEnabled, bool, bool, false)
DEFINE_SIMPLE_CACHED_STATE(ScissorRectangle, Rect, const Rect&, Rect(0.0f, 0.0f, 0.0f, 0.0f))
DEFINE_SIMPLE_CACHED_STATE(ShaderProgram, Carbon::ShaderProgram*, Carbon::ShaderProgram*, nullptr)
DEFINE_SIMPLE_CACHED_STATE(StencilClearValue, unsigned int, unsigned int, 0)
DEFINE_SIMPLE_CACHED_STATE(StencilOperationsForBackFaces, StencilOperations, const StencilOperations&, StencilOperations())
DEFINE_SIMPLE_CACHED_STATE(StencilOperationsForFrontFaces, StencilOperations, const StencilOperations&, StencilOperations())
DEFINE_SIMPLE_CACHED_STATE(StencilTestEnabled, bool, bool, false)
DEFINE_SIMPLE_CACHED_STATE(StencilTestFunction, StencilTestSetup, const StencilTestSetup&, StencilTestSetup())
DEFINE_SIMPLE_CACHED_STATE(StencilWriteEnabled, bool, bool, false)
DEFINE_SIMPLE_CACHED_STATE(VertexAttributeArrayConfiguration, GraphicsInterface::VertexAttributeArrayConfigurationObject,
                           GraphicsInterface::VertexAttributeArrayConfigurationObject, nullptr)
DEFINE_SIMPLE_CACHED_STATE(Viewport, Rect, const Rect&, Rect(0.0f, 0.0f, 1.0f, 1.0f))

DEFINE_INDEXED_CACHED_STATE(Texture, GraphicsInterface::TextureObject, GraphicsInterface::TextureObject, nullptr)
DEFINE_INDEXED_CACHED_STATE(VertexAttributeArrayEnabled, bool, bool, false)
DEFINE_INDEXED_CACHED_STATE(VertexAttributeArraySource, GraphicsInterface::ArraySource, const GraphicsInterface::ArraySource&,
                            GraphicsInterface::ArraySource())
}

}
