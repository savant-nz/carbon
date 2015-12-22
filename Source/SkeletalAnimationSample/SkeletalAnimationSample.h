/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/CarbonEngine.h"

using namespace Carbon;

class SkeletalAnimationSample : public CARBON_APPLICATION_CLASS
{
public:

    bool initialize() override;
    void frameUpdate() override;
    void queueScenes() override;

    bool onKeyDownEvent(const KeyDownEvent& e) override;
    bool onMouseButtonDownEvent(const MouseButtonDownEvent& e) override;

private:

    SplashScreen splashScreen_;

    // The scene
    Scene scene_;
    Camera* camera_ = nullptr;
    SkeletalMesh* skeletalMesh_ = nullptr;
    void createScene();

    bool showSkeletalMeshBoundingVolume_ = false;

    // The HUD
    Scene hud_;
    void createHUD();
};
