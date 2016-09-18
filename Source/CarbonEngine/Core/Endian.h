/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * This class contains helper methods for converting data between big and little endian formats. All binary file formats
 * have their data persisted in little endian format automatically by the FileReader and FileWriter classes, and this
 * class is used to perform any endianness conversions needed.
 */
class CARBON_API Endian
{
public:

    /**
     * Changes the endianness of the passed value.
     */
    static void convert(short& value) { convertScalar(value); }

    /**
     * Changes the endianness of the passed value.
     */
    static void convert(unsigned short& value) { convertScalar(value); }

    /**
     * Changes the endianness of the passed value.
     */
    static void convert(int& value) { convertScalar(value); }

    /**
     * Changes the endianness of the passed value.
     */
    static void convert(unsigned int& value) { convertScalar(value); }

    /**
     * Changes the endianness of the passed value.
     */
    static void convert(char32_t& value) { convertScalar(value); }

    /**
     * Changes the endianness of the passed value.
     */
    static void convert(float& value) { convertScalar(value); }

    /**
     * Changes the endianness of the passed value.
     */
    static void convert(double& value) { convertScalar(value); }

    /**
     * Changes the endianness of all the items in the given array.
     */
    template <typename T> static void convertArray(T* t, unsigned int size)
    {
        for (auto i = 0U; i < size; i++)
            convert(t[i]);
    }

    /**
     * Reverses the ordering of the bit pairs in the specified byte, i.e. 01234567 becomes 67452301.
     */
    static void reverseBitPairs(byte_t& b)
    {
        b = byte_t((b & 0x03) << 6) | byte_t((b & 0x0C) << 2) | byte_t((b & 0x30) >> 2) | byte_t((b & 0xC0) >> 6);
    }

private:

    template <typename T> static void convertScalar(T& t)
    {
        auto data = reinterpret_cast<byte_t*>(&t);

        std::reverse(data, data + sizeof(t));
    }
};

}
