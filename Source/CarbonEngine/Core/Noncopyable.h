/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Classes that don't permit copying can inherit from this class in order to cause compilation errors if any code tries to use
 * their assignment operator or copy constructor.
 */
class CARBON_API Noncopyable
{
public:

    Noncopyable() {}
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(Noncopyable) = delete;
};

}
