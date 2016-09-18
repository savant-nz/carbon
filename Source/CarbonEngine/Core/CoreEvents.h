/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Event.h"

namespace Carbon
{

/**
 * This event is sent when any of the text displaying on the console changes, including the console history.
 */
class CARBON_API ConsoleTextChangedEvent : public Event
{
public:

    bool isLoggable() const override { return false; }
};

/**
 * This event is sent whenever a file system error occurs, see the FileSystemError enumeration for details. Applications
 * can use this to display information about hardware faults, low disk space, and so on.
 */
class CARBON_API FileSystemErrorEvent : public Event
{
public:

    /**
     * Initializes the contents of this file system error event.
     */
    FileSystemErrorEvent(FileSystemError error, UnicodeString resourceName)
        : error_(error), resourceName_(std::move(resourceName))
    {
    }

    /**
     * Returns the file system error that occurred.
     */
    FileSystemError getError() const { return error_; }

    /**
     * Returns the file system error that occurred as a human readable string.
     */
    String getErrorString() const;

    /**
     * Returns the name of the file or directory resource that was being accessed when the error occurred.
     */
    const UnicodeString& getResourceName() const { return resourceName_; }

    operator UnicodeString() const override
    {
        return UnicodeString() << "error: " << getErrorString() << ", resource: " << getResourceName();
    }

private:

    const FileSystemError error_;
    const UnicodeString resourceName_;
};

/**
 * This event is sent at the start of the application's main loop every frame.
 */
class CARBON_API FrameBeginEvent : public Event
{
public:

    bool isLoggable() const override { return false; }
};

/**
 * This event is sent in order to gather information from the engine about where memory is being used. Parts of the
 * engine that are managing resources of a significant size should handle this event and call
 * GatherMemorySummaryEvent::addAllocation() for each of the relevant allocations. The gathered information is then be
 * used to display information showing where memory is being used and where savings could be made. It can also be useful
 * for uncovering resource leaks.
 */
class CARBON_API GatherMemorySummaryEvent : public Event
{
public:

    /**
     * This class describes a single memory allocation that has been gathered by this event. Handlers of this event add
     * information on their allocations using the GatherMemorySummaryEvent::addAllocation() method, the list of
     * allocations is returned by GatherMemorySummaryEvent::getAllocations().
     */
    class MemoryAllocation
    {
    public:

        /**
         * Returns the human readable description of what this allocation is for. There are no predefined allocation
         * types, but handlers of this event should use sensible names that reflect what each memory allocation is being
         * used for. This should generally specify a broad category of data, i.e. 'Texture', with specific details
         * unique to this allocation put into the separate details field.
         */
        const String& getType() const { return type_; }

        /**
         * Returns the details on this memory allocation that provides more information than the type alone. One example
         * of using this comes with texture image data, where the type is set to 'Texture' and details is set to the
         * name of the texture the image data is for. Can be left blank.
         */
        const String& getDetails() const { return details_; }

        /**
         * Returns the address of this memory allocation.
         */
        const void* getAddress() const { return address_; }

        /**
         * Returns the size in bytes of this memory allocation.
         */
        size_t getSize() const { return size_; }

        MemoryAllocation() {}

        /**
         * Initializes this memory allocation with the given values.
         */
        MemoryAllocation(String type, String details, const void* address, size_t size)
            : type_(std::move(type)), details_(std::move(details)), address_(address), size_(size)
        {
        }

    private:

        String type_;
        String details_;

        const void* address_ = nullptr;

        size_t size_ = 0;
    };

    /**
     * Adds a new allocation to the list of allocations gathered by this event. Handlers of this event should call this
     * method once for each allocation they have information on. If \a address is null then this method does nothing.
     */
    void addAllocation(const String& type, const String& details, const void* address, size_t size) const
    {
        if (address)
            allocations_.emplace(type, details, address, size);
    }

    /**
     * Returns all the allocations that have been added using GatherMemorySummaryEvent::addAllocation().
     */
    const Vector<MemoryAllocation>& getAllocations() const { return allocations_; }

    /**
     * Sends a GatherMemorySummaryEvent and then logs all returned memory usage information to the console and the
     * logfile.
     */
    static void report();

private:

    mutable Vector<MemoryAllocation> allocations_;
};

/**
 * This event is sent on some platforms when the operating system detects that the amount of free memory is running low,
 * the application should respond by freeing caches and other allocations that can be easily recreated later. At present
 * this event is only sent on iOS devices.
 */
class CARBON_API LowMemoryWarningEvent : public Event
{
};

/**
 * This event is sent when the engine is notified that the user wants to reset or shut down the application. This event
 * is only a notification to the application, i.e. the application is responsible for taking appropriate action to
 * terminate the main loop. Reset means returning the application to its startup screen, and can be distinguished from a
 * shutdown request using ShutdownRequestEvent::isReset().
 */
class CARBON_API ShutdownRequestEvent : public Event
{
public:

    /**
     * Initializes the contents of this shutdown request event.
     */
    ShutdownRequestEvent(bool isReset = false) : isReset_(isReset) {}

    /**
     * The shutdown request event may be sent in response to a reset request, which should return the application to its
     * startup screen. Applications that need to distinugish between shutdown and reset requests can do so by using this
     * value.
     */
    bool isReset() const { return isReset_; }

    operator UnicodeString() const override { return UnicodeString() << "reset: " << isReset(); }

private:

    const bool isReset_;
};

/**
 * This event is sent every frame to update all the different systems in the engine.
 */
class CARBON_API UpdateEvent : public Event
{
public:

    bool isLoggable() const override { return false; }
};

}
