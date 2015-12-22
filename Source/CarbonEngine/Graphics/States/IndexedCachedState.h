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
 * Indexed cached states are single-value states that are also associated with an index, examples include vertex attribute
 * arrays and textures. This template class generically wraps all management of indexed cached states.
 */
template <typename ValueType, typename GraphicsInterfaceSetterParameterType> class IndexedCachedState : private Noncopyable
{
public:

    /**
     * Typedef for a method that updates this indexed cached state in the graphics interface.
     */
    typedef std::function<bool(GraphicsInterface*, unsigned int index, GraphicsInterfaceSetterParameterType value)>
        SetGraphicsInterfaceStateMethod;

    /**
     * This is the actual CachedState subclass used by indexed cached states, it is allocated in IndexedCachedState::setup() and
     * then instances of it are returned by operator[].
     */
    class SingleIndexedCachedState : public CachedState
    {
    public:

        /**
         * Constructor takes the name of this indexed cached state, its default value, and a pointer to the GraphicsInterface
         * member function to use to update this indexed state.
         */
        SingleIndexedCachedState(const String& name, unsigned int index,
                                 SetGraphicsInterfaceStateMethod setGraphicsInterfaceStateMethod)
            : CachedState(name), index_(index), setGraphicsInterfaceStateMethod_(setGraphicsInterfaceStateMethod)
        {
        }

        /**
         * Returns the current value of this indexed cached state.
         */
        const ValueType& get() const { return stack_[stackPosition_]; }

        /**
         * Returns the current value of this indexed cached state.
         */
        ValueType& get() { return stack_[stackPosition_]; }

        /**
         * Sets the current value of this indexed cached state.
         */
        void set(const ValueType& value) { stack_[stackPosition_] = value; }

        /**
         * The assignment operator is a shorthand for SingleIndexedCachedState::set() which allows the instance of the class to
         * be assigned the cached state type directly. This permits a simple syntax for setting indexed cached states.
         */
        SingleIndexedCachedState& operator=(const ValueType& other)
        {
            set(other);
            return *this;
        }

        /**
         * The comparison operator compares directly to the value returned by SingleIndexedCachedState::get().
         */
        bool operator==(const ValueType& other) const { return get() == other; }

        /**
         * Automatic cast to the type of this indexed cached state.
         */
        operator const ValueType&() const { return get(); }

        /**
         * Automatic cast to the type of this indexed cached state.
         */
        operator ValueType&() { return get(); }

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
         * Calls SingleIndexedCachedState::push(), SingleIndexedCachedState::set() with the passed value,
         * SingleIndexedCachedState::flush() and then SingleIndexedCachedState::pop(). This is useful for ensuring the real
         * graphics interface state is set to a specific value without affecting the current cached state.
         */
        void pushSetFlushPop(const ValueType& value)
        {
            push();
            set(value);
            flush();
            pop();
        }

    private:

        friend class IndexedCachedState;

        const unsigned int index_ = 0;

        ValueType stack_[StackSize] = {};
        ValueType currentGraphicsInterfaceValue_ = ValueType();

        const SetGraphicsInterfaceStateMethod setGraphicsInterfaceStateMethod_ = nullptr;

        void updateGraphicsInterfaceState(const ValueType& value)
        {
            if (setGraphicsInterfaceStateMethod_(&graphics(), index_, value))
            {
                currentGraphicsInterfaceValue_ = value;
                graphicsInterfaceStateUpdateCount_++;
            }
        }

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
    };

    /**
     * Constructs this indexed cached state with the given name, default value, and graphics interface setter method.
     */
    IndexedCachedState(String name, ValueType defaultValue, SetGraphicsInterfaceStateMethod setGraphicsInterfaceStateMethod)
        : name_(std::move(name)),
          defaultValue_(std::move(defaultValue)),
          setGraphicsInterfaceStateMethod_(setGraphicsInterfaceStateMethod)
    {
    }

    ~IndexedCachedState() { clear(); }

    /**
     * Sets the number of entries in this indexed cached state, this is called inside GraphicsInterface::setup() implementations
     * in order to appropriately size indexed cached states to the active graphics hardware.
     */
    void setup(unsigned int size)
    {
        clear();

        states_.resize(size);
        for (auto i = 0U; i < size; i++)
        {
            states_[i] = new SingleIndexedCachedState(name_ + "[" + i + "]", i, setGraphicsInterfaceStateMethod_);
            states_[i]->set(defaultValue_);
        }
    }

    /**
     * Clears all the entries in this indexed cache state allocated by a previosu call to IndexedCachedState::setup().
     */
    void clear()
    {
        for (auto state : states_)
            delete state;

        states_.clear();
    }

    /**
     * Returns the number of entries in this indexed cache state.
     */
    unsigned int size() { return states_.size(); }

    /**
     * This is the primary accessor for using an indexed cached state, it returns the SingleIndexedCachedState instance for the
     * given index value.
     */
    SingleIndexedCachedState& operator[](unsigned int index) { return *states_[index]; }

    /**
     * Returns an iterator at the start of the vector of indexed cached states.
     */
    SingleIndexedCachedState** begin() { return states_.begin(); }

    /**
     * Returns an iterator at the end of the vector of indexed cached states.
     */
    SingleIndexedCachedState** end() { return states_.end(); }

    /**
     * If this indexed cached state is dealing with a GraphicsInterface object such as a texture then it needs to be able to
     * handle when the object gets deleted and properly flush it out of the caching system. This is particularly important
     * because new objects may reuse the same value as old objects and this would confuse the state cacher if the old value had
     * not been completely erased from the caching system.
     */
    void onGraphicsInterfaceObjectDelete(ValueType value)
    {
        for (auto state : states_)
            state->onGraphicsInterfaceObjectDelete(value);
    }

private:

    const String name_;
    const ValueType defaultValue_ = ValueType();
    const SetGraphicsInterfaceStateMethod setGraphicsInterfaceStateMethod_ = nullptr;

    Vector<SingleIndexedCachedState*> states_;
};

}

}
