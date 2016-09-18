/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * This timer reports its gathered timing information once it falls out of scope, and the scope usually matches up to
 * the task being timed. This class is useful when gathering performance data on code that is run relatively
 * infrequently, e.g. level loading rather than run every frame.
 */
class CARBON_API ScopedTimer
{
public:

    /**
     * Initializes this scoped timer with a summary string that should identify the overall task being timed in the
     * timer's scope. Optionally the time can be split into multiple tasks to get finer grained reporting, if this is
     * desired then details about the first task should be passed as the second constructor parameter.
     */
    ScopedTimer(String summary, const String& initialTask = String::Empty) : summary_(std::move(summary))
    {
        startTask(initialTask);
    }

    ~ScopedTimer();

    /**
     * Changes to a new task, this finishes the timing of the previous task and starts timing a new one. When this
     * scoped timer falls out of scope it will report the total time that it was alive and split that up amongst the
     * various tasks that were specified using ScopedTimer::startTask() in order to give more detailed information about
     * where the time was spent.
     */
    void startTask(const String& taskName);

private:

    String summary_;

    struct Task
    {
        String name;
        TimeValue startTime;

        Task() {}
        Task(String name_, TimeValue startTime_) : name(std::move(name_)), startTime(startTime_) {}
    };

    Vector<Task> tasks_;
};

}
