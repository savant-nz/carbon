/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Graphics/States/States.h"
#include "CarbonEngine/Render/Shaders/Blending.h"

namespace Carbon
{

namespace Shaders
{

const auto DefaultSourceFactor = Parameter(States::SourceAlpha);
const auto DefaultDestinationFactor = Parameter(States::OneMinusSourceAlpha);

bool Blending::isPresent(const ParameterArray& params)
{
    return params[Parameter::blend].getBoolean();
}

void Blending::setShaderParams(const ParameterArray& params)
{
    if (params[Parameter::blend].getBoolean())
    {
        States::BlendEnabled = true;
        States::BlendFunction = {
            States::BlendFactor(params.get(Parameter::blendSourceFactor, DefaultSourceFactor).getInteger()),
            States::BlendFactor(params.get(Parameter::blendDestinationFactor, DefaultDestinationFactor).getInteger())};

        States::DepthWriteEnabled = params[Parameter::depthWrite].getBoolean();
    }
    else
    {
        States::BlendEnabled = false;
        States::DepthWriteEnabled = true;
    }
}

}

}
