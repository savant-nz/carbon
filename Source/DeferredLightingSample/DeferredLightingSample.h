/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/CarbonEngine.h"

using namespace Carbon;

class DeferredLightingSample : public CARBON_APPLICATION_CLASS
{
public:

    bool initialize() override;
    void frameUpdate() override;
    void queueScenes() override;

    bool onMouseButtonDownEvent(const MouseButtonDownEvent& e) override;
    bool onKeyDownEvent(const KeyDownEvent& e) override;

private:

    SplashScreen splashScreen_;

    Scene scene_;
    Camera* camera_ = nullptr;
    void createScene();

    Vector<Entity*> boxes_;
    Entity* createBox(const Vec3& position);
    void removeBoxes();
    void createBoxStack();

    Light* spotLight_ = nullptr;
    bool isSpotlightAnimating_ = true;

    Vector<Light*> userLights_;

    Scene hud_;
    void createHUD();
};
