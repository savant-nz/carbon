/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/GUI/GUIWindow.h"

namespace Carbon
{

/**
 * Progress bar GUI item.
 */
class CARBON_API GUIProgressBar : public GUIWindow
{
public:

    GUIProgressBar() { clear(); }
    ~GUIProgressBar() override;

    /**
     * Returns the current progress bar fraction. Will be in the range 0.0 - 1.0.
     */
    float getFraction() const { return fraction_; }

    /**
     * Sets the current progress bar fraction. The given value will be clamped to the range 0.0 - 1.0 and then applied.
     */
    void setFraction(float fraction) { fraction_ = Math::clamp01(fraction); }

    /**
     * Sets the range of values for this progress bar. The GUIProgressBar::getValue() and GUIProgressBar::setValue() methods
     * work within this range. The default range is 0.0 to 1.0. Returns success flag.
     */
    bool setRange(float minValue, float maxValue);

    /**
     * Returns the current value on this progress bar by taking the current fraction and converting it into a value in the
     * progress bar range set by GUIProgressBar::setRange().
     */
    float getValue() const;

    /**
     * Sets the current value on this progress bar by converting the given value into the 0.0 to 1.0 range and calling
     * GUIProgressBar::setFraction() for that value. A value outside the progress bar's range will be clamped inside it.
     */
    void setValue(float value);

    /**
     * Returns the current background material that is used to draw the part of the progress bar that is not 'complete'.
     */
    const String& getBackgroundMaterial() const { return backgroundMaterial_; }

    /**
     * Sets the background material that is used to draw the part of the progress bar that is not 'complete'.
     */
    void setBackgroundMaterial(const String& material) { backgroundMaterial_ = material; }

    void clear() override;
    bool gatherGeometry(GeometryGather& gather) override;

private:

    float fraction_ = 0.0f;

    float rangeMinValue_ = 0.0f;
    float rangeMaxValue_ = 1.0f;

    String backgroundMaterial_;
};

}
