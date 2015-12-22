/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/ScopedTimer.h"

namespace Carbon
{

ScopedTimer::~ScopedTimer()
{
#ifdef CARBON_INCLUDE_LOGGING
    if (tasks_.empty())
        return;

    auto currentTime = platform().getTime();
    auto totalTime = currentTime - tasks_[0].startTime;

    LOG_DEBUG << "Timed task '" << summary_ << "' took " << int(totalTime.toMilliseconds()) << " ms";

    if (tasks_.size() > 1)
    {
        auto longestTaskNameLength = 0U;
        for (auto& task : tasks_)
            longestTaskNameLength = std::max(longestTaskNameLength, task.name.length());

        for (auto i = 0U; i < tasks_.size(); i++)
        {
            auto start = tasks_[i].startTime;
            auto end = i + 1 < tasks_.size() ? tasks_[i + 1].startTime : currentTime;
            auto taskTime = end - start;

            LOG_DEBUG << "    " << (tasks_[i].name + ":").padToLength(longestTaskNameLength + 3)
                      << (String(int(taskTime.toMilliseconds())) + "ms").padToLength(10) << "("
                      << String::formatPercentage(taskTime.toSeconds(), totalTime.toSeconds()) << ")";
        }
    }
#endif
}

void ScopedTimer::startTask(const String& taskName)
{
    tasks_.emplace(taskName, platform().getTime());
}

}
