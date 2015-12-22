/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/Parameter.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/TimeValue.h"

namespace Carbon
{

int64_t TimeValue::ticksPerSecond_;

TimeValue::TimeValue(const Parameter& parameter) : ticks_(parameter.getInteger64())
{
}

TimeValue::operator Parameter() const
{
    return ticks_;
}

TimeValue TimeValue::getTimeSince() const
{
    return platform().getTime() - *this;
}

}
