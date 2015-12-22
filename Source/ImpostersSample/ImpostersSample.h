/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/CarbonEngine.h"

using namespace Carbon;

class ImpostersSample : public CARBON_APPLICATION_CLASS
{
public:

    bool initialize() override;
    void frameUpdate() override;
    void queueScenes() override;
    void shutdown() override;

private:

    SplashScreen splashScreen_;

    // Imposter scene
    Scene imposterScene_;
    Entity* teapot_ = nullptr;

    // Imposter texture
    Texture2D* imposterTexture_ = nullptr;

    // The main scene
    Scene scene_;
    Vector<Sprite*> sprites_;
    void createScene();

    // HUD scene
    Scene hud_;
    void createHUD();
};
