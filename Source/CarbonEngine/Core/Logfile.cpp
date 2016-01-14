/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/Threads/Mutex.h"
#include "CarbonEngine/Core/Threads/Thread.h"
#include "CarbonEngine/Globals.h"

namespace Carbon
{

#if defined(CARBON_INCLUDE_LOGGING) && defined(CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS)
bool Logfile::Enabled = true;
#else
bool Logfile::Enabled = false;
#endif

bool Logfile::AssertOnWarnings = false;
bool Logfile::AssertOnErrors = false;

const char* Logfile::LogfileHeader =
    "<!DOCTYPE html>"
    "<html lang='en'>"
    "<head>"

    "<meta http-equiv='Content-type' content='text/html;charset=UTF-8'>"
    "<title>%s</title>"

    // Styles
    "<style type='text/css'>"
    "* { padding: 0; margin: 0; }"
    "body { padding-bottom: 10px; }"
    ".header { width: 100%%; background-color: #444; color: #EEE; font-family: sans-serif; padding: 10px 0 0 10px; }"
    ".bottom-border { border-bottom-style: solid; border-width: 3px; border-color: #000; margin-bottom: 10px; }"
    ".title, .subtitle { padding-bottom: 10px; }"
    ".title { font-size: 200%%; font-weight: bold; }"
    ".subtitle { font-size: 95%%; }"
    ".info, .debug, .warning, .error, .console { font-family: monospace; font-size: 10pt; padding-left: 10px }"
    ".debug { color: green; }"
    ".warning { color: blue; }"
    ".error { color: red; font-weight: bolder; }"
    ".console { color: #444; }"
    "</style>"

    // JavaScript to toggle div visibility
    "<script type='text/javascript'>"
    "function toggleDivVisibility(divId) {"
    "  if (document.getElementById(divId).style.display == 'none') {"
    "        document.getElementById(divId).style.display = 'block';"
    "  } else {"
    "    document.getElementById(divId).style.display = 'none';"
    "  }"
    "}"
    "</script>"

    "</head>"

    "<body>"
    "<div class='header bottom-border'>"
    "<div class='title'>%s</div>"
    "<div class='subtitle'>%s</div>"
    "</div>";

const char* Logfile::LogfileFooter = "</body></html>";

// This queue and associated mutex are used to queue logfile output sink calls when doing logfile writes from worker threads
// The flushOutputSinkCallQueue() function is responsible for clearing this queue every frame.
struct QueuedOutputSinkCall
{
    Logfile::OutputSink* sink = nullptr;
    Logfile::OutputType outputType = Logfile::Info;
    UnicodeString line;

    QueuedOutputSinkCall() {}
    QueuedOutputSinkCall(Logfile::OutputSink* sink_, Logfile::OutputType outputType_, UnicodeString line_)
        : sink(sink_), outputType(outputType_), line(std::move(line_))
    {
    }
};
static Vector<QueuedOutputSinkCall> queuedOutputSinkCalls;
static Mutex queuedOutputSinkCallsMutex;

class Logfile::Members
{
public:

    FileWriter file;

    OutputType currentOutputType = Logfile::Info;

    bool isHookingEnabled = false;
    UnicodeString currentLineForOutputSinks;

    static std::unordered_set<OutputSink*> outputSinks;

    // Logfile writes are thread-safe
    mutable Mutex mutex;

