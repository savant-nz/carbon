/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "DeferredLightingSample.h"

bool DeferredLightingSample::initialize()
{
    if (!renderer().isDeferredLightingSupported())
    {
        LOG_ERROR << "Graphics hardware does not support deferred lighting.";
        return false;
    }

    splashScreen_.addLogo("CarbonLogo.png");
    createHUD();
    createScene();

    return true;
}

void DeferredLightingSample::frameUpdate()
{
    if (isSpotlightAnimating_)
        spotLight_->rotate(Quaternion::createRotationY(platform().getSecondsPassed() * 2.0f));
}

void DeferredLightingSample::queueScenes()
{
    if (splashScreen_.update())
        return;

    scene_.queueForRendering();
    hud_.queueForRendering();
}

bool DeferredLightingSample::onMouseButtonDownEvent(const MouseButtonDownEvent& e)
{
    if (e.getButton() == LeftMouseButton)
    {
        // Clicking on a box causes a force to be applied to it

        auto intersection = scene_.intersect(platform().getWindowMiddle());
        if (intersection && boxes_.has(intersection.getEntity()))
            intersection.getEntity()->applyWorldForce(camera_->getDirection() * 10.0f, PhysicsInterface::ForceImpulse);
    }
    else if (e.getButton() == RightMouseButton)
    {
        removeBoxes();
        createBoxStack();

        for (auto light : userLights_)
            light->removeFromScene();

        userLights_.clear();
    }

    return CARBON_APPLICATION_CLASS::onMouseButtonDownEvent(e);
}

bool DeferredLightingSample::onKeyDownEvent(const KeyDownEvent& e)
{
    if (e.getKey() == KeySpacebar)
    {
        // Create a new light
        auto newLight = scene_.addEntity<Light>();
        newLight->setPointLight(Color::random().normalized(),
                                camera_->localToWorld({0, 0, 1.25f}),    // Place the light just in front of the camera
                                Math::random(1.0f, 3.0f));

        // Add a colored box to show the position of the light source
        newLight->attachMesh("Box");
        newLight->setMeshScale(0.1f);
        newLight->setMaterialOverrideParameter("diffuseColor", newLight->getColor());

        userLights_.append(newLight);
    }

    if (e.getKey() == KeyL)
        isSpotlightAnimating_ = !isSpotlightAnimating_;

    return CARBON_APPLICATION_CLASS::onKeyDownEvent(e);
}

void DeferredLightingSample::createScene()
{
    // Create the scene
    scene_.load("Boxes");
    scene_.makePhysical();

    // Enable deferred lighting on the scene
    scene_.setDeferredLightingEnabled(true);

    // Ambient light
    scene_.addEntity<Light>()->setAmbientLight(Color(0.2f));

    // Directional light with shadows
    auto directionalLight = scene_.addEntity<Light>();
    directionalLight->setDirectionalLight(Color::White, Vec3(-0.5f, -1.0f, 0.3f));
    directionalLight->setShadowsEnabled(true);

    // Create spot light
    spotLight_ = scene_.addEntity<Light>();
    spotLight_->setSpotLight(Color(0.25f, 1.0f, 0.25f), Vec3(12.0f, 2.5f, 5.0f), 50.0f);
    spotLight_->setShadowsEnabled(true);
    spotLight_->setProjectionTextureName("CarbonLogo");

    // Add a box at the origin of the spotlight
    spotLight_->attachMesh("Box", Vec3(0.0f, 0.0f, -1.0f));
    spotLight_->setMeshScale({0.25f, 0.25f, 2.0f});
    spotLight_->setShadowCaster(true);

    // Create a camera so we can see and move around
    camera_ = scene_.addEntity<Camera>();
    camera_->move({15.0f, 5.0f, -10.0f});
    camera_->rotateAroundY(Math::Pi * -0.9f);
    camera_->addController<PlayerEntityController>();

    // Add a point light to the camera
    camera_->addChild<Light>()->setPointLight(Color::White, Vec3::Zero, 2.0f);

    // Add sky dome
    scene_.addEntity<SkyDome>()->setMaterial("Sunset");

    createBoxStack();

    scene_.precache();
}

Entity* DeferredLightingSample::createBox(const Vec3& position)
{
    auto box = scene_.addEntity<Entity>();
    boxes_.append(box);

    box->attachMesh("Box");
    box->move(position);

    // Make boxes colorful
    box->setMaterialOverrideParameter("diffuseColor", (Color::randomRGB() + Color::White) * 0.5f);

    box->makePhysical(1.0f);
    box->setShadowCaster(true);

    return box;
}

void DeferredLightingSample::removeBoxes()
{
    for (auto box : boxes_)
        box->removeFromScene();

    boxes_.clear();
}

void DeferredLightingSample::createBoxStack()
{
    for (auto x = 0U; x < 5; x++)
    {
        for (auto y = 0U; y < 5; y++)
        {
            for (auto z = 0U; z < 5; z++)
                createBox(Vec3(0.5f) + Vec3(float(x), float(y), float(z)) * Vec3(2.0f, 1.0f, 2.0f));
        }
    }
}

void DeferredLightingSample::createHUD()
{
    hud_.setName("HUD");
    hud_.setIs2D(true);

    // Logo texture
    auto logo = hud_.addEntity<Sprite>("Logo", 64.0f, 64.0f);
    logo->setSpriteTexture("CarbonLogo.png");
    logo->alignToScreen(GUIWindow::ScreenTopLeft, Vec2(5.0f, -5.0f));

    // Crosshair
    auto crosshair = hud_.addEntity<Sprite>("Crosshair", 31.0f, 31.0f);
    crosshair->setSpriteTexture("Crosshair.png");
    crosshair->alignToScreen(GUIWindow::ScreenMiddle);

    // Add info label
    auto info = hud_.addEntity<GUIWindow>("Info", 300.0f, 35.0f, Vec2(5.0f, 30.0f));
    info->setText("Press the left mouse button to knock the boxes over.\n"
                  "Press the right mouse button to reset the simulation.\n\n"
                  "Press space to drop a light just in front of the camera.\n"
                  "Press L to toggle the spotlight animation.");
    info->setTextMargins(7.0f);
    info->autosize();
}

#define CARBON_ENTRY_POINT_CLASS DeferredLightingSample
#include "CarbonEngine/EntryPoint.h"
