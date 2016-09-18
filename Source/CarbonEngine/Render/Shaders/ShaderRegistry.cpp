/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Render/Shaders/Shader.h"
#include "CarbonEngine/Render/Shaders/ShaderRegistry.h"

namespace Carbon
{

// Holds details on a registered shader instance
struct RegisteredShader
{
    Shader* shader = nullptr;
    ShaderRegistry::IsSupportedByGraphicsInterfaceFunction fnIsSupportedByGraphicsInterface = nullptr;

    RegisteredShader() {}
    RegisteredShader(Shader* shader_,
                     ShaderRegistry::IsSupportedByGraphicsInterfaceFunction fnIsSupportedByGraphicsInterface_)
        : shader(shader_), fnIsSupportedByGraphicsInterface(fnIsSupportedByGraphicsInterface_)
    {
    }
};

static Vector<RegisteredShader> registeredShaders;

void ShaderRegistry::registerShader(Shader* shader, const String& className,
                                    IsSupportedByGraphicsInterfaceFunction fnIsSupportedByGraphicsInterface)
{
    shader->className_ = className;
    registeredShaders.emplace(shader, fnIsSupportedByGraphicsInterface);
}

void ShaderRegistry::unregisterShader(Shader* shader)
{
    registeredShaders.eraseIf([&](const RegisteredShader& r) { return r.shader == shader; });
}

Vector<Shader*> ShaderRegistry::getShadersForEffect(const String& effectName)
{
    auto shaders = Vector<Shader*>();

    for (auto& r : registeredShaders)
    {
        if (r.shader->getEffectName() == effectName && r.fnIsSupportedByGraphicsInterface())
            shaders.append(r.shader);
    }

    return shaders;
}

}
