/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Event.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

/**
 * This event is sent when the main window is resized.
 */
class CARBON_API ResizeEvent : public Event
{
public:

    /**
     * Constructs this resize event with the given window dimensions.
     */
    ResizeEvent(unsigned int width, unsigned int height) : width_(width), height_(height) {}

    /**
     * Returns the new width of the window.
     */
    unsigned int getWidth() const { return width_; }

    /**
     * Returns the new height of the window.
     */
    unsigned int getHeight() const { return height_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "width: " << getWidth() << ", height: " << getHeight();
    }

private:

    const unsigned int width_ = 0;
    const unsigned int height_ = 0;
};

/**
 * This event is sent when the application gains focus. On desktop platforms this corresponds to the main rendering
 * window becoming top-most. On phone and tablet platforms this event is sent when the application starts up, resumes
 * from being backgrounded, or is returned to after a system message has been shown and dismissed by the user. On iOS
 * ApplicationGainFocusEvent::wasBackgrounded() returns true when this event is sent because the application was resumed
 * from being backgrounded.
 */
class CARBON_API ApplicationGainFocusEvent : public Event
{
public:

    ApplicationGainFocusEvent() {}

    /**
     * Constructs this application lose focus event with the specified was-backgrounded value.
     */
    ApplicationGainFocusEvent(bool wasBackgrounded) : wasBackgrounded_(wasBackgrounded) {}

    /**
     * For use on iOS systems, if this returns true then this event signals that the application has been resumed from
     * being backgrounded.
     */
    bool wasBackgrounded() const { return wasBackgrounded_; }

    operator UnicodeString() const override { return UnicodeString() << "wasBackgrounded: " << wasBackgrounded(); }

private:

    const bool wasBackgrounded_ = false;
};

/**
 * This event is sent when the application loses focus. On desktop platforms this corresponds to the main rendering
 * window no longer being top-most. On phone and tablet platforms this event is sent when the application is
 * backgrounded or a system message appears, applications running on these platforms should respond by saving any state
 * necessary for resuming from the current position should the application be unexpectedly terminated. On iOS
 * ApplicationLoseFocusEvent::backgrounded() returns true when this event is sent because the application was
 * backgrounded.
 */
class CARBON_API ApplicationLoseFocusEvent : public Event
{
public:

    ApplicationLoseFocusEvent() {}

    /**
     * Constructs this application lose focus event with the specified backgrounded value.
     */
    ApplicationLoseFocusEvent(bool backgrounded) : backgrounded_(backgrounded) {}

    /**
     * For use on iOS systems, if this returns true then this event signals that the application has been backgrounded.
     */
    bool backgrounded() const { return backgrounded_; }

    operator UnicodeString() const override { return UnicodeString() << "backgrounded: " << backgrounded(); }

private:

    const bool backgrounded_ = false;
};

/**
 * Key down event. This class does not deal with character input, all conversion from raw input events to printable
 * characters, such as for display in editboxes is done automatically and exposed through the CharacterInputEvent class.
 */
class CARBON_API KeyDownEvent : public Event
{
public:

    /**
     * Constructs this key down event with the given key constant and repeat flag.
     */
    KeyDownEvent(KeyConstant key, bool isRepeat) : key_(key), isRepeat_(isRepeat) {}

    /**
     * Returns the key that was pressed. One of the KeyConstant enumeration values.
     */
    KeyConstant getKey() const { return key_; }

    /**
     * Returns whether this key down event is for a key repeat due to a key being held down.
     */
    bool isRepeat() const { return isRepeat_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "key: " << getKeyConstantAsString(getKey()) << ", repeat: " << isRepeat();
    }

private:

    const KeyConstant key_ = KeyNone;
    const bool isRepeat_ = false;
};

/**
 * Key up event.
 */
class CARBON_API KeyUpEvent : public Event
{
public:

    /**
     * Constructs this key up event with the given key constant.
     */
    KeyUpEvent(KeyConstant key) : key_(key) {}

    /**
     * Returns the key that was unpressed. One of the KeyConstant enumeration values.
     */
    KeyConstant getKey() const { return key_; }

    operator UnicodeString() const override { return UnicodeString() << "key: " << getKeyConstantAsString(getKey()); }

private:

    const KeyConstant key_ = KeyNone;
};

/**
 * Character input events are sent when raw user key events result in actual printable characters for the application to
 * process. This is analagous to the \a WM_CHAR message on Windows.
 */
class CARBON_API CharacterInputEvent : public Event
{
public:

    /**
     * Constructs this character input event with the given input string and triggering key press (if known).
     */
    CharacterInputEvent(UnicodeString input, KeyConstant key = KeyNone) : input_(std::move(input)), key_(key) {}

    /**
     * Returns the input string.
     */
    const UnicodeString& getInput() const { return input_; }

    /**
     * Returns the last key press that triggered this input event, if known.
     */
    KeyConstant getKey() const { return key_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "input: " << getInput() << ", key: " << getKeyConstantAsString(getKey());
    }

private:

    const UnicodeString input_;
    const KeyConstant key_ = KeyNone;
};

/**
 * Mouse button down event. This is sent by the input system when a mouse button is pressed down.
 */
class CARBON_API MouseButtonDownEvent : public Event
{
public:

    /**
     * Constructs this mouse button down event with the given button and position values.
     */
    MouseButtonDownEvent(MouseButton button, const Vec2& position) : button_(button), position_(position) {}

    /**
     * Returns the mouse button that was pressed down.
     */
    MouseButton getButton() const { return button_; }

    /**
     * Returns the position of the mouse in the main window when the mouse button was pressed down.
     */
    const Vec2& getPosition() const { return position_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "button: " << getButton() << ", position: " << getPosition();
    }

private:

    const MouseButton button_;
    const Vec2 position_;
};

/**
 * Mouse button up event. This is sent by the input system when a mouse button is released.
 */
class CARBON_API MouseButtonUpEvent : public Event
{
public:

    /**
     * Constructs this mouse button up event with the given button and position values.
     */
    MouseButtonUpEvent(MouseButton button, const Vec2& position) : button_(button), position_(position) {}

    /**
     * Returns the mouse button that was released.
     */
    MouseButton getButton() const { return button_; }

    /**
     * Returns the position of the mouse in the main window when the mouse button was released.
     */
    const Vec2& getPosition() const { return position_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "button: " << getButton() << ", position: " << getPosition();
    }

private:

    const MouseButton button_;
    const Vec2 position_;
};

/**
 * Mouse move event. This is sent by the input system when the mouse is moved.
 */
class CARBON_API MouseMoveEvent : public Event
{
public:

    /**
     * Constructs this mouse move event with the given position.
     */
    MouseMoveEvent(const Vec2& position) : position_(position) {}

    /**
     * Returns the position of the mouse in the window.
     */
    const Vec2& getPosition() const { return position_; }

    operator UnicodeString() const override { return UnicodeString() << "position: " << getPosition(); }

private:

    const Vec2 position_;
};

/**
 * Mouse wheel event. Sent when the mouse wheel is scrolled.
 */
class CARBON_API MouseWheelEvent : public Event
{
public:

    /**
     * Describes which way the mouse wheel was scrolled in a mouse wheel event.
     */
    enum Direction
    {
        TowardsUser,
        AwayFromUser
    };

    /**
     * Constructs this mouse wheel event with the given direction and position values.
     */
    MouseWheelEvent(Direction direction, const Vec2& position) : direction_(direction), position_(position) {}

    /**
     * Returns the direction that the mouse wheel was scrolled.
     */
    Direction getDirection() const { return direction_; }

    /**
     * Returns the position of the mouse in the window when the mouse wheel was scrolled.
     */
    const Vec2& getPosition() const { return position_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "direction: " << getDirection() << ", position: " << getPosition();
    }

private:

    const Direction direction_;
    const Vec2 position_;
};

/**
 * When the main window is being recreated due to an option change two events are sent out. A RecreateWindowEvent with a
 * type of CloseWindow is sent prior to the existing window being destroyed, and once the new window has been created
 * another RecreateWindowEvent is sent out with a type of NewWindow. The events are always sent out in pairs. The
 * expected response to these events is to delete all allocated graphics interface objects on the first event, then
 * recreate them for the new window on the second event.
 */
class CARBON_API RecreateWindowEvent : public Event
{
public:

    /**
     * Enumeration describing the different possible types of window recreation events.
     */
    enum WindowEventType
    {
        /**
         * Indicates that the main window is being destroyed.
         */
        CloseWindow,

        /**
         * Indicates that a new main window has been created.
         */
        NewWindow
    };

    /**
     * Initializes the type of this RecreateWindowEvent.
     */
    RecreateWindowEvent(WindowEventType type) : type_(type) {}

    /**
     * Returns the type of this recreate window event, indicating whether it is for a window close or for a window
     * creation.
     */
    WindowEventType getWindowEventType() const { return type_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "type: " << (type_ == CloseWindow ? "close window" : "new window");
    }

private:

    const WindowEventType type_;
};

/**
 * Game controller button event. This is sent by the input system when a button is pressed down on a game controller.
 */
