/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/GUI/GUIWindow.h"

namespace Carbon
{

/**
 * Slider GUI item.
 */
class CARBON_API GUISlider : public GUIWindow
{
public:

    GUISlider();
    ~GUISlider() override;

    /**
     * Slider changed event dispatcher for this slider, this can be used as an instance-specific alternative to handling
     * GUISliderChangedEvent globally.
     */
    EventDispatcher<GUISlider, const GUISliderChangedEvent&> onChangedEvent;

    /**
     * Describes a notch on a slider. Notches are added to sliders using GUISlider::addNotch.
     */
    class Notch
    {
    public:

        /**
         * Returns the position of the notch on the slider, in the range \a minValue to \a maxValue which was set by the last
         * call to GUISlider::setRange().
         */
        float getPosition() const { return position_; }

        /**
         * Returns whether clicking on the slider near this notch will snap directly to the notch position.
         */
        bool isSnappable() const { return isSnappable_; }

        Notch() {}

        /**
         * Constructs this notch with the given position and snappable values.
         */
        Notch(float position, bool isSnappable) : position_(position), isSnappable_(isSnappable) {}

    private:

        float position_ = 0.0f;
        bool isSnappable_ = false;
    };

    void initialize(float width, float height, const Vec2& position = Vec2::Zero,
                    const UnicodeString& text = UnicodeString::Empty) override
    {
        GUIWindow::initialize(width, height, position);
    }

    /**
     * Initializer method intended for use by Scene::addEntity<>() and ComplexEntity::addChild<>(), it sets the width, height,
     * position and range of this GUISlider.
     */
    virtual void initialize(float width, float height, const Vec2& position, float minValue, float maxValue)
    {
        GUIWindow::initialize(width, height, position);
        setRange(minValue, maxValue);
    }

    /**
     * Returns the current slider fraction. Will be in the range 0.0 - 1.0.
     */
    float getFraction() const { return fraction_; }

    /**
     * Sets the current slider fraction. The given value will be clamped to the range 0.0 - 1.0 and then applied to the slider.
     */
    void setFraction(float fraction);

    /**
     * Sets the range of values for this slider, this is used to set the minimum and maximum values for the slider. The
     * GUISlider::getValue() and GUISlider::setValue() methods work with the currently set range. The default range is 0.0 to
     * 1.0. Returns success flag.
     */
    bool setRange(float minValue, float maxValue);

    /**
     * Returns the current value on this slider by taking the current slider fraction and converting it into a value in the
     * slider range set by GUISlider::setRange().
     */
    float getValue() const;

    /**
     * Sets the current value on this slider by converting the given value into the 0.0 to 1.0 range and calling
     * GUISlider::setFraction() for that value. A value outside the slider's range will be clamped inside it.
     */
    void setValue(float value);

    /**
     * Returns the current height of the slider bar as a fraction of this slider's height. Default is 0.2f.
     */
    float getBarHeight() const { return sliderBarHeight_; }

    /**
     * Sets the height of the slider bar as a fraction of this slider's height. This is clamped between zero and one.
     */
    void setBarHeight(float height) { sliderBarHeight_ = Math::clamp01(height); }

    /**
     * Returns the current width of the slider handle as a fraction of this slider's width. Default is 0.05f.
     */
    float getHandleWidth() const { return handleWidth_; }

    /**
     * Sets the width of the slider handle as a fraction of this slider's width. This is clamped between zero and one.
     */
    void setHandleWidth(float width) { handleWidth_ = Math::clamp01(width); }

    /**
     * Returns the material that will be used to draw the slider handle, the bar behind the slider will be drawn with the
     * GUIWindow::getMaterial() material.
     */
    const String& getHandleMaterial() const { return handleMaterial_; }

    /**
     * Sets the material that will be used to draw the slider handle, the bar behind the slider will be drawn with the
     * GUIWindow::getMaterial() material.
     */
    void setHandleMaterial(const String& material) { handleMaterial_ = material; }

    /**
     * Adds a notch to this slider at the specified position. The position must not lie outside the range specified by setRange.
     * Notches can be made snappable so that when the user clicks on the slider near the notch the slider will snap onto the
     * notch position. Snapping can be controlled with GUISlider::getNotchSnapDistance() and GUISlider::setNotchSnapDistance().
     * Returns success flag.
     */
    bool addNotch(float position, bool isSnappable);

    /**
     * Returns the list of notches currently active on this slider.
     */
    const Vector<Notch>& getNotches() const { return notches_; }

    /**
     * Removes all notches from this slider.
     */
    void clearNotches() { notches_.clear(); }

    /**
     * Returns the distance in pixels around a snappable notch in which an automatic snap will occur when the user clicks on the
     * slider.
     */
    unsigned int getNotchSnapDistance() const { return notchSnapDistance_; }

    /**
     * Sets the distance in pixels around a snappable notch in which an automatic snap will occur when the user clicks on the
     * slider.
     */
    void setNotchSnapDistance(unsigned int distance) { notchSnapDistance_ = distance; }

    /**
     * Returns the width of any notches that are drawn on the slider bar.
     */
    float getNotchWidth() const { return notchWidth_; }

    /**
     * Sets the width of any notches that are drawn on the slider bar.
     */
    void setNotchWidth(float width);

    /**
     * Returns the height of any notches that are drawn on the slider bar.
     */
    float getNotchHeight() const { return notchHeight_; }

    /**
     * Sets the height of any notches that are drawn on the slider bar.
     */
    void setNotchHeight(float height);

    /**
     * Returns the slider value that corresponds to the slider being at the given position in world space. This will clamp the
     * value to the valid range if \a p lies outside the slider window's area.
     */
    float getValueFromWorldPosition(const Vec3& p) const;

    /**
     * Returns the world position on the slider that corresponds to the given slider value. If the given value lies outside the
     * valid range of the slider then it will be clamped appropriately. The returned position will always lie on the middle of
     * the sliding bar.
     */
    Vec3 getWorldPositionFromValue(float value) const;

    /**
     * Sets the output window that will automatically have its text set to this slider's value whenever this slider's value
     * changes. If set to null then automatic output window updating is disabled. Defaults to null.
     */
    void setOutputWindow(GUIWindow* window);

    bool isInteractive() const override { return true; }
    bool processEvent(const Event& e) override;

    void clear() override;
    bool gatherGeometry(GeometryGather& gather) override;

protected:

    void updateLines() override;

private:

    float fraction_ = 0.0f;
    bool inSliderDrag_ = false;

    float rangeMinValue_ = 0.0f;
    float rangeMaxValue_ = 0.0f;

    float sliderBarHeight_ = 0.0f;
    float handleWidth_ = 0.0f;

    Vector<Notch> notches_;
    unsigned int notchSnapDistance_ = 0;
    float notchWidth_ = 0.0f;
    float notchHeight_ = 0.0f;

    String handleMaterial_;

    GUIWindow* outputWindow_ = nullptr;
    void onOutputWindowDestroy(Entity& sender, Entity* entity);
};

}
