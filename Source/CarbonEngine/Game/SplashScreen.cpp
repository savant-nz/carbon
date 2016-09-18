/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Game/SplashScreen.h"
#include "CarbonEngine/Game/Sprite.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

SplashScreen::SplashScreen(String backgroundMaterial) : backgroundMaterial_(std::move(backgroundMaterial))
{
    scene_ = new Scene("SplashScreen");
    setFade();
}

SplashScreen::~SplashScreen()
{
    delete scene_;
    scene_ = nullptr;
}

void SplashScreen::addLogo(const String& logoTexture, Vec2 size, GUIWindow::ScreenLocation alignment,
                           const Vec2& offset)
{
    logos_.emplace(logoTexture, size, alignment, offset);
}

void SplashScreen::setFade(float fadeInTime, float holdTime, float fadeOutTime)
{
    fadeInTime_ = fadeInTime;
    holdTime_ = holdTime;
    fadeOutTime_ = fadeOutTime;
}

bool SplashScreen::update()
{
    if (!scene_)
        return false;

    if (!timer_.isRunning())
        timer_.start();

    auto secondsPassed = timer_.getElapsedTime().toSeconds();

    auto totalTime = fadeInTime_ + holdTime_ + fadeOutTime_;

    auto result = false;

    if (secondsPassed < totalTime)
    {
        if (!scene_->getEntityCount())
        {
            // Create scene
            scene_->create2DCamera(0.0f, 1.0f);
            scene_->setBackgroundMaterial(backgroundMaterial_);

            // Add logos
            for (auto& logo : logos_)
            {
                logo.sprite = scene_->addEntity<Sprite>("Logo", logo.size.x, logo.size.y);
                logo.sprite->setSpriteTexture(logo.texture);
                logo.sprite->alignToScreen(logo.alignment, logo.offset);
            }
        }

        auto sceneAlpha = 1.0f;
        auto logoAlpha = 1.0f;
        if (secondsPassed < fadeInTime_)
        {
            logoAlpha = secondsPassed / fadeInTime_;
            result = true;
        }
        else if (secondsPassed < fadeInTime_ + holdTime_)
            result = secondsPassed < fadeInTime_ + holdTime_ * 0.1f;
        else
        {
            sceneAlpha = 1.0f - (secondsPassed - (fadeInTime_ + holdTime_)) / fadeOutTime_;

            // Shrink logos away while they are fading out
            for (auto& logo : logos_)
                logo.sprite->setSize(logo.size * powf(sceneAlpha, 4.0f));
        }

        // Set alphas
        scene_->getRootEntity()->setAlpha(sceneAlpha);
        for (auto& logo : logos_)
            logo.sprite->setAlpha(logoAlpha);

        scene_->queueForRendering(nullptr, INT_MAX);
    }
    else
    {
        delete scene_;
        scene_ = nullptr;
    }

    return result;
}

}