    // Writes directly to the open logfile if any, skipping any HTML-related transformations on the input
    template <typename T> void writeRaw(const StringBase<T>& text)
    {
        try
        {
            if (Logfile::Enabled)
                file.writeText(text, 0);
        }
        catch (const Exception&)
        {
        }
    }
};
std::unordered_set<Logfile::OutputSink*> Logfile::Members::outputSinks;

void Logfile::addOutputSink(OutputSink* sink)
{
    Members::outputSinks.insert(sink);
}

void Logfile::removeOutputSink(OutputSink* sink)
{
    Members::outputSinks.erase(sink);

    auto lock = ScopedMutexLock(queuedOutputSinkCallsMutex);
    queuedOutputSinkCalls.eraseIf([&](const QueuedOutputSinkCall& call) { return call.sink == sink; });
}

Logfile::Logfile()
{
    m = new Members;

    if (!Enabled)
        return;

#if defined(CARBON_INCLUDE_LOGGING) && defined(CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS)
    if (!m->file.openLocalFile(getFilename()))
        return;

    // Get log title from the client name
    auto title = Globals::getClientName() + " Log";
    auto subtitle = "Run on " + FileSystem::getDateTime();

    // Write HTML header, inserting the title and subtitle as needed
    auto header = String(LogfileHeader);
    header.replace("%%", "%");
    header.replace("<title>%s</title>", "<title>" + title + "</title>");
    header.replace("<div class='title'>%s</div>", "<div class='title'>" + title + "</div>");
    header.replace("<div class='subtitle'>%s</div>", "<div class='subtitle'>" + subtitle + "</div>");
    m->writeRaw(header);
#endif
}

Logfile::~Logfile()
{
    try
    {
        m->file.close();
    }
    catch (const Exception&)
    {
    }

    delete m;
    m = nullptr;
}

#ifdef CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS

UnicodeString Logfile::getFilename() const
{
    return getFilename(Globals::getClientName().length() ? Globals::getClientName() + " Log" : "Carbon Log");
}

UnicodeString Logfile::getFilename(const UnicodeString& logfileName)
{
    auto logsDirectory = UnicodeString::Period;

#ifdef WINDOWS

    // On Windows logfiles are put into the current user's AppData/Roaming/<client name> directory
    auto path = PWSTR();
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path);
    logsDirectory = FileSystem::joinPaths(fromUTF16(path), Globals::getClientName());
    CoTaskMemFree(path);

#elif defined(LINUX)

    // On Linux logfiles are put into the ~/.<client name> directory
    logsDirectory = FileSystem::joinPaths(FileSystem::getHomeDirectory(), String::Period + Globals::getClientName());

#elif defined(APPLE)

    // Put logfiles under ~/Library/Logs
    logsDirectory = FileSystem::joinPaths(FileSystem::getUserLibraryDirectory(), "Logs");

#ifdef MACOSX
    logsDirectory = FileSystem::joinPaths(logsDirectory, Globals::getClientName());
#endif

#endif

    return FileSystem::joinPaths(logsDirectory, logfileName + ".html");
}

#endif

void Logfile::writeLine(const String& caller, const UnicodeString& lineContent, OutputType type, bool writeTimestamp)
{
    auto lock = ScopedMutexLock(m->mutex);

    m->currentOutputType = type;

    // Open div for this output type
    *this << "<div class='";
    if (type == Info)
        *this << "info";
    else if (type == Debug)
        *this << "debug";
    else if (type == Warning)
        *this << "warning";
    else if (type == Error)
        *this << "error";
    else if (type == Console)
        *this << "console";
    *this << "'>";

    m->isHookingEnabled = true;

    if (writeTimestamp)
        *this << "[" << FileSystem::getShortDateTime() << "] ";

    // Add the prefix for this output type
    if (type == Warning)
        *this << "Warning: ";
    else if (type == Error)
        *this << "Error: ";

    if (caller.length())
    {
        // Format the result of CARBON_CURRENT_FUNCTION in a consistent way across all platforms. CARBON_CURRENT_FUNCTION uses
        // either __FUNCTION__ or __PRETTY_FUNCTION__ depending on the compiler. Aim for a string in the format Class::Method()
        // regardless of which macro the name originated from.

        auto formattedCaller = caller;

        // Cut off everything after the final ')'
        auto index = formattedCaller.findLastOf(")");
        if (index != -1)
        {
            formattedCaller = formattedCaller.substr(0, index + 1);

            // Cut out all the parameter type information. Parentheses need to be counted to determine where the parameter type
            // information stops because function pointer parameters will have parentheses in them.
            auto parenthesisCount = 0;
            for (auto i = int(formattedCaller.length()) - 1; i >= 0; i--)
            {
                if (formattedCaller.at(i) == ')')
                    parenthesisCount++;
                else if (formattedCaller.at(i) == '(')
                {
                    parenthesisCount--;
                    if (parenthesisCount <= 0)
                    {
                        formattedCaller = formattedCaller.substr(0, i);
                        break;
                    }
                }
            }
        }

        // Cut out the return type information if present
        index = formattedCaller.findLastOf(String::Space);
        if (index != -1)
        {
            formattedCaller = formattedCaller.substr(index + 1);
            formattedCaller.trimLeft("*&");
        }

        *this << formattedCaller.withoutPrefix("Carbon::") << "() - ";
    }

    *this << lineContent << UnicodeString::Newline;

    m->isHookingEnabled = false;

    *this << "</div>";

#ifdef CARBON_DEBUG
    // Trigger assertions on warnings/errors if enabled
    if (type == Warning && AssertOnWarnings)
        assert(false && "Asserting because a warning was reported");
    else if (type == Error && AssertOnErrors)
        assert(false && "Asserting because an error was reported");
#endif
}

