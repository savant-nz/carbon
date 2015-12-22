/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Manages the HTML logfile which is how information, warnings and errors are recorded by the engine and application. The
 * location of the application's logfile depends on the platform:
 *
 * - On Windows the logfile is in the `%APPDATA%/Roaming/<application name>` directory.
 * - On Linux the logfile is in the `~/.\<application name\>` directory.
 * - On Mac OS X the logfile is in the `~/Library/Logs/\<application name\>` directory
 * - On iOS the logfile is in the `Library/Logs` directory inside the application's sandbox.
 *
 * The supplied `OpenLogfile.rb` script can be used to open the most recently written logfile in the default browser, and is the
 * simplest way to open the logfile. When the MemoryInterceptor is included in the build then a second logfile reporting any
 * memory leaks will also be created in the same directory as the logfile.
 *
 * Writing output to the logfile is best done with one of \ref LOG_INFO, \ref LOG_WARNING, \ref LOG_ERROR, \ref LOG_DEBUG,
 * LOG_VALUE(), or \ref LOG_CONSOLE. The different output types are color coded in the HTML logfile.
 *
 * Writing to the logfile is thread-safe.
 *
 * Output written to the logfile can be sent through arbitrary output sinks using Logfile::addOutputSink().
 */
class CARBON_API Logfile : private Noncopyable
{
public:

    /**
     * Copy constructor (not implemented).
     */
    Logfile(const Logfile& other);

    /**
     * The different types of logfile output, these are then color coded in the HTML logfile.
     */
    enum OutputType
    {
        Info,
        Debug,
        Warning,
        Error,
        Console
    };

    /**
     * Controls whether logfiles are written to the local filesystem. Note that when this is set to false logfile output can
     * still be intercepted and redirected elsewhere through logfile output sinks, see Logfile::addOutputSink() for details.
     * Defaults to true on platforms that support local filesystem access, and false on platforms that don't. When this is set
     * to false, all logfile output is redirected through Globals::debugLog().
     */
    static bool Enabled;

    /**
     * Controls whether an assert should be triggered whenever a warning is written to the logfile, this can be useful when
     * debugging. Defaults to false.
     */
    static bool AssertOnWarnings;

    /**
     * Controls whether an assert should be triggered whenever an error is written to the logfile, this can be useful when
     * debugging. Defaults to false.
     */
    static bool AssertOnErrors;

    /**
     * Interface for a logfile output sink that can receive/redirect logfile output to custom locations.
     */
    class CARBON_API OutputSink
    {
    public:

        virtual ~OutputSink() {}

        /**
         * This method is called by the Logfile class to write a line of logfile output to this output sink. This method is
         * always called on the main thread.
         */
        virtual void processLogfileOutput(OutputType type, const UnicodeString& line) = 0;
    };

    /**
     * Adds an output sink that will receive every line that is written to the logfile. This allows logfile output to be sent
     * into places such as the console or an output window. Logfiles can be written to by any thread, however output sinks are
     * always called from the main thread.
     */
    static void addOutputSink(OutputSink* sink);

    /**
     * Removes a logfile output sink added with Logfile::addOutputSink().
     */
    static void removeOutputSink(OutputSink* sink);

    /**
     * This is the printf-compatible string used for the HTML logfile header. It requires three parameters be passed to it when
     * being evaluated: the HTML page title, the log title, and a subtitle. All three parameters must be null-terminated
     * strings.
     */
    static const char* LogfileHeader;

    /**
     * This is the string used for the HTML logfile footer.
     */
    static const char* LogfileFooter;

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS
    /**
     * Returns the fully qualified path and filename for this logfile, note that this will vary depending on the active
     * platform.
     */
    UnicodeString getFilename() const;

    /**
     * Given a logfile name this method returns the fully qualified path for that logfile, note that this will vary depending on
     * the active platform.
     */
    static UnicodeString getFilename(const UnicodeString& logfileName);
#endif

    /**
     * Writes a line of output of the given type to this logfile.
     */
    void writeLine(const String& caller, const UnicodeString& lineContent, OutputType type, bool writeTimestamp = true);

    /**
     * Writes the given lines to the logfile using the Logfile::writeLine() method.
     */
    void writeLines(const Vector<String>& lines, OutputType type);

    /**
     * Writes the passed content into the HTML logfile as a section that can be expanded and collapsed by the user when reading
     * through the logfile. This is useful to avoid cluttering up the logfile with long lists of information that would often be
     * extraneous but still need to be recorded.
     */
    void writeCollapsibleSection(const UnicodeString& title, const Vector<UnicodeString>& contents, OutputType type = Info,
                                 bool writeLineNumbers = false);

    /**
     * Returns the global Logfile instance.
     */
    static Logfile& get();

