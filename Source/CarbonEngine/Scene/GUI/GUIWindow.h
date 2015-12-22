/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventDelegate.h"
#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Math/Rect.h"
#include "CarbonEngine/Render/Font.h"
#include "CarbonEngine/Scene/ComplexEntity.h"

namespace Carbon
{

/**
 * This is the base class for all the 2D GUI entities. Each GUI window has a material and text which gets drawn with the window.
 */
class CARBON_API GUIWindow : public ComplexEntity, public EventHandler
{
public:

    GUIWindow();
    ~GUIWindow() override;

    /**
     * Mouse button down event dispatcher for this window, this can be used as an alternative to handling
     * GUIMouseButtonDownEvent globally.
     */
    EventDispatcher<GUIWindow, const GUIMouseButtonDownEvent&> onMouseButtonDownEvent;

    /**
     * Mouse button up event dispatcher for this window, this can be used as an alternative to handling GUIMouseButtonUpEvent
     * globally.
     */
    EventDispatcher<GUIWindow, const GUIMouseButtonUpEvent&> onMouseButtonUpEvent;

    /**
     * Mouse move event dispatcher for this window, this can be used as an alternative to handling GUIMouseMoveEvent globally.
     */
    EventDispatcher<GUIWindow, const GUIMouseMoveEvent&> onMouseMoveEvent;

    /**
     * Mouse enter event dispatcher for this window, this can be used as an alternative to handling GUIMouseEnterEvent globally.
     */
    EventDispatcher<GUIWindow, const GUIMouseEnterEvent&> onMouseEnterEvent;

    /**
     * Mouse exit event dispatcher for this window, this can be used as an alternative to handling GUIMouseExitEvent globally.
     */
    EventDispatcher<GUIWindow, const GUIMouseExitEvent&> onMouseExitEvent;

    /**
     * Window pressed event dispatcher for this window, this can be used as an alternative to handling GUIWindowPressedEvent
     * globally.
     */
    EventDispatcher<GUIWindow, const GUIWindowPressedEvent&> onWindowPressedEvent;

    /**
     * Gain focus event dispatcher for this window, this can be used as an alternative to handling GUIGainFocusEvent globally.
     */
    EventDispatcher<GUIWindow, const GUIGainFocusEvent&> onGainFocusEvent;

    /**
     * Lose focus event dispatcher for this window, this can be used as an alternative to handling GUILoseFocusEvent globally.
     */
    EventDispatcher<GUIWindow, const GUILoseFocusEvent&> onLoseFocusEvent;

    /**
     * Size changed event dispatcher for this window, fired whenever the window size is altered.
     */
    EventDispatcher<GUIWindow, const Vec2&> onSizeChangedEvent;

    /**
     * Initializer method intended for use by Scene::addEntity<>() and ComplexEntity::addChild<>(), it sets the width, height,
     * position and text of this GUIWindow. If the width and height and are both zero and some text is specified then
     * GUIWindow::autosize() is called.
     */
    virtual void initialize(float width, float height, float x = 0.0f, float y = 0.0f,
                            const UnicodeString& text = UnicodeString::Empty);

    /**
     * Returns whether or not this window is enabled. Enabled windows are able to respond to user input. Defaults to true.
     */
    bool isEnabled() const { return isEnabled_; }

    /**
     * Sets whether this window is enabled. Enabled windows are able to respond to user input. Defaults to true.
     */
    void setEnabled(bool enabled);

    /**
     * Returns the width of this window.
     */
    float getWidth() const { return width_; }

    /**
     * Sets the width of this window.
     */
    void setWidth(float width) { setSize(width, getHeight()); }

    /**
     * Returns the height of this window.
     */
    float getHeight() const { return height_; }

    /**
     * Sets the height of this window.
     */
    void setHeight(float height) { setSize(getWidth(), height); }

    /**
     * Sets the width and height of this window.
     */
    void setSize(const Vec2& dimensions) { setSize(dimensions.x, dimensions.y); }

    /**
     * Sets the width and height of this window.
     */
    void setSize(float width, float height);

