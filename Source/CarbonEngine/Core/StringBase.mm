/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

namespace Carbon
{

template <> StringBase<char>::StringBase(const NSString* nsString) : StringBase()
{
    if (nsString)
        *this = A(fromUTF8([nsString UTF8String]));
}

template <> StringBase<UnicodeCharacter>::StringBase(const NSString* nsString) : StringBase()
{
    if (nsString)
    {
        auto utf8 = [nsString UTF8String];

        *this = fromUTF8(reinterpret_cast<const byte_t*>(utf8), uint(strlen(utf8)));
    }
}

}
