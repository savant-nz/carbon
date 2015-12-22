/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Graphics/States/CachedState.h"
#include "CarbonEngine/Graphics/States/StateCacher.h"
#include "CarbonEngine/Graphics/States/States.h"

namespace Carbon
{

namespace States
{

Vector<CachedState*> StateCacher::allStates_;
Vector<CachedState*> StateCacher::activeStates_;

void StateCacher::setup()
{
    allStates_.clear();

    // Setup simple cached states
    allStates_.append(&BlendEnabled);
    allStates_.append(&BlendFunction);
    allStates_.append(&ClearColor);
    allStates_.append(&ColorWriteEnabled);
    allStates_.append(&CullMode);
    allStates_.append(&DepthClampEnabled);
    allStates_.append(&DepthClearValue);
    allStates_.append(&DepthCompareFunction);
    allStates_.append(&DepthTestEnabled);
    allStates_.append(&DepthWriteEnabled);
    allStates_.append(&MultisampleEnabled);
    allStates_.append(&RenderTarget);
    allStates_.append(&ScissorEnabled);
    allStates_.append(&ScissorRectangle);
    allStates_.append(&ShaderProgram);
    allStates_.append(&StencilClearValue);
    allStates_.append(&StencilOperationsForBackFaces);
    allStates_.append(&StencilOperationsForFrontFaces);
    allStates_.append(&StencilTestEnabled);
    allStates_.append(&StencilTestFunction);
    allStates_.append(&StencilWriteEnabled);
    allStates_.append(&VertexAttributeArrayConfiguration);
    allStates_.append(&Viewport);

    // Setup indexed texture state
    Texture.setup(graphics().getTextureUnitCount());
    for (auto state : Texture)
        allStates_.append(state);

    // Setup indexed vertex attribute array state if needed
    if (!graphics().isVertexAttribtuteArrayConfigurationSupported())
    {
        VertexAttributeArrayEnabled.setup(graphics().getVertexAttributeArrayCount());
        VertexAttributeArraySource.setup(graphics().getVertexAttributeArrayCount());
        for (auto state : VertexAttributeArrayEnabled)
            allStates_.append(state);
        for (auto state : VertexAttributeArraySource)
            allStates_.append(state);
    }

    // All states are initially active
    activeStates_ = allStates_;

    // Disable states that don't have hardware support in the active graphics interface
    if (!graphics().isDepthClampSupported())
        disable(DepthClampEnabled);
    if (!graphics().isStencilBufferSupported())
    {
        disable(StencilClearValue);
        disable(StencilOperationsForBackFaces);
        disable(StencilOperationsForFrontFaces);
        disable(StencilTestEnabled);
        disable(StencilTestFunction);
        disable(StencilWriteEnabled);
    }
    if (!graphics().isVertexAttribtuteArrayConfigurationSupported())
        disable(VertexAttributeArrayConfiguration);

    // Let the graphics interface disable any more states that it knows it doesn't need
    graphics().disableUnusedCachedStates();

    // Enable and force flush all active states
    for (auto state : activeStates_)
    {
        state->isEnabled_ = true;

        state->setDirty(true);
        state->flush();
    }
}

void StateCacher::setDirty()
{
    for (auto state : activeStates_)
        state->setDirty(true);
}

void StateCacher::disable(CachedState& state)
{
    state.isEnabled_ = false;
    activeStates_.eraseValue(&state);
}

void StateCacher::flush()
{
    for (auto state : activeStates_)
        state->flush();
}

void StateCacher::push()
{
    for (auto state : activeStates_)
        state->push();
}

void StateCacher::pop()
{
    for (auto state : activeStates_)
        state->pop();
}

Vector<UnicodeString> StateCacher::getCurrentState()
{
    auto result = Vector<UnicodeString>();

    for (auto state : activeStates_)
        state->toString(result);

    return result;
}

void StateCacher::resetGraphicsInterfaceStateUpdateCount()
{
    for (auto state : activeStates_)
        state->resetGraphicsInterfaceStateUpdateCount();
}

}

}