Logfile& Logfile::write(const UnicodeString& data)
{
    if (m->file.isOpen())
    {
        auto html = data;

        if (m->isHookingEnabled)
        {
            html.replace("&", "&amp;");
            html.replace("\"", "&quot;");
            html.replace("<", "&lt;");
            html.replace(">", "&gt;");
            html.replace(UnicodeString::Space, "&nbsp;");
        }

        // Replace newline characters with <br/> tags
        html.replace(UnicodeString::Newline, "<br/>");

        // Write content to the logfile
        m->writeRaw(html);

        // Write footer
        static const auto footer = String(LogfileFooter);
        m->writeRaw(footer);

        // Wind back the file write position to before the footer
        m->file.setPosition(-int(footer.length()), true);

        m->file.flush();
    }

    // If Logfile::Enabled is set to false then the logfile output is automatically sent through Globals::debugLog() as well as
    // through any output sinks that have been registered
    auto echoLogfileOutputThroughGlobalsDebugLog = !Enabled;

    if (m->isHookingEnabled && (m->outputSinks.size() || echoLogfileOutputThroughGlobalsDebugLog))
    {
        m->currentLineForOutputSinks << data;

        if (m->currentLineForOutputSinks.find(UnicodeString::Newline) != -1)
        {
            m->currentLineForOutputSinks.trimRight();

            if (echoLogfileOutputThroughGlobalsDebugLog)
                Globals::debugLog(m->currentLineForOutputSinks.toUTF8().as<char>());

            if (Thread::isRunningInMainThread())
            {
                for (auto outputSink : m->outputSinks)
                    outputSink->processLogfileOutput(m->currentOutputType, m->currentLineForOutputSinks);
            }
            else
            {
                auto lock = ScopedMutexLock(queuedOutputSinkCallsMutex);

                for (auto outputSink : m->outputSinks)
                    queuedOutputSinkCalls.emplace(outputSink, m->currentOutputType, m->currentLineForOutputSinks);
            }

            m->currentLineForOutputSinks.clear();
        }
    }

    return *this;
}

void Logfile::writeLines(const Vector<String>& lines, OutputType type)
{
    for (auto& line : lines)
        writeLine(String::Empty, line, type);
}

void Logfile::writeCollapsibleSection(const UnicodeString& title, const Vector<UnicodeString>& contents, OutputType type,
                                      bool writeLineNumbers)
{
    // Collapsible sections are done with an <a> tag that toggles the display style on a div holding the contents of the
    // section. The toggleDivVisibility() JavaScript function used here is defined in Logfile::LogfileHeader.

    static auto nextSectionID = 0U;

    auto divID = String() + "collapsible-section-" + nextSectionID++;

    m->writeRaw(UnicodeString() + "<div class='info'>[" + FileSystem::getShortDateTime() + "] " +
                "<a href='javascript:;' onmousedown='toggleDivVisibility(\"" + divID + "\");'>" + title + "</a></div>" +
                "<div id='" + divID + "' style='display: none; padding-left: 5em; padding-top: 1em; padding-bottom: 1em;'>");

    for (auto i = 0U; i < contents.size(); i++)
    {
        if (writeLineNumbers)
            writeLine(String::Empty, (UnicodeString(i + 1) + ":").padToLength(10) + contents[i], type, false);
        else
            writeLine(String::Empty, contents[i], type, false);
    }

    m->writeRaw(String("</div>"));
}

Logfile& Logfile::get()
{
    static auto logfile = Logfile();
    return logfile;
}

// Flush the contents of the output sink call queue every frame in response to UpdateEvent. Logfile writes from other threads
// cannot call the output sinks directly and so the required calls are queued and then flushed every frame here on the main
// thread.
static bool flushOutputSinkCallQueue(const UpdateEvent& e)
{
    auto lock = ScopedMutexLock(queuedOutputSinkCallsMutex);

    for (auto& queuedCall : queuedOutputSinkCalls)
        queuedCall.sink->processLogfileOutput(queuedCall.outputType, queuedCall.line);

    queuedOutputSinkCalls.clear();

    return true;
}
CARBON_REGISTER_EVENT_HANDLER_FUNCTION(UpdateEvent, flushOutputSinkCallQueue)

}
