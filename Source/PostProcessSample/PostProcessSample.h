/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/CarbonEngine.h"

using namespace Carbon;

class PostProcessSample : public CARBON_APPLICATION_CLASS
{
public:

    bool initialize() override;
    void frameUpdate() override;
    void queueScenes() override;

    bool onMouseButtonDownEvent(const MouseButtonDownEvent& e) override;
    bool onMouseButtonUpEvent(const MouseButtonUpEvent& e) override;
    bool onGUISliderChangedEvent(const GUISliderChangedEvent& e) override;

private:

    SplashScreen splashScreen_;

    bool inObjectDrag_ = false;
    bool inCameraDrag_ = false;

    // The scene
    Scene scene_;
    Camera* camera_ = nullptr;
    Entity* object_ = nullptr;
    void createScene();
    void unloadScene();

    // The HUD
    Scene hud_;
    void createHUD();
    void destroyHUD();

    // The GUI
    Scene gui_;
    GUICombobox* materialCombobox_ = nullptr;

    // Brightpass controls
    GUILabel* brightThresholdLabel_ = nullptr;
    GUISlider* brightThresholdSlider_ = nullptr;
    GUILabel* brightThresholdValueLabel_ = nullptr;

    // Blur controls
    GUILabel* blurTypeLabel_ = nullptr;
    GUICombobox* blurTypeCombobox_ = nullptr;
    GUILabel* blurScaleLabel_ = nullptr;
    GUISlider* blurScaleSlider_ = nullptr;
    GUILabel* blurScaleValueLabel_ = nullptr;
    GUILabel* blurStandardDeviationLabel_ = nullptr;
    GUISlider* blurStandardDeviationSlider_ = nullptr;
    GUILabel* blurStandardDeviationValueLabel_ = nullptr;

    // Depth of field controls
    GUILabel* focalLengthLabel_ = nullptr;
    GUISlider* focalLengthSlider_ = nullptr;
    GUILabel* focalLengthValueLabel_ = nullptr;
    GUILabel* focalRangeLabel_ = nullptr;
    GUISlider* focalRangeSlider_ = nullptr;
    GUILabel* focalRangeValueLabel_ = nullptr;

    // Bloom controls
    GUILabel* bloomFactorLabel_ = nullptr;
    GUISlider* bloomFactorSlider_ = nullptr;
    GUILabel* bloomFactorValueLabel_ = nullptr;

    // Exposure controls
    GUILabel* exposureLabel_ = nullptr;
    GUISlider* exposureSlider_ = nullptr;
    GUILabel* exposureValueLabel_ = nullptr;

    void createGUI();
    void destroyGUI();

    // GUI event handlers
    void onMaterialComboboxItemSelect(GUICombobox& sender, const GUIComboboxItemSelectEvent& e);
    void onBlurTypeComboboxItemSelect(GUICombobox& sender, const GUIComboboxItemSelectEvent& e);

    void setPostProcessMaterial(const String& name);
};
