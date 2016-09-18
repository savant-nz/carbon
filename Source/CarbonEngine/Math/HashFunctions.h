/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Wrapper around the DJB hashing function.
 */
class CARBON_API HashFunctions
{
public:

    /**
     * Hashing algorithm by Daniel J Bernstein.
     */
    static unsigned int djbHash(const void* p, unsigned int size);

    /**
     * Returns a DJB hash of the passed data type.
     */
    static unsigned int hash(const void* p, unsigned int size) { return djbHash(p, size); }

    /**
     * Returns a hash of the raw data contained in the passed vector.
     */
    template <typename T> static unsigned int hash(const Vector<T>& data)
    {
        return hash(data.getData(), data.getDataSize());
    }

    /**
     * Returns a hash of the raw data contained in the passed vector.
     */
    template <typename T> static unsigned int hash(const T& data) { return hash(&data, sizeof(data)); }
};

}