    /**
     * Returns the size of the window border that is used when this window has no material. Defaults to 1.
     */
    float getBorderSize() const { return borderSize_; }

    /**
     * Sets the size of the window border that is used when this window has no material. Defaults to 1.
     */
    void setBorderSize(float size) { borderSize_ = size; }

    /**
     * Returns this window's material.
     */
    const String& getMaterial() const { return material_; }

    /**
     * Sets this window's material.
     */
    virtual void setMaterial(const String& material) { material_ = material; }

    /**
     * Returns this window's hover material.
     */
    const String& getHoverMaterial() const { return hoverMaterial_; }

    /**
     * Sets this window's hover material.
     */
    void setHoverMaterial(const String& material) { hoverMaterial_ = material; }

    /**
     * Returns the fill color for this window. If a color has been set with GUIWindow::setFillColor() then it will be used,
     * otherwise the color will be taken from the current GUI theme.
     */
    virtual Color getFillColor() const;

    /**
     * Sets the fill color for this window. If a color is not set then one will be taken from the default GUI theme.
     */
    void setFillColor(const Color& color);

    /**
     * Returns the border color for this window. If a color has been set with GUIWindow::setFillColor() then it will be used,
     * otherwise the color will be taken from the current GUI theme.
     */
    virtual Color getBorderColor() const;

    /**
     * Sets the border color for this window. If a color is not set then one will be taken from the current GUI theme.
     */
    void setBorderColor(const Color& color);

    /**
     * Returns the text color for this window. If a color has been set with GUIWindow::setTextColor() then it will be used,
     * otherwise the color will be taken from the current GUI theme.
     */
    virtual Color getTextColor() const;

    /**
     * Sets the text color for this window. If a color is not set then one will be taken from the current GUI theme.
     */
    void setTextColor(const Color& color);

    /**
     * This erases any custom window colors set with GUIWindow::setFillColor(), GUIWindow::setBorderColor(). and
     * GUIWindow::setTextColor(). The window will now be drawn with colors from the current GUI theme. This is the default
     * behavior.
     */
    void useThemeColors();

    /**
     * Returns the surface color at the given local space point on this GUIWindow. If there is a material applied to this
     * GUIWindow then its diffuse texture sample will be returned. Otherwise the fill color of this window will be returned. The
     * alpha value for this window is included in the returned color. If the given local space point is outside the bounds of
     * this window then Color::Zero is returned.
     */
    virtual Color getSurfaceColor(const Vec2& localPosition) const;

    /**
     * Returns this window's text.
     */
    const UnicodeString& getText() const { return text_; }

    /**
     * Sets this window's text. Text is rendered on this window inside the local text rectangle.
     */
    virtual void setText(const UnicodeString& text);

    /**
     * Resizes this window so that it is as small as possible with all its text still visible. If the text is multilined (i.e.
     * it contains newline characters), then the width of the window will be based on the length of the longest line and the
     * height based on the number of lines. If the text horizontal alignment is set to centered or right aligned then after the
     * window width is changed, the window position may be altered as well to keep the center or right hand side of the text in
     * the same place.
     */
    virtual void autosize();

    /**
     * Returns this window's text margins.
     */
    const Rect& getTextMargins() const { return textMargins_; }

    /**
     * Sets this window's text margins. The default margin size is zero.
     */
    void setTextMargins(const Rect& margins);

    /**
     * Sets this window's text margins all to the same value. The default margin size is zero.
     */
    void setTextMargins(float margin) { setTextMargins({margin, margin, margin, margin}); }

    /**
     * Returns whether word-wrap is enabled for the text on this window. Defaults to true.
     */
    bool isWordWrapEnabled() const { return isWordWrapEnabled_; }

    /**
     * Sets whether word-wrap is enabled for the text on this window.
     */
    void setWordWrapEnabled(bool enabled);

    /**
     * Returns this window's text alignment.
     */
    Font::TextAlignment getTextAlignment() const { return textAlignment_; }

    /**
     * Sets this window's text alignment.
     */
    void setTextAlignment(Font::TextAlignment alignment);

    /**
     * Returns the current font being used on this window. Defaults to the system variable width font defined on the FontManager
     * class.
     */
    const Font* getFont() const { return font_; }