    /**
     * Internal helper class used by the logging macros, it houses a temporary UnicodeString instance that is written to the
     * logfile on destruction.
     */
    class LogfileWriter : private Noncopyable
    {
    public:

        /**
         * Constructs this logfile writer instance with the specified output type and caller.
         */
        LogfileWriter(Logfile::OutputType outputType, String caller)
#ifdef CARBON_INCLUDE_LOGGING
            : outputType_(outputType), caller_(std::move(caller))
#endif
        {
        }

#ifdef CARBON_INCLUDE_LOGGING
        ~LogfileWriter() { Logfile::get().writeLine(caller_, content_, outputType_); }
#endif

#ifdef CARBON_INCLUDE_LOGGING
        /**
         * Concatenates the passed argument onto the internal string.
         */
        template <typename T> UnicodeString& operator<<(T&& argument) { return content_ << std::forward<T>(argument); }
#else
        template <typename T> LogfileWriter& operator<<(T&&) { return *this; }
#endif

    private:

#ifdef CARBON_INCLUDE_LOGGING
        const Logfile::OutputType outputType_;
        const String caller_;

        UnicodeString content_;
#endif
    };

private:

    Logfile();
    ~Logfile();

    class Members;
    Members* m = nullptr;

    Logfile& write(const UnicodeString& data);

    Logfile& operator<<(const String& data) { return write(data.cStr()); }

    template <typename T> Logfile& operator<<(const T& data) { return write(UnicodeString(data)); }
};

/**
 * \file
 */

/**
 * Logs the subsequent items to the main logfile with the specified caller and output type. Multiple items can be logged
 * using `operator <<` to append them together.
 */
#define CARBON_LOG(OutputType, Caller) Carbon::Logfile::LogfileWriter(Carbon::Logfile::OutputType, Caller)

/**
 * Logs the subsequent items to the main logfile. Multiple items can be logged using `operator <<` to append them together.
 *
 * In addition to simple logging of types such as int and float, any type that provides a conversion to String can also be
 * logged directly. Most classes in the engine provide such a conversion which means that logging types such as Vec2, Vec3,
 * Quaternion, AABB, Matrix3, Matrix4, Entity and other commonly used types can be logged for debugging or reporting purposes
 * without having to manually do a conversion to a string.
 */
#define LOG_INFO CARBON_LOG(Info, Carbon::String::Empty)

/**
 * Logs the subsequent items to the main logfile and to the Console as a warning along with the name of the caller. Multiple
 * items can be logged using `operator <<` to append them together. See \ref LOG_INFO for more details on logging.
 */
#define LOG_WARNING CARBON_LOG(Warning, CARBON_CURRENT_FUNCTION)

/**
 * Logs the subsequent items to the main logfile and to the Console as a warning without the name of the caller. Multiple items
 * can be logged using `operator <<` to append them together. See \ref LOG_INFO for more details on logging.
 */
#define LOG_WARNING_WITHOUT_CALLER CARBON_LOG(Warning, Carbon::String::Empty)

/**
 * Logs the subsequent items to the main logfile and to the Console as an error along with the name of the caller. Multiple
 * items can be logged using `operator <<` to append them together. See \ref LOG_INFO for more details on logging.
 */
#define LOG_ERROR CARBON_LOG(Error, CARBON_CURRENT_FUNCTION)

/**
 * Logs the subsequent items to the main logfile and to the Console as an error without the name of the caller. Multiple items
 * can be logged using `operator <<` to append them together. See \ref LOG_INFO for more details on logging.
 */
#define LOG_ERROR_WITHOUT_CALLER CARBON_LOG(Error, Carbon::String::Empty)

/**
 * Logs the subsequent items to the main logfile and to the Console as debug information. Multiple items can be logged using
 * `operator <<` to append them together. See \ref LOG_INFO for more details on logging.
 */
#define LOG_DEBUG CARBON_LOG(Debug, Carbon::String::Empty)

/**
 * Uses \ref LOG_DEBUG to log the \a Data parameter as a string followed by what it evaluates to. This can be useful when
 * debugging to easily log a variable's name and value. For example, the following code will produce the output
 * `position: 1 2 3`:
\code
auto position = Vec3(1.0f, 2.0f, 3.0f);
LOG_VALUE(position);
\endcode
 *
 * See \ref LOG_INFO for more details on logging.
 */
#define LOG_VALUE(Data) LOG_DEBUG << #Data ": " << Data

/**
 * Logs the subsequent items to the main logfile and to the dropdown console. Multiple items can be logged using `operator <<`
 * to append them together. See \ref LOG_INFO for more details on logging.
 */
#define LOG_CONSOLE CARBON_LOG(Console, Carbon::String::Empty)

}
