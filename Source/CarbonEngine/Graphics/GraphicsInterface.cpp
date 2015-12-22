/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/InterfaceRegistry.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Graphics/iOS/iOSOpenGLES2.h"
#include "CarbonEngine/Graphics/OpenGL11/OpenGL11.h"
#include "CarbonEngine/Graphics/OpenGL41/OpenGL41.h"
#include "CarbonEngine/Graphics/OpenGLES2/OpenGLES2.h"
#include "CarbonEngine/Graphics/States/StateCacher.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

CARBON_DEFINE_INTERFACE_REGISTRY(GraphicsInterface)
{
    return i->isSupported();
}

typedef GraphicsInterface NullInterface;
CARBON_REGISTER_INTERFACE_IMPLEMENTATION(GraphicsInterface, NullInterface, 0)

#ifdef CARBON_INCLUDE_OPENGL11
    CARBON_REGISTER_INTERFACE_IMPLEMENTATION(GraphicsInterface, OpenGL11, 100)
#endif
#ifdef CARBON_INCLUDE_OPENGLES2
    CARBON_REGISTER_INTERFACE_IMPLEMENTATION(GraphicsInterface, OpenGLES2, 200)
#endif
#ifdef CARBON_INCLUDE_OPENGL41
    CARBON_REGISTER_INTERFACE_IMPLEMENTATION(GraphicsInterface, OpenGL41, 300)
#endif
#ifdef iOS
    CARBON_REGISTER_INTERFACE_IMPLEMENTATION(GraphicsInterface, iOSOpenGLES2, 400)
#endif

bool GraphicsInterface::setup()
{
    States::StateCacher::setup();

    drawCallCount_ = 0;
    triangleCount_ = 0;
    apiCallCount_ = 0;

    return true;
}

Rect GraphicsInterface::getOutputDestinationViewport(OutputDestination destination) const
{
    if (destination == OutputDefault)
        return platform().getWindowRect();

    return {};
}

}
