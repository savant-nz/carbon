/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Graphics/States/CachedState.h"

namespace Carbon
{

namespace States
{

/**
 * This class implements CachedState for simple states that are set via a single call to the graphics interface. This
 * covers the majority of cases with the exception of indexed states that are handled by the IndexedCachedState class.
 */
template <typename ValueType, typename GraphicsInterfaceSetterParameterType>
class SimpleCachedState : public CachedState
{
public:

    /**
     * Typedef for a method that updates this simple cached state in the graphics interface.
     */
    typedef std::function<void(GraphicsInterface*, GraphicsInterfaceSetterParameterType value)>
        SetGraphicsInterfaceStateMethod;

    /**
     * Constructor takes the name of this simple cached state, its default value, and a pointer to the GraphicsInterface
     * member function to use to update this state.
     */
    SimpleCachedState(const String& name, const ValueType& defaultValue, SetGraphicsInterfaceStateMethod setStateMethod)
        : CachedState(name), setGraphicsInterfaceStateMethod_(setStateMethod)
    {
        set(defaultValue);
    }

    /**
     * Returns the current value of this simple cached state.
     */
    const ValueType& get() const { return stack_[stackPosition_]; }

    /**
     * Returns the current value of this simple cached state.
     */
    ValueType& get() { return stack_[stackPosition_]; }

    /**
     * Sets the current value of this simple cached state.
     */
    void set(const ValueType& value) { stack_[stackPosition_] = value; }

    /**
     * The assignment operator is a shortcut for set(), this allows for direct assignment,
     * e.g. "States::DepthTestEnabled = true;".
     */
    SimpleCachedState<ValueType, GraphicsInterfaceSetterParameterType>& operator=(const ValueType& other)
    {
        set(other);
        return *this;
    }

    /**
     * The comparison operator compares directly to the value returned by SimpleCachedState::get().
     */
    bool operator==(const ValueType& other) const { return get() == other; }

    /**
     * Automatic cast to the type of this simple cached state.
     */
    operator const ValueType&() const { return get(); }

    /**
     * Automatic cast to the type of this simple cached state.
     */
    operator ValueType&() { return get(); }

    /**
     * Array index operator, only ever used when ValueType is Matrix4.
     */
    float& operator[](unsigned int index) { return get()[index]; }

    void flush() override
    {
        if (!isEnabled())
            return;

        if (isDirty() || get() != currentGraphicsInterfaceValue_)
        {
            updateGraphicsInterfaceState(get());
            setDirty(false);
        }
    }

    void toString(Vector<UnicodeString>& v) const override { v.append(UnicodeString(name_) + ": " + get()); }

    void push() override
    {
        assert(stackPosition_ + 1 < StackSize && "State stack overflow");

        stack_[stackPosition_ + 1] = get();
        stackPosition_++;
    }

    /**
     * Calls SimpleCachedState::push(), SimpleCachedState::set() with the passed value, SimpleCachedState::flush() and
     * then SimpleCachedState::pop(). This is useful for ensuring the real graphics interface state is set to a specific
     * value without affecting the current cached state.
     */
    void pushSetFlushPop(const ValueType& value)
    {
        push();
        set(value);
        flush();
        pop();
    }

    /**
     * If this simple cached state is dealing with a GraphicsInterface object such as a data buffer or a render target
     * then it needs to be able to handle when the object gets deleted and properly flush it out of the caching system.
     * This is particularly important because new objects may reuse the same value as old objects and this would confuse
     * the state cacher if the old value had not been completely erased from the caching system.
     */
    void onGraphicsInterfaceObjectDelete(ValueType value)
    {
        // Clear it out of the stack
        for (auto& entry : stack_)
        {
            if (entry == value)
                entry = nullptr;
        }

        // Clear it out of the hardware state
        if (currentGraphicsInterfaceValue_ == value)
            updateGraphicsInterfaceState(nullptr);
    }

    /**
     * Returns the current value of this state that was set by the last call through to the graphics interface.
     */
    const ValueType& getCurrentGraphicsInterfaceValue() const { return currentGraphicsInterfaceValue_; }

private:

    ValueType stack_[StackSize] = {};
    ValueType currentGraphicsInterfaceValue_ = ValueType();

    const SetGraphicsInterfaceStateMethod setGraphicsInterfaceStateMethod_ = nullptr;

    void updateGraphicsInterfaceState(const ValueType& value)
    {
        currentGraphicsInterfaceValue_ = value;
        setGraphicsInterfaceStateMethod_(&graphics(), currentGraphicsInterfaceValue_);

        graphicsInterfaceStateUpdateCount_++;
    }
};

}

}