    /**
     * Sets the font to use on this window, the font size is set through GUIWindow::setFontSize(). The default font is the
     * system variable width font.
     */
    void setFont(const String& font);

    /**
     * Returns the font size being used when drawing text on this window, this directly specifies the maximum allowed height of
     * a font character. If the font size is zero then the size used will be taken directly from the specified font.
     */
    float getFontSize() const { return fontSize_; }

    /**
     * Sets the font size to use when drawing text as part of this window, see GUIWindow::getFontSize() for details.
     */
    void setFontSize(float size);

    /**
     * Returns whether this window has any interactive behavior. Returns false for GUIWindows unless they are draggable or
     * resizeable. The different GUI window types which subclass GUIWindow will return true in this function if they have
     * interactive behavior. An interactive window can have focus and therefore receive directed user input and fire events in
     * response to user input.
     */
    virtual bool isInteractive() const { return isDraggable_ || isResizable_; }

    /**
     * Tests whether the given world space 2D point is inside this window's rectangle. Subclasses can override this to alter the
     * hit area for a window.
     */
    virtual bool intersect(const Vec2& position) const;

    /**
     * Tests whether the given world space 3D point is inside this window's rectangle. Subclasses can override this to alter the
     * hit area for a window. The z component of the position is ignored.
     */
    virtual bool intersect(const Vec3& position) const { return intersect(position.toVec2()); }

    /**
     * The GUIWindow::intersect(const Vec2 &) method hides the standard Entity::intersect(const Entity*) method, redefine it
     * here so that it's still available.
     */
    bool intersect(const Entity* entity) const override { return ComplexEntity::intersect(entity); }

    /**
     * Processes events on this GUI window such as mouse and keyboard events.
     */
    bool processEvent(const Event& e) override;

    /**
     * Returns whether this window currently has focus.
     */
    bool hasFocus() const { return hasFocus_; }

    /**
     * Returns whether the mouse pointer is currently in this window.
     */
    bool isMouseInWindow() const;

    /**
     * Returns whether there is an active touch currently in this window, only relevant on devices that support touch input.
     */
    bool isTouchInWindow() const;

    /**
     * Returns whether this window can be dragged and repositioned using the mouse
     */
    bool isDraggable() const { return isDraggable_; }

    /**
     * Sets whether this window can be dragged and repositioned using the mouse.
     */
    void setDraggable(bool draggable);

    /**
     * If a window is draggable then before entering a drag this method is called to ask whether the current local mouse
     * position is a place on the window that can be used to drag it. By default this method always returns true but this can be
     * overriden by subclasses that need to customize dragging behavior.
     */
    virtual bool allowDragEnter(const Vec2& localPosition) const { return true; }

    /**
     * If a window is draggable then this method is called just before entering a drag.
     */
    virtual void onEnterDrag() {}

    /**
     * If a window is draggable then this method is called just after exiting a drag.
     */
    virtual void onExitDrag() {}

    /**
     * Returns whether this window is currently being dragged.
     */
    bool isBeingDragged() const { return isBeingDragged_; }

    /**
     * Returns whether this window can be resized dynamically by the user.
     */
    bool isResizable() const { return isResizable_; }

    /**
     * Sets whether this window can be resized by the user.
     */
    void setResizable(bool resizable);

    /**
     * Returns whether this window is currently being resized.
     */
    bool isBeingResized() const { return isBeingResized_; }

    /**
     * The different screen locations that GUI windows can be automatically aligned to using GUIWindow::alignToScreen().
     */
    enum ScreenLocation
    {
        ScreenLocationNone,
        ScreenBottomLeft,
        ScreenBottomMiddle,
        ScreenBottomRight,
        ScreenMiddleLeft,
        ScreenMiddle,
        ScreenMiddleRight,
        ScreenTopLeft,
        ScreenTopMiddle,
        ScreenTopRight
    };

