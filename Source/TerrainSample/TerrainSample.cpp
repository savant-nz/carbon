/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "TerrainSample.h"

bool TerrainSample::initialize()
{
    splashScreen_.addLogo("CarbonLogo.png");

    createHUD();
    createScene();

    return true;
}

void TerrainSample::frameUpdate()
{
    if (platform().isKeyPressed(KeyUpArrow))
        scene_.getEntity<Light>("Sun")->rotateAroundX(platform().getSecondsPassed());

    if (platform().isKeyPressed(KeyDownArrow))
        scene_.getEntity<Light>("Sun")->rotateAroundX(-platform().getSecondsPassed());
}

void TerrainSample::queueScenes()
{
    if (splashScreen_.update())
        return;

    scene_.queueForRendering();
    hud_.queueForRendering();
}

void TerrainSample::createScene()
{
    // Create terrain
    auto terrain = scene_.addEntity<Terrain>();
    terrain->createFromTexture("TerrainHeightmap.png");
    terrain->setMaterial("Terrain");
    terrain->setTextureScale(1.0f / terrain->getHeightmapWidth());

    // Create a camera so we can see and move around
    camera_ = scene_.addEntity<Camera>();
    camera_->move({200.0f, 40.0f, 200.0f});
    camera_->rotateAroundY(Math::Pi * 0.75f);
    camera_->addController<PlayerEntityController>()->setMovementAcceleration(200.0f);

    // Add sky dome
    scene_.addEntity<SkyDome>()->setMaterial("ScatteringSky");

    scene_.addPostProcessMaterial("PostProcess/Scattering");

    // Setup lighting
    scene_.addEntity<Light>("Sun")->setDirectionalLight(Color::White, Vec3(0.707f, -0.707f, 0.0f));
    scene_.addEntity<Light>()->setAmbientLight(Color(0.2f));

    scene_.precache();
}

void TerrainSample::createHUD()
{
    hud_.setName("HUD");
    hud_.setIs2D(true);

    // Logo texture
    auto logo = hud_.addEntity<Sprite>("Logo", 64.0f, 64.0f);
    logo->setSpriteTexture("CarbonLogo.png");
    logo->alignToScreen(GUIWindow::ScreenTopLeft, Vec2(5.0f, -5.0f));

    // Add info label
    auto info = hud_.addEntity<GUIWindow>("Info", 300.0f, 35.0f);
    info->move({5.0f, 30.0f});
    info->setText("This sample demonstrates infinite terrain using geometry\n"
                  "clipmapping, as well as post-process atmospheric scattering.");
    info->setTextMargins(7.0f);
    info->autosize();
}

#define CARBON_ENTRY_POINT_CLASS TerrainSample
#include "CarbonEngine/EntryPoint.h"
