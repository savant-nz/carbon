/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Graphics/States/CachedState.h"
#include "CarbonEngine/Graphics/States/IndexedCachedState.h"
#include "CarbonEngine/Graphics/States/SimpleCachedState.h"
#include "CarbonEngine/Graphics/States/StateTypes.h"
#include "CarbonEngine/Math/Color.h"
#include "CarbonEngine/Math/Rect.h"

namespace Carbon
{

namespace States
{

#define DECLARE_SIMPLE_CACHED_STATE(Name, Type, GraphicsInterfaceSetterParameterType) \
    extern CARBON_API SimpleCachedState<Type, GraphicsInterfaceSetterParameterType> Name

#define DECLARE_INDEXED_CACHED_STATE(Name, Type, GraphicsInterfaceSetterParameterType) \
    extern CARBON_API IndexedCachedState<Type, GraphicsInterfaceSetterParameterType> Name

DECLARE_SIMPLE_CACHED_STATE(BlendEnabled, bool, bool);
DECLARE_SIMPLE_CACHED_STATE(BlendFunction, BlendFunctionSetup, const BlendFunctionSetup&);
DECLARE_SIMPLE_CACHED_STATE(ClearColor, Color, const Color&);
DECLARE_SIMPLE_CACHED_STATE(ColorWriteEnabled, bool, bool);
DECLARE_SIMPLE_CACHED_STATE(CullMode, CullingMode, CullingMode);
DECLARE_SIMPLE_CACHED_STATE(DepthClampEnabled, bool, bool);
DECLARE_SIMPLE_CACHED_STATE(DepthClearValue, float, float);
DECLARE_SIMPLE_CACHED_STATE(DepthCompareFunction, CompareFunction, CompareFunction);
DECLARE_SIMPLE_CACHED_STATE(DepthTestEnabled, bool, bool);
DECLARE_SIMPLE_CACHED_STATE(DepthWriteEnabled, bool, bool);
DECLARE_SIMPLE_CACHED_STATE(MultisampleEnabled, bool, bool);
DECLARE_SIMPLE_CACHED_STATE(RenderTarget, GraphicsInterface::RenderTargetObject, GraphicsInterface::RenderTargetObject);
DECLARE_SIMPLE_CACHED_STATE(ScissorEnabled, bool, bool);
DECLARE_SIMPLE_CACHED_STATE(ScissorRectangle, Rect, const Rect&);
DECLARE_SIMPLE_CACHED_STATE(ShaderProgram, ShaderProgram*, ShaderProgram*);
DECLARE_SIMPLE_CACHED_STATE(StencilClearValue, unsigned int, unsigned int);
DECLARE_SIMPLE_CACHED_STATE(StencilOperationsForBackFaces, StencilOperations, const StencilOperations&);
DECLARE_SIMPLE_CACHED_STATE(StencilOperationsForFrontFaces, StencilOperations, const StencilOperations&);
DECLARE_SIMPLE_CACHED_STATE(StencilTestEnabled, bool, bool);
DECLARE_SIMPLE_CACHED_STATE(StencilTestFunction, StencilTestSetup, const StencilTestSetup&);
DECLARE_SIMPLE_CACHED_STATE(StencilWriteEnabled, bool, bool);
DECLARE_SIMPLE_CACHED_STATE(VertexAttributeArrayConfiguration,
                            GraphicsInterface::VertexAttributeArrayConfigurationObject,
                            GraphicsInterface::VertexAttributeArrayConfigurationObject);
DECLARE_SIMPLE_CACHED_STATE(Viewport, Rect, const Rect&);

DECLARE_INDEXED_CACHED_STATE(Texture, GraphicsInterface::TextureObject, GraphicsInterface::TextureObject);
DECLARE_INDEXED_CACHED_STATE(VertexAttributeArrayEnabled, bool, bool);
DECLARE_INDEXED_CACHED_STATE(VertexAttributeArraySource, GraphicsInterface::ArraySource,
                             const GraphicsInterface::ArraySource&);
}

}
