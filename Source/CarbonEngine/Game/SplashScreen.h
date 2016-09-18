/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Platform/SimpleTimer.h"
#include "CarbonEngine/Scene/GUI/GUIWindow.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

/**
 * A splash screen that fades in and out that can be customized with multiple logos.
 */
class CARBON_API SplashScreen : private Noncopyable
{
public:

    /**
     * Initializes this splash screen with the given background material.
     */
    SplashScreen(String backgroundMaterial = "Black");

    ~SplashScreen();

    /**
     * Adds a logo to this splash screen. The logo size defaults to half of the maximum size the logo could be set to
     * without being clipped. By default the logo will be aligned to the center of the screen, but this can be
     * overridden. An offset from the aligned position can also be given and is a normalized resolution-independent
     * value.
     */
    void addLogo(const String& logoTexture, Vec2 size = Vec2(0.33f),
                 GUIWindow::ScreenLocation alignment = GUIWindow::ScreenMiddle, const Vec2& offset = Vec2::Zero);

    /**
     * Controls the fading behavior of the splash screen.
     */
    void setFade(float fadeInTime = 1.5f, float holdTime = 1.0f, float fadeOutTime = 2.0f);

    /**
     * Updates the splash screen for this frame. The return value specifies whether the splash screen is still
     * displaying, and if this is true then the splash screen scene will automatically be queued for rendering.
     */
    bool update();

private:

    SimpleTimer timer_{false};

    Scene* scene_ = nullptr;

    class Logo
    {
    public:

        String texture;
        Vec2 size;
        GUIWindow::ScreenLocation alignment;
        Vec2 offset;

        Sprite* sprite = nullptr;

        Logo(String texture_, const Vec2& size_, GUIWindow::ScreenLocation alignment_, const Vec2& offset_)
            : texture(std::move(texture_)), size(size_), alignment(alignment_), offset(offset_)
        {
        }
    };
    Vector<Logo> logos_;

    const String backgroundMaterial_;

    float fadeInTime_ = 0.0f;
    float fadeOutTime_ = 0.0f;
    float holdTime_ = 0.0f;
};

}
