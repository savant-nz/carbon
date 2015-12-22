/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Scene/GUI/GUIMousePointer.h"

namespace Carbon
{

const auto GUIMousePointerVersionInfo = VersionInfo(1, 0);

GUIMousePointer::~GUIMousePointer()
{
    onDestruct();
    clear();
}

void GUIMousePointer::clear()
{
    localPointerOrigin_.setXY(0.0f, 1.0f);

    GUIWindow::clear();

    setMaterial("MousePointer");
    setRenderPriority(INT_MAX);
}

const Vec2& GUIMousePointer::getLocalPointerOrigin() const
{
    return localPointerOrigin_;
}

void GUIMousePointer::setLocalPointerOrigin(const Vec2& origin)
{
    localPointerOrigin_ = origin;
}

void GUIMousePointer::update()
{
    if (isEnabled())
    {
        setWorldPosition(screenToWorld(platform().getMousePosition()) -
                         (getWorldOrientation() * (Vec2(getWidth(), getHeight()) * localPointerOrigin_)).toVec2());
    }

    GUIWindow::update();
}

void GUIMousePointer::save(FileWriter& file) const
{
    GUIWindow::save(file);

    file.beginVersionedSection(GUIMousePointerVersionInfo);
    file.write(localPointerOrigin_);
    file.endVersionedSection();
}

void GUIMousePointer::load(FileReader& file)
{
    try
    {
        GUIWindow::load(file);

        file.beginVersionedSection(GUIMousePointerVersionInfo);
        file.read(localPointerOrigin_);
        file.endVersionedSection();
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

}
