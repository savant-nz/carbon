/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "PostProcessSample.h"

const auto ObjectMesh = String("Teapot");

bool PostProcessSample::initialize()
{
    renderer().setHDREnabled(true);

    splashScreen_.addLogo("CarbonLogo.png");
    createHUD();
    createScene();
    createGUI();

    return true;
}

void PostProcessSample::frameUpdate()
{
    // Left-click drag rotates the object
    if (inObjectDrag_)
    {
        auto v = platform().getMouseRelative() * 0.01f;

        object_->rotateAroundY(-v.x);
        object_->rotateAxisAngle(object_->getLocalOrientation().getXVector(), v.y);
    }

    // Right-click drag rotates the camera
    if (inCameraDrag_)
    {
        auto v = platform().getMouseRelative() * 0.01f;

        camera_->rotateAroundPoint(Vec3::Zero, Quaternion::createRotationY(-v.x));
        camera_->rotateAroundPoint(Vec3::Zero,
                                   Quaternion::createFromAxisAngle(camera_->getLocalOrientation().getXVector(), -v.y));
    }
}

void PostProcessSample::queueScenes()
{
    if (splashScreen_.update())
        return;

    scene_.queueForRendering();
    gui_.queueForRendering();
    hud_.queueForRendering();
}

void PostProcessSample::onMaterialComboboxItemSelect(GUICombobox& sender, const GUIComboboxItemSelectEvent& e)
{
    if (materialCombobox_->getSelectedItem() < 1)
        setPostProcessMaterial("");
    else
        setPostProcessMaterial("PostProcess/" + A(materialCombobox_->getText()));
}

void PostProcessSample::onBlurTypeComboboxItemSelect(GUICombobox& sender, const GUIComboboxItemSelectEvent& e)
{
    if (scene_.getPostProcessMaterials().empty())
        return;

    auto& material = materials().getMaterial(scene_.getPostProcessMaterials()[0]);

    material.setParameter("blurType", A(blurTypeCombobox_->getText()));
}

bool PostProcessSample::onGUISliderChangedEvent(const GUISliderChangedEvent& e)
{
    // Check that the scene has a post-process material set
    if (scene_.getPostProcessMaterials().empty())
        return true;

    // Get the material being used for post-processing so we can update the values on it
    auto& material = materials().getMaterial(scene_.getPostProcessMaterials()[0]);

    if (e.getWindow() == brightThresholdSlider_)
        material.setParameter("brightThreshold", brightThresholdSlider_->getValue());
    else if (e.getWindow() == blurScaleSlider_)
        material.setParameter("blurScale", blurScaleSlider_->getValue());
    else if (e.getWindow() == blurStandardDeviationSlider_)
        material.setParameter("blurStandardDeviation", blurStandardDeviationSlider_->getValue());
    else if (e.getWindow() == focalLengthSlider_)
        material.setParameter("focalLength", focalLengthSlider_->getValue());
    else if (e.getWindow() == focalRangeSlider_)
        material.setParameter("focalRange", focalRangeSlider_->getValue());
    else if (e.getWindow() == bloomFactorSlider_)
        material.setParameter("bloomFactor", bloomFactorSlider_->getValue());
    else if (e.getWindow() == exposureSlider_)
        material.setParameter("exposure", exposureSlider_->getValue());

    // Update the labels displaying the current values being used
    brightThresholdValueLabel_->setText(brightThresholdSlider_->getValue());
    blurScaleValueLabel_->setText(blurScaleSlider_->getValue());
    blurStandardDeviationValueLabel_->setText(blurStandardDeviationSlider_->getValue());
    focalLengthValueLabel_->setText(focalLengthSlider_->getValue());
    focalRangeValueLabel_->setText(focalRangeSlider_->getValue());
    bloomFactorValueLabel_->setText(bloomFactorSlider_->getValue());
    exposureValueLabel_->setText(exposureSlider_->getValue());

    brightThresholdValueLabel_->autosize();
    blurScaleValueLabel_->autosize();
    blurStandardDeviationValueLabel_->autosize();
    focalLengthValueLabel_->autosize();
    focalRangeValueLabel_->autosize();
    bloomFactorValueLabel_->autosize();
    exposureValueLabel_->autosize();

    return CARBON_APPLICATION_CLASS::onGUISliderChangedEvent(e);
}