    /**
     * Aligns this GUIWindow to a screen location. The alignment is then automatically kept when the screen is resized. An
     * optional offset can be given to shift the window from its aligned position. Note that the alignment applies to the full
     * extents of this GUIWindow and any child windows it may have. This means that a collection of windows that share a parent
     * can easily be aligned to the screen as a group by aligning the parent entity. Calling this method with ScreenLocationNone
     * will disable any current alignment active on this window, but will not alter its current position. A 2D offset from the
     * screen location can be specified as well if required.
     */
    void alignToScreen(ScreenLocation location, const Vec2& offset = Vec2::Zero);

    /**
     * Sets whether to center this window on its position, when this is false the bottom left corner of the window is located at
     * the origin in local entity space. Defaults to false on GUI entities and true on Sprite entities.
     */
    void setCenteredOnLocalOrigin(bool center);

    /**
     * Returns whether this window is centered on its local origin. See GUIWindow::setCenteredOnLocalOrigin() for details.
     * Defaults to false.
     */
    bool isCenteredOnLocalOrigin() const { return centerOnLocalOrigin_; }

    /**
     * Returns the current angle by which this window is currently rotated around its center. Clockwise rotations are returned
     * as positive angles and counter-clockwise rotations are returned as negative angles.
     */
    float getRotationAroundCenter() const;

    /**
     * Sets the current angle which this window is currently rotated around its center.
     */
    void setRotationAroundCenter(float radians);

    /**
     * Rotates this window clockwise around its center by the given angle in radians.
     */
    void rotateAroundCenter(float radians);

    void clear() override;
    void intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry) override;
    bool gatherGeometry(GeometryGather& gather) override;
    void precache() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;
    int getRenderPriority() const override;
    void lookAtPoint(const Vec3& p) override;
    operator UnicodeString() const override;
    bool invalidateWorldTransform(const String& attachmentPoint = String::Empty) override;

protected:

    /**
     * The text string for this window. This is what is set by the user of this class and is then processed and positioned into
     * individual lines by GUIWindow::updateLines() for display in order to account for wrapping, newline characters, text
     * alignment, and overflow if there is too much text to fit inside the window.
     */
    UnicodeString text_;

    /**
     * Struct used to describe a single piece of positioned text that will be drawn in this window, these are setup by
     * GUIWindow::updateLines() based on the value of GUIWindow::text_.
     */
    class GUITextLine
    {
    public:

        /**
         * Returns the position of this line of text in local coordinates.
         */
        const Vec2& getPosition() const { return position_; }

        /**
         * Sets the position of this line of text in local coordinates.
         */
        void setPosition(const Vec2& position) { position_ = position; }

        /**
         * Returns the content of this line of text.
         */
        const UnicodeString& getText() const { return text_; }

        /**
         * Returns whether this line should be drawn, i.e. whether it lies inside the bounds of this window.
         */
        bool isVisible() const { return isVisible_; }

        /**
         * Sets whether this line should be drawn.
         */
        void setVisible(bool visible) { isVisible_ = visible; }

        GUITextLine() {}

        /**
         * Constructs this text line with the specified text.
         */
        GUITextLine(UnicodeString text) : text_(std::move(text)) {}

    private:

        Vec2 position_;
        UnicodeString text_;
        bool isVisible_ = false;
    };

    /**
     * The lines of text to draw in this window as setup by GUIWindow::updateLines().
     */
    Vector<GUITextLine> lines_;

    /**
     * Whether the contents of GUIWindow::lines_ are current or need to be updated before this window can be drawn.
     */
    bool areLinesCurrent_ = false;

    /**
     * Updates the GUIWindow::lines_ vector based on the contents of GUIWindow::text_, subclasses can override this method to
     * change how text is positioned and drawn.
     */
    virtual void updateLines();

    /**
     * Sets the \a position and \a isVisible members of the current entries in the GUIWindow::lines_ vector. This is called
     * immediately after GUIWIndow::updateLines() and prior to any lines of text being drawn.
     */
    virtual void positionLines();

    /**
     * Returns the text margins to use when positioning text inside this window, by default this is the text margins as set by
     * GUIWindow::setTextMargins() plus the border size for this window. Subclasses can override this to alter the text margins.
     */
    virtual Rect getTextMarginsToUse() const { return textMargins_ + Vec2(getBorderSize()); }

