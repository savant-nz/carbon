/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ImpostersSample.h"

const auto ImposterTextureName = String("Imposter");
const auto ImposterTextureSize = 256U;

bool ImpostersSample::initialize()
{
    splashScreen_.addLogo("CarbonLogo.png");

    createHUD();
    createScene();

    return true;
}

void ImpostersSample::createScene()
{
    // Create the scene that will be used to render the imposter texture and place a camera into it
    imposterScene_.setName("Imposter");
    imposterScene_.addEntity<Camera>();

    // Create the teapot entity and put it into the imposter scene
    teapot_ = imposterScene_.addEntity<Entity>();
    teapot_->attachMesh("Teapot");
    teapot_->move({0.0f, 0.0f, -125.0f});

    // Put an ambient and directional light into the imposter scene to light the teapot
    imposterScene_.addEntity<Light>()->setAmbientLight(Color(0.25f));
    imposterScene_.addEntity<Light>()->setDirectionalLight(Color(0.5f), Vec3(-1.0f, -1.0f, -1.0f));

    // Create the texture for the imposter
    imposterTexture_ = textures().create2DTexture(ImposterTextureName, ImposterTextureSize, ImposterTextureSize, true);

    // Create the scene which will hold a bunch of sprites with the imposter texture
    scene_.setName("Sprites");
    auto camera = scene_.create2DCamera(0.0f, 1.0f);

    // Create a 2D grid of sprites which each display the rendered imposter texture
    auto ySprites = 10U;
    auto xSprites = uint(camera->getDefaultAspectRatio() * ySprites);

    for (auto x = 0U; x < xSprites; x++)
    {
        for (auto y = 0U; y < ySprites; y++)
        {
            auto s = scene_.addEntity<Sprite>("", 0.1f, 0.1f, x * 0.1f + 0.05f, y * 0.1f + 0.05f);

            s->setSpriteTexture(ImposterTextureName);
            s->setSpriteDiffuseColor(Color::randomRGB().normalized());
        }
    }

    scene_.center();
    scene_.precache();
}

void ImpostersSample::createHUD()
{
    hud_.setName("HUD");
    hud_.setIs2D(true);

    // Logo texture
    auto logo = hud_.addEntity<Sprite>("Logo", 64.0f, 64.0f);
    logo->setSpriteTexture("CarbonLogo.png");
    logo->alignToScreen(GUIWindow::ScreenTopLeft, Vec2(5.0f, -5.0f));
}

void ImpostersSample::frameUpdate()
{
    // Rotate the teapot
    teapot_->rotateAroundY(platform().getSecondsPassed() * Math::QuarterPi);
    teapot_->rotateAroundZ(platform().getSecondsPassed() * Math::QuarterPi * 0.25f);

    // Update imposter texture
    imposterScene_.renderToTexture(imposterTexture_);
}

void ImpostersSample::queueScenes()
{
    if (splashScreen_.update())
        return;

    scene_.queueForRendering();
    hud_.queueForRendering();
}

void ImpostersSample::shutdown()
{
    textures().releaseTexture(imposterTexture_);
    imposterTexture_ = nullptr;
}

#define CARBON_ENTRY_POINT_CLASS ImpostersSample
#include "CarbonEngine/EntryPoint.h"
