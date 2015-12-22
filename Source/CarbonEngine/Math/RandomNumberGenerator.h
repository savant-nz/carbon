/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Wrapper around C++11's random number generation functionality.
 */
class CARBON_API RandomNumberGenerator
{
public:

    /**
     * Generates a random integer in the range 0 - (2^32 - 1).
     */
    static unsigned int run();
};

}