class CARBON_API GameControllerButtonDownEvent : public Event
{
public:

    /**
     * Constructs this game controller button event with the given button index.
     */
    GameControllerButtonDownEvent(unsigned int controllerID, unsigned int button)
        : controllerID_(controllerID), button_(button)
    {
    }

    /**
     * Returns the ID of the game controller that this button down event was fired for.
     */
    unsigned int getControllerID() const { return controllerID_; }

    /**
     * Returns the button that had its state changed. Will be in range 0-127
     */
    unsigned int getButton() const { return button_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "controller: " << getControllerID() << ", button: " << getButton();
    }

private:

    const unsigned int controllerID_ = 0;
    const unsigned int button_ = 0;
};

/**
 * Game controller button event. This is sent by the input system when a button is released on a game controller.
 */
class CARBON_API GameControllerButtonUpEvent : public Event
{
public:

    /**
     * Constructs this game controller button up event with the given button index.
     */
    GameControllerButtonUpEvent(unsigned int controllerID, unsigned int button)
        : controllerID_(controllerID), button_(button)
    {
    }

    /**
     * Returns the ID of the game controller that this button up event was fired for.
     */
    unsigned int getControllerID() const { return controllerID_; }

    /**
     * Returns the button that had its state changed. Will be in range 0-127
     */
    unsigned int getButton() const { return button_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "controller: " << getControllerID() << ", button: " << getButton();
    }

private:

    const unsigned int controllerID_ = 0;
    const unsigned int button_ = 0;
};

/**
 * This event is sent whenever the current device is shaken, only supported on devices that have the relevant movement
 * sensors such as many iOS devices.
 */
class CARBON_API DeviceShakeEvent : public Event
{
};

/**
 * This event is sent when a finger touches down onto the touchscreen, only supported on devices that support touch
 * input.
 */
class CARBON_API TouchBeginEvent : public Event
{
public:

    /**
     * Initializes the contents of this touch begin event.
     */
    TouchBeginEvent(uintptr_t touchID, const Vec2& position) : touchID_(touchID), position_(position) {}

    /**
     * Returns an ID value that can be used to identify this touch throughout its lifetime.
     */
    uintptr_t getTouchID() const { return touchID_; }

    /**
     * Returns the position on the touchscreen that the touch begin occurred.
     */
    const Vec2& getPosition() const { return position_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "touch: " << uint64_t(getTouchID()) << ", position: " << getPosition();
    }

private:

    const uintptr_t touchID_ = 0;
    const Vec2 position_;
};

/**
 * This event is sent when a finger is lifted off the touchscreen, only supported on devices that support touch input.
 */
class CARBON_API TouchEndEvent : public Event
{
public:

    /**
     * Initializes the contents of this touch end event.
     */
    TouchEndEvent(uintptr_t touchID, const Vec2& position) : touchID_(touchID), position_(position) {}

    /**
     * Returns an ID value that can be used to identify this touch throughout its lifetime.
     */
    uintptr_t getTouchID() const { return touchID_; }

    /**
     * Returns the position on the touchscreen that the touch end occurred.
     */
    const Vec2& getPosition() const { return position_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "touch: " << uint64_t(getTouchID()) << ", position: " << getPosition();
    }

private:

    const uintptr_t touchID_ = 0;
    const Vec2 position_;
};

/**
 * This event is sent when a finger is moved on the touchscreen, only supported on devices that support touch input.
 */
class CARBON_API TouchMoveEvent : public Event
{
public:

    /**
     * Initializes the contents of this touch move event.
     */
    TouchMoveEvent(uintptr_t touchID, const Vec2& position, const Vec2& previousPosition)
        : touchID_(touchID), position_(position), previousPosition_(previousPosition)
    {
    }

    /**
     * Returns an ID value that can be used to identify this touch throughout its lifetime.
     */
    uintptr_t getTouchID() const { return touchID_; }

    /**
     * Returns the position on the touchscreen that the touch move occurred.
     */
    const Vec2& getPosition() const { return position_; }

    /**
     * Returns the previous position on this touchscreen for this touch.
     */
    const Vec2& getPreviousPosition() const { return previousPosition_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "touch: " << uint64_t(getTouchID()) << ", position: " << getPosition();
    }

private:

    const uintptr_t touchID_ = 0;
    const Vec2 position_, previousPosition_;
};

/**
 * This event is sent when fingers are tapped on the touchscreen, only supported on devices that support touch input.
 */
class CARBON_API TouchTapEvent : public Event
{
public:

