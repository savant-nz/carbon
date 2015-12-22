/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Platform/ThemeManager.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/GUI/GUIEvents.h"
#include "CarbonEngine/Scene/GUI/GUIProgressBar.h"

namespace Carbon
{

GUIProgressBar::~GUIProgressBar()
{
    onDestruct();
    clear();
}

bool GUIProgressBar::setRange(float minValue, float maxValue)
{
    if (minValue > maxValue)
    {
        LOG_ERROR << "minValue must not be greater than maxValue";
        return false;
    }

    rangeMinValue_ = minValue;
    rangeMaxValue_ = maxValue;

    return true;
}

float GUIProgressBar::getValue() const
{
    return rangeMinValue_ + getFraction() * (rangeMaxValue_ - rangeMinValue_);
}

void GUIProgressBar::setValue(float value)
{
    setFraction((value - rangeMinValue_) / (rangeMaxValue_ - rangeMinValue_));
}

void GUIProgressBar::clear()
{
    fraction_ = 0.0f;
    rangeMinValue_ = 0.0f;
    rangeMaxValue_ = 1.0f;

    GUIWindow::clear();
}

bool GUIProgressBar::gatherGeometry(GeometryGather& gather)
{
    if (shouldProcessGather(gather))
    {
        auto fullMaterialName = getMaterialRoot() + material_;

        gather.changePriority(getRenderPriority());
        gather.changeTransformation(getWorldTransform());

        if (material_.length())
        {
            auto overrideParameters = getMaterialOverrideParameters(fullMaterialName);
            gather.changeMaterial(fullMaterialName, overrideParameters);
            gather.addRectangle(getWidth() * fraction_, getHeight());
        }
        else
            queueWindow(gather, getWidth() * fraction_, getHeight(), getBorderSize(), getFillColor(), getBorderColor());

        auto fullBackgroundMaterialName = getMaterialRoot() + backgroundMaterial_;

        gather.changeTransformation(localToWorld(Vec3(getWidth() * fraction_, 0.0f)), getWorldOrientation());
        if (backgroundMaterial_.length())
        {
            auto overrideParameters = getMaterialOverrideParameters(fullBackgroundMaterialName);

            gather.changeMaterial(fullBackgroundMaterialName, overrideParameters);
            gather.addRectangle(getWidth() * (1.0f - fraction_), getHeight());
        }
        else
        {
            queueWindow(gather, getWidth() * (1.0f - fraction_), getHeight(), getBorderSize(), getFillColor(),
                        getBorderColor());
        }
    }

    return true;
}

}
