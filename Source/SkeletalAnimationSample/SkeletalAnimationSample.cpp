/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SkeletalAnimationSample.h"

bool SkeletalAnimationSample::initialize()
{
    splashScreen_.addLogo("CarbonLogo.png");

    createHUD();
    createScene();

    return true;
}

void SkeletalAnimationSample::createScene()
{
    // Create the scene
    scene_.load("Boxes");
    scene_.makePhysical();

    // Add a camera so we can see and move around
    camera_ = scene_.addEntity<Camera>();
    camera_->move({-3.0f, 10.0f, 25.0f});
    camera_->addController<PlayerEntityController>();

    // Add sky dome
    scene_.addEntity<SkyDome>()->setMaterial("Sunset");

    // Add skeletal mesh object
    skeletalMesh_ = scene_.addEntity<SkeletalMesh>();
    skeletalMesh_->addSkeletalMesh("Brawler");
    skeletalMesh_->addAnimation("BrawlerIdle", true);

    // Setup lighting
    scene_.addEntity<Light>()->setAmbientLight(Color(0.1f));
    scene_.addEntity<Light>()->setDirectionalLight(Color::White, Vec3(0.4f, -0.7f, -0.7f));

    scene_.precache();
}

void SkeletalAnimationSample::createHUD()
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
    auto info = hud_.addEntity<GUIWindow>("Info", 300.0f, 35.0f);
    info->move({5.0f, 30.0f});
    info->setText("Press the left mouse button to switch between animations.\n\n"
                  "Press R to toggle ragdoll, or K to toggle rendering of the skeleton.\n"
                  "Press B to toggle display of the bounding volume.");
    info->setTextMargins(7.0f);
    info->autosize();
}

void SkeletalAnimationSample::frameUpdate()
{
    scene_.clearImmediateGeometry();

    if (showSkeletalMeshBoundingVolume_)
        scene_.addImmediateGeometry(skeletalMesh_->getWorldAABB(), SimpleTransform::Identity, Color::Red);
}

void SkeletalAnimationSample::queueScenes()
{
    if (splashScreen_.update())
        return;

    scene_.queueForRendering();
    hud_.queueForRendering();
}

bool SkeletalAnimationSample::onKeyDownEvent(const KeyDownEvent& e)
{
    if (e.getKey() == KeyK)
    {
        // Pressing k toggles skeleton rendering
        skeletalMesh_->setDrawSkeletonEnabled(!skeletalMesh_->isDrawSkeletonEnabled());
    }
    else if (e.getKey() == KeyR)
    {
        // Pressing r toggles ragdoll
        if (!skeletalMesh_->isPhysical())
        {
            skeletalMesh_->removeAllAnimations();
            skeletalMesh_->makePhysical(10.0f, true);
        }
        else
        {
            skeletalMesh_->makeNotPhysical();
            skeletalMesh_->setReferencePose();
        }
    }
    else if (e.getKey() == KeyB)
        showSkeletalMeshBoundingVolume_ = !showSkeletalMeshBoundingVolume_;

    return CARBON_APPLICATION_CLASS::onKeyDownEvent(e);
}

bool SkeletalAnimationSample::onMouseButtonDownEvent(const MouseButtonDownEvent& e)
{
    if (e.getButton() == LeftMouseButton)
    {
        // Left mouse button toggles between the two animations

        skeletalMesh_->makeNotPhysical();

        if (skeletalMesh_->getAnimations().has("BrawlerIdle"))
            skeletalMesh_->setAnimation("BrawlerWalk", true);
        else
            skeletalMesh_->setAnimation("BrawlerIdle", true);
    }

    return CARBON_APPLICATION_CLASS::onMouseButtonDownEvent(e);
}

#define CARBON_ENTRY_POINT_CLASS SkeletalAnimationSample
#include "CarbonEngine/EntryPoint.h"
