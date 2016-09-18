/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Globals.h"

namespace Carbon
{

/**
 * Static class that manages the registered shader classes. Each shader has a static instance that is created and
 * registered using the CARBON_REGISTER_SHADER() macro.
 */
class CARBON_API ShaderRegistry : private Noncopyable
{
public:

    /**
     * When a shader instance is registered with ShaderRegistry::registerShader() a function is supplied that can be
     * called to query whether the shader can be used with the currently active graphics interface. This function is
     * used to filter the list of shaders returned by ShaderRegistry::getShadersForEffect().
     */
    typedef std::function<bool()> IsSupportedByGraphicsInterfaceFunction;

    /**
     * Registers a shader instance.
     */
    static void registerShader(Shader* shader, const String& className,
                               IsSupportedByGraphicsInterfaceFunction fnIsSupportedByGraphicsInterface);

    /**
     * Unregisters the given shader instance.
     */
    static void unregisterShader(Shader* shader);

    /**
     * Returns the shaders that are registered for the given effect and which can be used with the active graphics
     * interface.
     */
    static Vector<Shader*> getShadersForEffect(const String& effectName);
};

/**
 * \file
 */

/**
 * This macro automatically registers a shader class for use, and ties it to a specific graphics interface subclass.
 */
#define CARBON_REGISTER_SHADER(ShaderClass, GraphicsInterfaceSubclass)                                     \
    namespace Shader##ShaderClass                                                                          \
    {                                                                                                      \
        static ShaderClass shaderInstance;                                                                 \
        static bool isSupportedByGraphicsInterface()                                                       \
        {                                                                                                  \
            return dynamic_cast<GraphicsInterfaceSubclass*>(&graphics()) != nullptr;                       \
        }                                                                                                  \
        static void registerShader()                                                                       \
        {                                                                                                  \
            ShaderRegistry::registerShader(&shaderInstance, #ShaderClass, isSupportedByGraphicsInterface); \
        }                                                                                                  \
        static void unregisterShader() { ShaderRegistry::unregisterShader(&shaderInstance); }              \
        CARBON_REGISTER_STARTUP_FUNCTION(registerShader, 0)                                                \
        CARBON_REGISTER_SHUTDOWN_FUNCTION(unregisterShader, 0)                                             \
    }
}
