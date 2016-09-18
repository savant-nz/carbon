/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

namespace States
{

/**
 * Base class for all cached graphics states. All graphics states are tracked by subclasses of this class and are
 * responsible for only updating the graphics state when it is actually necessary for rendering. Individual state values
 * themselves are altered and retrieved through instances of CachedState subclasses, and are all placed in the States
 * namespace. The two classes that implement this class are SimpleCachedState (used for single-value states such as
 * BlendEnabled, ClearColor and Viewport) and IndexedCachedState (used for indexed states such as Texture,
 * VertexAttributeArrayEnabled and VertexAttributeArraySource).
 *
 * In order to make management simpler the whole set of cached states can be pushed and popped (up to a fixed limit,
 * currently 8 pushes are allowed), and there are methods on the StateCacher class to do this. Each cached state can
 * also be managed individually if needed.
 */
class CARBON_API CachedState : private Noncopyable
{
public:

    /**
     * Initializes this cached state with the given name, the name is used for debugging and profiling purposes.
     */
    CachedState(String name) : name_(std::move(name)) {}

    virtual ~CachedState() {}

    /**
     * Returns whether this cached state is enabled, only enabled cached states will be flushed through to the graphics
     * interface. All cached states are enabled by default, and graphics backends can choose to disable any cached
     * states that they don't use in order to avoid the overhead of caching and flushing states that are just going to
     * be ignored anyway. States are disabled by a call to StateCacher::disable() in
     * GraphcisInterface::disableUnusedCachedStates().
     */
    bool isEnabled() const { return isEnabled_; }

    /**
     * Flushes this cached state to the graphics interface, i.e. updates the graphics state if it is out of date.
     */
    virtual void flush() = 0;

    /**
     * Returns this cached state's dirty flag, see CachedState::setDirty() for details.
     */
    bool isDirty() const { return isDirty_; }

    /**
     * Sets this cached state's dirty flag, the next call to CachedState::flush() on a dirty state will always trigger a
     * graphics interface state update call.
     */
    void setDirty(bool dirty) { isDirty_ = dirty; }

    /**
     * Pushes the current state onto the stack, these should always be matched by corresponding calls to
     * CachedState::pop(). In general, the StateCacher::push() and StateCacher::pop() methods should be preferred if the
     * whole of the current state needs to be pushed and popped.
     */
    virtual void push() = 0;

    /**
     * Pops the current state off the state stack. See CachedState::push() for more details.
     */
    void pop()
    {
        assert(stackPosition_ && "Tried to pop an empty state stack");

        stackPosition_--;
    }

    /**
     * Converts this cached state to a string of the form "<name>: <value>" and appends it to the passed string vector.
     * If this state has multiple internal values then multiple items will be appended. This method is used for
     * assisting in debugging and optimization.
     */
    virtual void toString(Vector<UnicodeString>& v) const = 0;

    /**
     * Logs the value returned by this cached state's CachedState::toString() method using LOG_DEBUG().
     */
    void log() const
    {
        auto v = Vector<UnicodeString>();
        toString(v);

        LOG_DEBUG << v;
    }

    /**
     * Returns the name of this cached state.
     */
    const String& getName() const { return name_; }

    /**
     * Returns the number of graphics interface state changes this cached state has made since the last reset. This is
     * used to track the number of state changes during a frame.
     */
    unsigned int getGraphicsInterfaceStateUpdateCount() const { return graphicsInterfaceStateUpdateCount_; }

    /**
     * Resets to zero the counter that is incremented every time a graphics interface state change is made by this
     * cached state. Called at the start of each frame by the renderer.
     */
    void resetGraphicsInterfaceStateUpdateCount() { graphicsInterfaceStateUpdateCount_ = 0; }

protected:

    /**
     * CachedState subclasses implement ther own stack internally which must be of size StackSize, the index of the top
     * of the stack is stored in \a stackPosition_.
     */
    static const auto StackSize = 9U;

    /**
     * The current top of the stack.
     */
    unsigned int stackPosition_ = 0;

    /**
     * The displayable name of this state that is used to report statistics on state changes.
     */
    const String name_;

    /**
     * This counter is incremented every time a graphics interface state change is actually made and is used to track
     * the number of state changes that occur per-frame. It is reset at the start of each frame by
     * CachedState::resetGraphicsInterfaceStateUpdateCount().
     */
    unsigned int graphicsInterfaceStateUpdateCount_ = 0;

private:

    friend class StateCacher;

    bool isEnabled_ = true;
    bool isDirty_ = true;
};

}

}