bool PostProcessSample::onMouseButtonDownEvent(const MouseButtonDownEvent& e)
{
    // The left-click and right-click drags that are used to manipulate the object and the camera are only allowed to be started
    // by a mouse button down event that does not happen over one of the gui windows, as otherwise the act of altering a slider
    // would also be interpreted as a left-click drag, which is not the desired behavior.

    if (e.getButton() == LeftMouseButton)
        inObjectDrag_ = !gui_.intersect(e.getPosition());
    else if (e.getButton() == RightMouseButton)
        inCameraDrag_ = !gui_.intersect(e.getPosition());

    return CARBON_APPLICATION_CLASS::onMouseButtonDownEvent(e);
}

bool PostProcessSample::onMouseButtonUpEvent(const MouseButtonUpEvent& e)
{
    if (e.getButton() == LeftMouseButton)
        inObjectDrag_ = false;
    else if (e.getButton() == RightMouseButton)
        inCameraDrag_ = false;

    return CARBON_APPLICATION_CLASS::onMouseButtonUpEvent(e);
}

void PostProcessSample::createScene()
{
    // Add a camera so we can see and move around
    camera_ = scene_.addEntity<Camera>();
    camera_->move({0.0f, 0.0f, 200.0f});

    // Add sky dome
    scene_.addEntity<SkyDome>()->setMaterial("Sunset");

    // Add object to manipulate
    object_ = scene_.addEntity<Entity>();
    object_->attachMesh(ObjectMesh);

    // Setup lighting
    scene_.addEntity<Light>()->setDirectionalLight(Color::White, Vec3(0.0f, -0.707f, -0.707f));

    scene_.precache();
}

void PostProcessSample::createHUD()
{
    hud_.setName("HUD");
    hud_.setIs2D(true);

    // Logo texture
    auto logo = hud_.addEntity<Sprite>("Logo", 64.0f, 64.0f);
    logo->setSpriteTexture("CarbonLogo.png");
    logo->alignToScreen(GUIWindow::ScreenTopLeft, Vec2(5.0f, -5.0f));

    // Mouse pointer
    hud_.addEntity<GUIMousePointer>("MousePointer", 32.0f, 32.0f);
}

void PostProcessSample::createGUI()
{
    gui_.setName("GUI");
    gui_.setIs2D(true);

    // Info window

    auto infoText =
        UnicodeString("Use the dropdown box to select a material and then adjust it using the provided controls.\n\n"
                      "Rotate the object with left-click drag.\n"
                      "Rotate the camera with right-click drag.\n\n");

    if (renderer().isHDRSupported())
        infoText += "Note: HDR is supported on this hardware.";
    else
        infoText += "Note: HDR is not supported on this hardware.";

    auto info = gui_.addEntity<GUIWindow>("", 280.0f, 170.0f, 5.0f, 30.0f, infoText);
    info->setTextMargins(7.0f);

    // List of post-process materials
    auto materials = Vector<UnicodeString>{"PassThrough", "Color", "Blur", "BrightPass", "DepthOfField", "Bloom"};

    // Create material combobox
    gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 300.0f, 175.0f, "Current material:");
    materialCombobox_ = gui_.addEntity<GUICombobox>("", 200.0f, 25.0f, 450.0f, 175.0f, materials);

    // Brightpass controls
    brightThresholdLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 300.0f, 130.0f, "Bright threshold:");
    brightThresholdSlider_ = gui_.addEntity<GUISlider>("", 300.0f, 20.0f, 450.0f, 130.0f, 0.5f, 1.5f);
    brightThresholdValueLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 770.0f, 130.0f);

    // Blur controls
    blurTypeLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 300.0f, 130.0f, "Blur type:");
    blurTypeCombobox_ = gui_.addEntity<GUICombobox>("", 300.0f, 25.0f, 450.0f, 130.0f);
    blurTypeCombobox_->onItemSelectEvent.addHandler(this, &PostProcessSample::onBlurTypeComboboxItemSelect);
    blurTypeCombobox_->addItems({"horizontal", "vertical", "2D"});

    blurScaleLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 300.0f, 105.0f, "Blur scale:");
    blurScaleSlider_ = gui_.addEntity<GUISlider>("", 300.0f, 20.0f, 450.0f, 105.0f, 0.0f, 10.0f);
    blurScaleValueLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 770.0f, 105.0f);
    blurStandardDeviationLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 300.0f, 80.0f, "Blur std dev:");
    blurStandardDeviationSlider_ = gui_.addEntity<GUISlider>("", 300.0f, 20.0f, 450.0f, 80.0f, 0.01f, 10.0f);
    blurStandardDeviationValueLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 770.0f, 80.0f, "");

    // Depth of field controls
    focalLengthLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 300.0f, 55.0f, "Focal length:");
    focalLengthSlider_ = gui_.addEntity<GUISlider>("", 300.0f, 20.0f, 450.0f, 55.0f, 0.0f, 500.0f);
    focalLengthValueLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 770.0f, 55.0f);
    focalRangeLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 300.0f, 30.0f, "Focal range:");
    focalRangeSlider_ = gui_.addEntity<GUISlider>("", 300.0f, 20.0f, 450.0f, 30.0f, 0.0f, 500.0f);
    focalRangeValueLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 770.0f, 30.0f, "");

    // Bloom controls
    bloomFactorLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 300.0f, 55.0f, "Bloom factor:");
    bloomFactorSlider_ = gui_.addEntity<GUISlider>("", 300.0f, 20.0f, 450.0f, 55.0f, 0.0f, 2.5f);
    bloomFactorValueLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 770.0f, 55.0f);

    // Exposure controls
    exposureLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 300.0f, 30.0f, "Exposure:");
    exposureSlider_ = gui_.addEntity<GUISlider>("", 300.0f, 20.0f, 450.0f, 30.0f, 0.0f, 2.0f);
    exposureValueLabel_ = gui_.addEntity<GUILabel>("", 0.0f, 0.0f, 770.0f, 30.0f);

    // Setup handler for the postprocess material being changed
    materialCombobox_->onItemSelectEvent.addHandler(this, &PostProcessSample::onMaterialComboboxItemSelect);
    materialCombobox_->onItemSelectEvent.fireWith(materialCombobox_, 0);
}

