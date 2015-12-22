/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Math/RandomNumberGenerator.h"

namespace Carbon
{

unsigned int RandomNumberGenerator::run()
{
    static auto generator = std::mt19937(std::random_device()());
    static auto distribution = std::uniform_int_distribution<unsigned int>(0U, UINT_MAX);

    return distribution(generator);
}

}