    /**
     * Initializes the contents of this touch tap event.
     */
    TouchTapEvent(const Vec2& position, unsigned int tapCount, unsigned int fingerCount)
        : position_(position), tapCount_(tapCount), fingerCount_(fingerCount)
    {
    }

    /**
     * Returns the position on the touchscreen that the tap(s) occurred.
     */
    const Vec2& getPosition() const { return position_; }

    /**
     * Returns the number of taps, i.e. 1 = single tap, 2 = double tap, and so on.
     */
    unsigned int getTapCount() const { return tapCount_; }

    /**
     * Returns the number of fingers that were tapped.
     */
    unsigned int getFingerCount() const { return fingerCount_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "position: " << getPosition() << ", taps: " << getTapCount()
                               << ", fingers: " << getFingerCount();
    }

private:

    const Vec2 position_;
    const unsigned int tapCount_ = 0;
    const unsigned int fingerCount_ = 0;
};

/**
 * This event is sent when a pinch gesture is performed on the touchscreen, only supported on devices that support touch
 * input.
 */
class CARBON_API TouchPinchEvent : public Event
{
public:

    /**
     * Initializes the contents of this touch pinch event.
     */
    TouchPinchEvent(float scale, float velocity) : scale_(scale), velocity_(velocity) {}

    /**
     * Returns the current scale factor for the in-progress pinch gesture.
     */
    float getScale() const { return scale_; }

    /**
     * Returns the current velocity of the in-progress pinch gesture, measured in scale factors per second.
     */
    float getVelocity() const { return velocity_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "scale: " << getScale() << ", velocity: " << getVelocity();
    }

private:

    const float scale_ = 0.0f;
    const float velocity_ = 0.0f;
};

/**
 * This event is sent when a rotation gesture is performed on the touchscreen, only supported on devices that support
 * touch input.
 */
class CARBON_API TouchRotationEvent : public Event
{
public:

    /**
     * Initializes the contents of this touch rotation event.
     */
    TouchRotationEvent(float rotation, float velocity) : rotation_(rotation), velocity_(velocity) {}

    /**
     * Returns the current rotation for the in-progress rotation gesture, measured in radians.
     */
    float getRotation() const { return rotation_; }

    /**
     * Returns the current velocity of the in-progress rotation gesture, measured in radians per second.
     */
    float getVelocity() const { return velocity_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "rotation: " << getRotation() << ", velocity: " << getVelocity();
    }

private:

    const float rotation_ = 0.0f;
    const float velocity_ = 0.0f;
};

/**
 * This event is sent when a swipe gesture is performed on the touchscreen, only supported on devices that support touch
 * input.
 */
class CARBON_API TouchSwipeEvent : public Event
{
public:

    /**
     * The potential swipe directions.
     */
    enum SwipeDirection
    {
        SwipeLeft,
        SwipeRight,
        SwipeUp,
        SwipeDown
    };

    /**
     * Initializes the contents of this touch swipe event.
     */
    TouchSwipeEvent(const Vec2& position, SwipeDirection direction, unsigned int fingerCount)
        : position_(position), direction_(direction), fingerCount_(fingerCount)
    {
    }

    /**
     * Returns the position of this swipe on the touchscreen.
     */
    const Vec2& getPosition() const { return position_; }

    /**
     * Returns the direction of this swipe, e.g. left, right, up or down.
     */
    SwipeDirection getDirection() const { return direction_; }

    /**
     * Returns the number of fingers that were swiped, will be in the range 1-4.
     */
    unsigned int getFingerCount() const { return fingerCount_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "position: " << getPosition() << ", direction: " << getDirection()
                               << ", fingers: " << getFingerCount();
    }

private:

    const Vec2 position_;
    const SwipeDirection direction_;
    const unsigned int fingerCount_ = 0;
};

/**
 * This event is sent when a pan gesture is performed on the touchscreen, only supported on devices that support touch
 * input.
 */
class CARBON_API TouchPanEvent : public Event
{
public:

    /**
     * Initializes the contents of this touch pan event.
     */
    TouchPanEvent(const Vec2& translation, unsigned int fingerCount)
        : translation_(translation), fingerCount_(fingerCount)
    {
    }

    /**
     * Returns the current translation of the pan in pixels.
     */
    const Vec2& getTranslation() const { return translation_; }

    /**
     * Returns the number of fingers used in the pan, will be in the range 1-4.
     */
    unsigned int getFingerCount() const { return fingerCount_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "translation: " << getTranslation() << ", finger count: " << getFingerCount();
    }

private:

    const Vec2 translation_;
    const unsigned int fingerCount_ = 0;
};

}
