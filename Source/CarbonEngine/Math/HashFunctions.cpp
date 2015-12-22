/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/HashFunctions.h"

namespace Carbon
{

unsigned int HashFunctions::djbHash(const void* p, unsigned int size)
{
    auto data = reinterpret_cast<const byte_t*>(p);

    auto hash = 5381U;

    for (auto i = 0U; i < size; i++, data++)
        hash = (hash << 5) + hash + *data;

    return hash;
}

}
