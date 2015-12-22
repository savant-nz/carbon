/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Threads/Mutex.h"

namespace Carbon
{

/**
 * Defines an interface for a task that can be "run". Implementations of this class subclass the Runnable::run() method to carry
 * out a task. While this task is being carried out the other methods on its Runnable instance are used to check if the task has
 * been cancelled and should terminate, as well as keep users of this class up to date in regard to what is currently being
 * executed inside Runnable::run() and how much progress has been made. This class is intended to be used in combination with a
 * GUI and is fully thread-safe, it is used primarily to help with communication between a worker thread and a GUI thread.
 */
class CARBON_API Runnable : private Noncopyable
{
public:

    /**
     * Empty Runnable instance used as a default parameter to routines that have an optional 'Runnable &' parameter.
     */
    static Runnable Empty;

    Runnable() { taskStack_.emplace("", 100.0f); }

    /**
     * Copy constructor (not implemented).
     */
    Runnable(const Runnable&);

    virtual ~Runnable() {}

    /**
     * The entry point for the task to be carried out by this Runnable instance, should be implemented by subclasses.
     */
    virtual bool run() { return true; }

    /**
     * Returns whether the task executing in Runnable::run() has been asked to terminate for some reason such as the user
     * pressing cancel. The code in Runnable::run() should check this value frequently and exit immediately returning false if
     * it detects a cancellation.
     */
    bool isCancelled() const { return isCancelled_; }

    /**
     * Sets the cancelled state to true, see Runnable::isCancelled() for details.
     */
    void cancel() { isCancelled_ = true; }

    /**
     * Returns a string describing what the Runnable::run() method is currently executing.
     */
    String getTaskString() const;

    /**
     * Similar to Runnable::getTaskString() except that only the top level task is returned.
     */
    String getSimpleTaskString() const;

    /**
     * Returns whether the task string has changed since the last call to Runnable::getTaskString() or
     * Runnable::getSimpleTaskString().
     */
    bool isTaskStringDirty() const { return isTaskStringDirty_; }

    /**
     * Sets the task string state to dirty, the next call to Runnable::isTaskStringDirty() will return true.
     */
    void setTaskStringDirty();

    /**
     * This begins a new task that will occupy the given percentage of the currently active task. Tasks can be layered, for
     * example a first task that accounts for 20% of the total percentage for the Runnable::run() operation could be started,
     * and it internally could have two subtasks, one making up 25% of the parent task's 20% and the other making up 75% of the
     * parent task's 20%. These subtasks would run internally from 0 - 100% and all the accumulation of percentages up to parent
     * tasks is done automatically. Each call to Runnable::beginTask() must be paired with a call to Runnable::endTask(). There
     * is no limit to the level of subtask layering, but callers should ensure that at each level the subtasks have a combined
     * percentage of 100.
     */
    void beginTask(const String& task, float percentage);

    /**
     * Ends a task started with Runnable::beginTask().
     */
    void endTask();

    /**
     * Sets the percentage complete of the current task from the number of items to do and the number currently done. This
     * percentage is for the task most recently started with Runnable::beginTask() that has not been ended with
     * Runnable::endTask(). The return value is set to the current return value from Runnable::isCancelled() in order to allow
     * an update of task progress to be done along with a cancellation check in one call rather than needing two.
     */
    bool setTaskProgress(unsigned int done, unsigned int total);

    /**
     * Returns the total percentage complete of this Runnable.
     */
    float getPercentage() const;

    /**
     * Returns whether the total percentage complete to display for this class has changed since the last call to
     * Runnable::getPercentage().
     */
    bool isPercentageDirty() const { return isPercentageDirty_; }

private:

    mutable Mutex mutex_;

    bool isCancelled_ = false;
    mutable bool isTaskStringDirty_ = true;
    mutable bool isPercentageDirty_ = true;

    struct Task
    {
        String name;
        float percentage = 0.0f;

        float percentComplete = 0.0f;

        Task() {}
        Task(String name_, float percentage_) : name(std::move(name_)), percentage(percentage_) {}
    };
    Vector<Task> taskStack_;
};

}