    /**
     * Returns the font to use when positioning and rendering text for this window, if no font has been set with
     * GUIWindow::setFont() then the system variable width font will be returned.
     */
    const Font* getFontToUse() const;

    /**
     * Returns the font size to use when positioning and rendering text for this window, if no font size has been set with
     * GUIWindow::setFontSize() then the
     */
    float getFontSizeToUse(const Font* font) const;

    /**
     * Returns whether this window is culled by the camera as specified in the passed GeometryGather.
     */
    bool isCulledBy(const GeometryGather& gather) const;

    /**
     * Wrapper over Scene::screenToWorld() that drops the Z dimension.
     */
    Vec2 screenToWorld(const Vec2& p) const;

    /**
     * Converts from local entity space to window coordinates for this window where (0,0) is the bottom left corner of this
     * window.
     */
    Vec2 localToWindow(const Vec3& p) const;

    /**
     * The material to sue when rendering this window, if no material is set then the window is rendered with a border and
     * translucent background the default colors for which are controlled by the current theme.
     */
    String material_;

    /**
     * Takes the specified color and returns it with its alpha value multipled through by this window's final alpha as returned
     * by Entity::getFinalAlpha(), the RGB component is returned unchanged.
     */
    Color adjustColorAlpha(const Color& c) const { return {c.r, c.g, c.b, c.a * getFinalAlpha()}; }

    /**
     * Whether to use a custom fill color for this window instead of the current theme color.
     */
    bool useCustomFillColor_ = false;

    /**
     * Whether to use a custom border color for this window instead of the current theme color.
     */
    bool useCustomBorderColor_ = false;

    /**
     * Whether to use a custom text color for this window instead of the current theme color.
     */
    bool useCustomTextColor_ = false;

    /**
     * The custom fill color for this window, used if GUIWindow::useCustomFillColor_ is true.
     */
    Color fillColor_;

    /**
     * The custom border color for this window, used if GUIWindow::useCustomBorderColor_ is true.
     */
    Color borderColor_;

    /**
     * The custom text color for this window, used if GUIWindow::useCustomTextColor_ is true.
     */
    Color textColor_;

    /**
     * Queues a window for rendering using the given dimensions, border size and colors.
     */
    void queueWindow(GeometryGather& gather, float width, float height, float borderSize, const Color& fillColor,
                     const Color& borderColor);

    /**
     * Queues text for rendering at the given local position.
     */
    void queueText(GeometryGather& gather, const Vec2& position, const UnicodeString& text, const Color& color);

    /**
     * Called prior to a GUIMouseButtonDownEvent being sent from this window.
     */
    virtual void onBeforeGUIMouseButtonDownEvent(const GUIMouseButtonDownEvent& gmbde) {}

    void calculateLocalAABB() const override;

private:

    friend class Scene;

    bool isEnabled_ = true;
    float width_ = 0.0f;
    float height_ = 0.0f;
    bool centerOnLocalOrigin_ = false;
    float borderSize_ = 0.0f;

    bool alignToScreen_ = false;
    ScreenLocation alignScreenLocation_ = ScreenLocationNone;
    Vec2 alignOffset_;

    String hoverMaterial_;

    Rect textMargins_;
    bool isWordWrapEnabled_ = true;
    Font::TextAlignment textAlignment_ = Font::AlignTopLeft;
    const Font* font_ = nullptr;
    float fontSize_ = 0.0f;

    bool isDraggable_ = false;
    bool isBeingDragged_ = false;
    Vec2 dragOrigin_;
    uintptr_t dragTouchID_ = 0;
    void setIsBeingDragged(bool dragged);    // This is responsible for calling onEnterDrag() and onExitDrag() as appropriate

    bool isResizable_ = false;
    bool isBeingResized_ = false;
    Vec2 resizeOrigin_;

    bool isMouseInWindow_ = false;

    void doMouseEnterExitHandling(const Vec2& mousePosition);

    float getResizeHandleSize() const { return getBorderSize() * 10.0f; }

    bool hasFocus_ = false;
    void setHasFocus(bool hasFocus);
};

}
