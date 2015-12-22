/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/Runnable.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

Runnable Runnable::Empty;

String Runnable::getTaskString() const
{
    auto lock = ScopedMutexLock(mutex_);

    auto result = String();

    for (auto& task : taskStack_)
    {
        if (!task.name.length())
            continue;

        if (result.length())
            result << " - ";

        result << task.name;
    }

    isTaskStringDirty_ = false;

    return result;
}

String Runnable::getSimpleTaskString() const
{
    auto lock = ScopedMutexLock(mutex_);

    return taskStack_.size() > 1 ? taskStack_[1].name : String::Empty;
}

void Runnable::setTaskStringDirty()
{
    auto lock = ScopedMutexLock(mutex_);

    isTaskStringDirty_ = true;
}

void Runnable::beginTask(const String& task, float percentage)
{
    auto lock = ScopedMutexLock(mutex_);

    if (taskStack_.back().percentComplete + percentage > 100.1f)
    {
        if (this != &Runnable::Empty)
            LOG_WARNING << "Task percentages exceed 100";
    }

    isTaskStringDirty_ = true;
    taskStack_.emplace(task, percentage);
}

void Runnable::endTask()
{
    auto lock = ScopedMutexLock(mutex_);

    if (taskStack_.size() <= 1)
    {
        LOG_WARNING << "Attempt to end task when none exist";
        return;
    }

    isTaskStringDirty_ = true;
    isPercentageDirty_ = true;

    auto percentage = taskStack_.back().percentage;
    taskStack_.popBack();

    if (taskStack_.size())
        taskStack_.back().percentComplete += percentage;
}

bool Runnable::setTaskProgress(unsigned int done, unsigned int total)
{
    auto lock = ScopedMutexLock(mutex_);

    if (done > total)
        done = total;

    taskStack_.back().percentComplete = (float(done) / float(total)) * 100.0f;
    isPercentageDirty_ = true;

    return isCancelled();
}

float Runnable::getPercentage() const
{
    auto lock = ScopedMutexLock(mutex_);

    auto percent = 0.0f;
    auto percentWeight = 1.0f;

    for (auto& task : taskStack_)
    {
        percentWeight *= task.percentage / 100.0f;
        percent += task.percentComplete * percentWeight;
    }

    isPercentageDirty_ = false;

    return percent;
}

}
