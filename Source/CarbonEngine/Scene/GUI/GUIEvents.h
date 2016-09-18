/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Platform/PlatformEvents.h"

namespace Carbon
{

/**
 * Holds details common to all GUI events, currently this is just the window that the event originated from.
 */
class CARBON_API GUIEventDetails
{
public:

    /**
     * Copy constructor.
     */
    GUIEventDetails(const GUIEventDetails& other) = default;

    /**
     * Sets the window that the GUI event came from.
     */
    GUIEventDetails(GUIWindow* window) : window_(window) {}

    virtual ~GUIEventDetails() {}

    /**
     * Returns the Scene that this event came from.
     */
    const Scene* getScene() const;

    /**
     * Returns the GUIWindow that this event came from.
     */
    GUIWindow* getWindow() const { return window_; }

    /**
     * Returns the name of the GUIWindow that this event came from.
     */
    const String& getWindowName() const;

private:

    GUIWindow* const window_ = nullptr;
};

/**
 * GUI mouse button down event. Sent when a mouse button is pressed down over an interactive GUIWindow.
 */
class CARBON_API GUIMouseButtonDownEvent : public GUIEventDetails, public MouseButtonDownEvent
{
public:

    /**
     * Copy constructor.
     */
    GUIMouseButtonDownEvent(const GUIMouseButtonDownEvent& other) = default;

    /**
     * Constructs this GUI mouse button down event with the given window and mouse button event values.
     */
    GUIMouseButtonDownEvent(GUIWindow* window, MouseButton button, const Vec2& position, const Vec2& localPosition)
        : GUIEventDetails(window), MouseButtonDownEvent(button, position), localPosition_(localPosition)
    {
    }

    /**
     * Returns the position of the mouse inside the window.
     */
    const Vec2& getLocalPosition() const { return localPosition_; }

    operator UnicodeString() const override
    {
        return MouseButtonDownEvent::operator UnicodeString() << ", window: " << getWindowName()
                                                              << ", local position: " << getLocalPosition();
    }

private:

    const Vec2 localPosition_;
};

/**
 * GUI mouse button up event. Sent when a mouse button is released down over an interactive GUIWindow.
 */
class CARBON_API GUIMouseButtonUpEvent : public GUIEventDetails, public MouseButtonUpEvent
{
public:

    /**
     * Constructs this GUI mouse button up event with the given window and mouse button event values.
     */
    GUIMouseButtonUpEvent(GUIWindow* window, MouseButton button, const Vec2& position, const Vec2& localPosition)
        : GUIEventDetails(window), MouseButtonUpEvent(button, position), localPosition_(localPosition)
    {
    }

    /**
     * Returns the position of the mouse inside the window.
     */
    const Vec2& getLocalPosition() const { return localPosition_; }

    operator UnicodeString() const override
    {
        return MouseButtonUpEvent::operator UnicodeString() << ", window: " << getWindowName()
                                                            << ", local position: " << getLocalPosition();
    }

private:

    const Vec2 localPosition_;
};

/**
 * GUI mouse move event. Sent when the mouse moves over an interactive GUIWindow.
 */
class CARBON_API GUIMouseMoveEvent : public GUIEventDetails, public MouseMoveEvent
{
public:

    /**
     * Constructs this GUI mouse move event with the given window and position values.
     */
    GUIMouseMoveEvent(GUIWindow* window, const Vec2& position, const Vec2& localPosition)
        : GUIEventDetails(window), MouseMoveEvent(position), localPosition_(localPosition)
    {
    }

    /**
     * Returns the position of the mouse inside the window.
     */
    const Vec2& getLocalPosition() const { return localPosition_; }

    operator UnicodeString() const override
    {
        return MouseMoveEvent::operator UnicodeString() << ", window: " << getWindowName()
                                                        << ", local position: " << getLocalPosition();
    }

private:

    const Vec2 localPosition_;
};

/**
 * GUI mouse enter event. Sent when the mouse enters an interactive GUIWindow.
 */
class CARBON_API GUIMouseEnterEvent : public GUIEventDetails, public MouseMoveEvent
{
public:

    /**
     * Constructs this GUI mouse enter event with the given window and position values.
     */
    GUIMouseEnterEvent(GUIWindow* window, const Vec2& position, const Vec2& localPosition)
        : GUIEventDetails(window), MouseMoveEvent(position), localPosition_(localPosition)
    {
    }

    /**
     * Returns the position of the mouse inside the window.
     */
    const Vec2& getLocalPosition() const { return localPosition_; }

    operator UnicodeString() const override
    {
        return MouseMoveEvent::operator UnicodeString() << ", window: " << getWindowName()
                                                        << ", local position: " << getLocalPosition();
    }

private:

    const Vec2 localPosition_;
};

/**
 * GUI mouse exit event. Sent when the mouse exits an interactive GUIWindow.
 */
class CARBON_API GUIMouseExitEvent : public GUIEventDetails, public MouseMoveEvent
{
public:

    /**
     * Constructs this GUI mouse exit event with the given window and position values.
     */
    GUIMouseExitEvent(GUIWindow* window, const Vec2& position, const Vec2& localPosition)
        : GUIEventDetails(window), MouseMoveEvent(position), localPosition_(localPosition)
    {
    }

    /**
     * Returns the position of the mouse inside the window.
     */
    const Vec2& getLocalPosition() const { return localPosition_; }

    operator UnicodeString() const override
    {
        return MouseMoveEvent::operator UnicodeString() << ", window: " << getWindowName()
                                                        << ", local position: " << getLocalPosition();
    }

private:

    const Vec2 localPosition_;
};

/**
 * GUI gain focus event. Sent when an interactive GUI window gets focus.
 */
class CARBON_API GUIGainFocusEvent : public GUIEventDetails, public Event
{
public:

    /**
     * Constructs this GUI gain focus event with the given window.
     */
    GUIGainFocusEvent(GUIWindow* window) : GUIEventDetails(window) {}

    operator UnicodeString() const override { return UnicodeString() << "window: " << getWindowName(); }
};

/**
 * GUI lose focus event. Sent when an interactive GUI window loses focus.
 */
class CARBON_API GUILoseFocusEvent : public GUIEventDetails, public Event
{
public:

    /**
     * Constructs this GUI lose focus event with the given window.
     */
    GUILoseFocusEvent(GUIWindow* window) : GUIEventDetails(window) {}

    operator UnicodeString() const override { return UnicodeString() << "window: " << getWindowName(); }
};

/**
 * GUI combobox item select event that is sent when the active item in a GUICombobox is changed either by the user or
 * programmatically.
 */
class CARBON_API GUIComboboxItemSelectEvent : public GUIEventDetails, public Event
{
public:

    /**
     * Constructs this combobox item select event with the combobox and item index.
     */
    GUIComboboxItemSelectEvent(GUICombobox* combobox, unsigned int item);

    /**
     * Returns the GUICombobox that this event came from.
     */
    const GUICombobox* getCombobox() const;

    /**
     * Returns the index of the item that was selected.
     */
    unsigned int getItem() const { return item_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "combobox: " << getWindowName() << ", item: " << getItem();
    }

private:

    unsigned int item_ = 0;
};

/**
 * GUI slider changed event. Sent when the value of a GUISlider is changed.
 */
class CARBON_API GUISliderChangedEvent : public GUIEventDetails, public Event
{
public:

    /**
     * Constructs this slider changed event with the given slider.
     */
    GUISliderChangedEvent(GUISlider* slider);

    /**
     * Returns the slider that this event came from.
     */
    GUISlider* getSlider() const;

    operator UnicodeString() const override { return UnicodeString() << "slider: " << getSlider(); }
};

/**
 * GUI window pressed down event. Sent when a GUIWindow is clicked or touched.
 */
class CARBON_API GUIWindowPressedEvent : public GUIEventDetails, public Event
{
public:

    /**
     * Constructs this GUI window pressed event with the given window and local position values.
     */
    GUIWindowPressedEvent(GUIWindow* window, const Vec2& localPosition)
        : GUIEventDetails(window), localPosition_(localPosition)
    {
    }

    /**
     * Returns the position inside the window that the press occurred.
     */
    const Vec2& getLocalPosition() const { return localPosition_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "window: " << getWindowName() << ", local position: " << getLocalPosition();
    }

private:

    const Vec2 localPosition_;
};

}
