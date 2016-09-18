/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "BoxesSample.h"

bool BoxesSample::initialize()
{
    splashScreen_.addLogo("CarbonLogo.png");

    createScene();
    createHUD();

    return true;
}

void BoxesSample::frameUpdate()
{
}

void BoxesSample::queueScenes()
{
    if (splashScreen_.update())
        return;

    scene_.queueForRendering();
    hud_.queueForRendering();
}

bool BoxesSample::onMouseButtonDownEvent(const MouseButtonDownEvent& e)
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
        createHangingBoxes();
    }

    return CARBON_APPLICATION_CLASS::onMouseButtonDownEvent(e);
}

void BoxesSample::createScene()
{
    scene_.load("Boxes");
    scene_.makePhysical();

    createCamera();
    createSkyDome();
    createBoxStack();
    createHangingBoxes();
    createLights();

    scene_.precache();
}

void BoxesSample::createCamera()
{
    camera_ = scene_.addEntity<Camera>();
    camera_->move({-5.0f, 5.0f, -15.0f});
    camera_->rotateAroundY(Math::Pi * 0.75f);
    camera_->addController<PlayerEntityController>();
}

void BoxesSample::createSkyDome()
{
    scene_.addEntity<SkyDome>()->setMaterial("Sunset");
}

void BoxesSample::createLights()
{
    scene_.addEntity<Light>()->setDirectionalLight(Color::White, Vec3(0.707f, -0.707f, 0.0f));
    scene_.addEntity<Light>()->setAmbientLight(Color(0.5f));
}

Entity* BoxesSample::createBox(const Vec3& position)
{
    auto box = scene_.addEntity<Entity>();
    boxes_.append(box);

    box->attachMesh("Box");
    box->move(position);

    // Make box colorful
    box->setMaterialOverrideParameter("diffuseColor", (Color::randomRGB() + Color::White) * 0.5f);

    return box;
}

void BoxesSample::removeBoxes()
{
    for (auto box : boxes_)
        box->removeFromScene();

    boxes_.clear();
}

void BoxesSample::createBoxStack()
{
    for (auto x = 0U; x < 5; x++)
    {
        for (auto y = 0U; y < 5; y++)
        {
            for (auto z = 0U; z < 5; z++)
            {
                auto box = createBox(Vec3(0.5f) + Vec3(float(x), float(y), float(z)) * Vec3(2.0f, 1.0f, 2.0f));

                box->makePhysical(1.0f);
            }
        }
    }
}

void BoxesSample::createHangingBoxes()
{
    auto hangingBoxPositions =
        Vector<Vec3>{{18.0f, 0.0f, 3.0f}, {16.0f, 0.0f, 5.0f}, {20.0f, 0.0f, 5.0f}, {18.0f, 0.0f, 7.0f}};

    for (auto& hangingBoxPosition : hangingBoxPositions)
    {
        for (auto y = 0U; y < 5; y++)
        {
            auto box = createBox(hangingBoxPosition + Vec3(0.0f, float(y + 1) * 1.5f));

            // The topmost box is fixed in place
            box->makePhysical(1.0f, y == 4);

            // Add hinges
            if (y)
            {
                physics().createBallAndSocketJoint(box->getRigidBody(), boxes_[boxes_.size() - 2]->getRigidBody(),
                                                   box->getWorldPosition() - Vec3::UnitY);
            }
        }
    }
}

void BoxesSample::createHUD()
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
    info->setText("Press the left mouse button to move the boxes.\n\n"
                  "Press the right mouse button to reset the simulation.");
    info->setTextMargins(7.0f);
    info->autosize();
}

#define CARBON_ENTRY_POINT_CLASS BoxesSample
#include "CarbonEngine/EntryPoint.h"