void PostProcessSample::setPostProcessMaterial(const String& name)
{
    scene_.clearPostProcessMaterials();
    if (name.length())
        scene_.addPostProcessMaterial(name);

    // Update the controller GUI that is being shown based on the post process effect of the chosen material

    auto& material = materials().getMaterial(name);
    auto& effect = material.getEffectName();

    auto visible = (effect == "PostProcessBrightPass" || effect == "PostProcessBloom");
    brightThresholdLabel_->setVisible(visible);
    brightThresholdSlider_->setVisible(visible);
    brightThresholdValueLabel_->setVisible(visible);

    visible = (effect == "PostProcessBlur" || effect == "PostProcessBloom" || effect == "PostProcessDepthOfField");
    blurScaleLabel_->setVisible(visible);
    blurScaleSlider_->setVisible(visible);
    blurScaleValueLabel_->setVisible(visible);
    blurStandardDeviationLabel_->setVisible(visible);
    blurStandardDeviationSlider_->setVisible(visible);
    blurStandardDeviationValueLabel_->setVisible(visible);

    visible = (effect == "PostProcessBlur");
    blurTypeLabel_->setVisible(visible);
    blurTypeCombobox_->setVisible(visible);

    visible = (effect == "PostProcessDepthOfField");
    focalLengthLabel_->setVisible(visible);
    focalLengthSlider_->setVisible(visible);
    focalLengthValueLabel_->setVisible(visible);
    focalRangeLabel_->setVisible(visible);
    focalRangeSlider_->setVisible(visible);
    focalRangeValueLabel_->setVisible(visible);

    visible = (effect == "PostProcessBloom");
    bloomFactorLabel_->setVisible(visible);
    bloomFactorSlider_->setVisible(visible);
    bloomFactorValueLabel_->setVisible(visible);
    exposureLabel_->setVisible(visible);
    exposureSlider_->setVisible(visible);
    exposureValueLabel_->setVisible(visible);

    // Set the controls to reflect the current parameter values being used on the new material

    brightThresholdSlider_->setValue(material.getParameter("brightThreshold").getFloat());
    blurTypeCombobox_->setText(material.getParameter("blurType").getString());
    blurScaleSlider_->setValue(material.getParameter("blurScale").getFloat());
    blurStandardDeviationSlider_->setValue(material.getParameter("blurStandardDeviation").getFloat());
    focalLengthSlider_->setValue(material.getParameter("focalLength").getFloat());
    focalRangeSlider_->setValue(material.getParameter("focalRange").getFloat());
    bloomFactorSlider_->setValue(material.getParameter("bloomFactor").getFloat());
    exposureSlider_->setValue(material.getParameter("exposure").getFloat());
}

#define CARBON_ENTRY_POINT_CLASS PostProcessSample
#include "CarbonEngine/EntryPoint.h"
