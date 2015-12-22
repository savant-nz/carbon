/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Simple exception class that holds an error string.
 */
class CARBON_EXCEPTION_API Exception
{
public:

    Exception() {}

    /**
     * Constructs this exception with the specified error message.
     */
    explicit Exception(UnicodeString error) : error_(std::move(error)) {}

    /**
     * Returns the error description of this exception.
     */
    const UnicodeString& get() const { return error_; }

    /**
     * Stream concatenation onto this Exception's error string.
     */
    template <typename T> Exception& operator<<(T&& arg)
    {
        error_ << std::forward<T>(arg);
        return *this;
    }

    /**
     * Automatic conversion to a UnicodeString.
     */
    operator const UnicodeString&() const { return error_; }

private:

    UnicodeString error_;
};

}
