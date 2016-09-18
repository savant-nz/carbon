/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Graphics/GraphicsInterface.h"

namespace Carbon
{

/**
 * Empty class that is subclassed by OpenGLShared which is then subclassed by all OpenGL graphics backends. This is
 * mainly used to register GLSL-based shaders that are able to be used by any OpenGL backend, e.g. both OpenGL11 and
 * OpenGLES2.
 */
class CARBON_API OpenGLBase : public GraphicsInterface
{
};

}
