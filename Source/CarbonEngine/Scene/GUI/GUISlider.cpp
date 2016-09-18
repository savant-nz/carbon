/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/GUI/GUIEvents.h"
#include "CarbonEngine/Scene/GUI/GUISlider.h"

namespace Carbon
{

GUISlider::GUISlider() : onChangedEvent(this)
{
    clear();
}

GUISlider::~GUISlider()
{
    onDestruct();
    clear();
}

void GUISlider::clear()
{
    fraction_ = 1.0f;
    inSliderDrag_ = false;

    rangeMinValue_ = 0.0f;
    rangeMaxValue_ = 1.0f;

    sliderBarHeight_ = 0.2f;
    handleWidth_ = 0.03f;

    notches_.clear();
    notchSnapDistance_ = 5;
    notchWidth_ = 1.0f;
    notchHeight_ = 10.0f;

    handleMaterial_.clear();

    setOutputWindow(nullptr);

    GUIWindow::clear();
}

bool GUISlider::gatherGeometry(GeometryGather& gather)
{
    if (!ComplexEntity::gatherGeometry(gather))
        return false;

    if (shouldProcessGather(gather))
    {
        auto fullMaterialName = getMaterialRoot() + material_;

        // Draw slider bar
        gather.changePriority(getRenderPriority() - 1);
        gather.changeTransformation(localToWorld(Vec3(0.0f, (getHeight() * (1.0f - sliderBarHeight_)) * 0.5f)),
                                    getWorldOrientation());
        if (material_.length())
        {
            auto overrideParameters = getMaterialOverrideParameters(fullMaterialName);

            gather.changeMaterial(fullMaterialName, overrideParameters);
            gather.addRectangle(getWidth(), sliderBarHeight_ * getHeight());
        }
        else
        {
            queueWindow(gather, getWidth(), sliderBarHeight_ * getHeight(), getBorderSize(), getFillColor(),
                        getBorderColor());
        }

        // Draw notches
        for (const auto& notch : notches_)
        {
            auto notchFraction = (notch.getPosition() - rangeMinValue_) / (rangeMaxValue_ - rangeMinValue_);

            gather.changeTransformation(
                localToWorld(Vec3(handleWidth_ * 0.5f + (getWidth() - handleWidth_) * notchFraction,
                                  (getHeight() - notchHeight_) * 0.5f)),
                getWorldOrientation());

            if (material_.length())
            {
                auto overrideParameters = getMaterialOverrideParameters(fullMaterialName);

                gather.changeMaterial(fullMaterialName, overrideParameters);
                gather.addRectangle(notchWidth_, notchHeight_);
            }
            else
                queueWindow(gather, notchWidth_, notchHeight_, getBorderSize(), getFillColor(), getBorderColor());
        }

        // Draw slider
        fullMaterialName = getMaterialRoot() + handleMaterial_;
        gather.changePriority(getRenderPriority());
        gather.changeTransformation(localToWorld(Vec3((getWidth() * (1.0f - handleWidth_)) * fraction_, 0.0f)),
                                    getWorldOrientation());
        if (handleMaterial_.length())
        {
            auto overrideParameters = getMaterialOverrideParameters(fullMaterialName);

            gather.changeMaterial(fullMaterialName, overrideParameters);
            gather.addRectangle(getWidth() * handleWidth_, getHeight());
        }
        else
        {
            queueWindow(gather, getWidth() * handleWidth_, getHeight(), getBorderSize(), getFillColor(),
                        getBorderColor());
        }
    }

    return true;
}

bool GUISlider::processEvent(const Event& e)
{
    if (isEnabled() && isVisibleIgnoreAlpha())
    {
        // The slider moves 5% at a time in response to mouse wheel and keyboard input events
        auto stepPercentage = 0.05f;

        if (auto kde = e.as<KeyDownEvent>())
        {
            if (hasFocus())
            {
                // Slider keyboard input: left and down arrows move left, right and up arrows move right, the home key
                // jumps to zero and the end key jumps to one.
                switch (kde->getKey())
                {
                    case KeyLeftArrow:
                    case KeyDownArrow:
                        setFraction(fraction_ - stepPercentage);
                        break;

                    case KeyRightArrow:
                    case KeyUpArrow:
                        setFraction(fraction_ + stepPercentage);
                        break;

                    case KeyHome:
                        setFraction(0.0f);
                        break;

                    case KeyEnd:
                        setFraction(1.0f);
                        break;

                    default:
                        break;
                }
            }
        }
        else if (auto mwe = e.as<MouseWheelEvent>())
        {
            // Mouse wheel moves the slider
            if (isMouseInWindow())
            {
                if (mwe->getDirection() == MouseWheelEvent::AwayFromUser)
                    setFraction(fraction_ + stepPercentage);
                else if (mwe->getDirection() == MouseWheelEvent::TowardsUser)
                    setFraction(fraction_ - stepPercentage);
            }
        }
        else if (auto mbde = e.as<MouseButtonDownEvent>())
        {
            // Position the slider on a click in the window
            if (mbde->getButton() == LeftMouseButton)
            {
                if (isMouseInWindow())
                {
                    setValue(getValueFromWorldPosition(screenToWorld(mbde->getPosition())));

                    // Clicking on a slider widget tries to snap the slider position to all notches on the slider that
                    // are defined as being snappable.

                    // Work out snap range
                    auto snapFraction = notchSnapDistance_ / (getWidth() - handleWidth_);
                    auto snapDistance = rangeMinValue_ + (rangeMaxValue_ - rangeMinValue_) * snapFraction;

                    // Search for the closest snappable notch that is within the snap distance
                    auto closestSnapNotch = pointer_to<const Notch>::type();
                    for (const auto& notch : notches_)
                    {
                        if (!notch.isSnappable())
                            continue;

                        auto distanceToNotch = fabsf(getValue() - notch.getPosition());

                        // See if the mouse was clicked within snapping distance of this notch
                        if (distanceToNotch < snapDistance)
                        {
                            if (closestSnapNotch &&
                                distanceToNotch > fabsf(getValue() - closestSnapNotch->getPosition()))
                                continue;

                            closestSnapNotch = &notch;
                        }
                    }

                    // Snap to a notch if one was found
                    if (closestSnapNotch)
                        setValue(closestSnapNotch->getPosition());

                    // This is used for click and drag input
                    inSliderDrag_ = true;
                }
            }
        }
        else if (auto mbue = e.as<MouseButtonUpEvent>())
        {
            if (mbue->getButton() == LeftMouseButton)
            {
                // Mouse button was released, so finish the drag
                inSliderDrag_ = false;
            }
        }
        else if (auto mme = e.as<MouseMoveEvent>())
        {
            // If the slider is currently being dragged then update its position on mouse move events
            if (inSliderDrag_)
                setValue(getValueFromWorldPosition(screenToWorld(mme->getPosition())));
        }
    }

    return GUIWindow::processEvent(e);
}

float GUISlider::getValueFromWorldPosition(const Vec3& p) const
{
    auto fraction = Math::clamp01((worldToLocal(p).x - handleWidth_ * 0.5f) / (getWidth() - handleWidth_));

    return rangeMinValue_ + (rangeMaxValue_ - rangeMinValue_) * fraction;
}

Vec3 GUISlider::getWorldPositionFromValue(float value) const
{
    auto fraction = Math::clamp01((value - rangeMinValue_) / (rangeMaxValue_ - rangeMinValue_));

    return localToWorld(Vec3(handleWidth_ * 0.5f + (getWidth() - handleWidth_) * fraction, getHeight() * 0.5f, 0.0f));
}

void GUISlider::setOutputWindow(GUIWindow* window)
{
    if (outputWindow_)
        outputWindow_->onDestroyEvent.removeHandler(this, &GUISlider::onOutputWindowDestroy);

    outputWindow_ = window;

    if (outputWindow_)
        outputWindow_->onDestroyEvent.addHandler(this, &GUISlider::onOutputWindowDestroy);
}

void GUISlider::onOutputWindowDestroy(Entity& sender, Entity* entity)
{
    outputWindow_ = nullptr;
}

void GUISlider::updateLines()
{
    // Sliders don't display any text

    lines_.clear();
}

void GUISlider::setFraction(float fraction)
{
    fraction_ = Math::clamp01(fraction);

    auto event = GUISliderChangedEvent(this);
    events().dispatchEvent(event);
    onChangedEvent.fire(event);

    if (outputWindow_)
    {
        auto stream = std::ostringstream();
        stream << std::fixed << std::setprecision(2) << getValue();

        outputWindow_->setText(stream.str().c_str());
    }
}

bool GUISlider::setRange(float minValue, float maxValue)
{
    if (minValue > maxValue)
    {
        LOG_ERROR << "minValue must not be greater than maxValue";
        return false;
    }

    rangeMinValue_ = minValue;
    rangeMaxValue_ = maxValue;

    // Check if any notches are now out of range and if so then delete them
    notches_.eraseIf(
        [&](const Notch& notch) { return notch.getPosition() < minValue || notch.getPosition() > maxValue; });

    return true;
}

float GUISlider::getValue() const
{
    return rangeMinValue_ + getFraction() * (rangeMaxValue_ - rangeMinValue_);
}

void GUISlider::setValue(float value)
{
    setFraction((value - rangeMinValue_) / (rangeMaxValue_ - rangeMinValue_));
}

bool GUISlider::addNotch(float position, bool isSnappable)
{
    if (position < rangeMinValue_ || position > rangeMaxValue_)
        return false;

    notches_.emplace(position, isSnappable);

    return true;
}

void GUISlider::setNotchWidth(float width)
{
    notchWidth_ = Math::clamp(width, 0.0f, getWidth());
}

void GUISlider::setNotchHeight(float height)
{
    notchHeight_ = Math::clamp(height, 0.0f, getHeight());
}

}
