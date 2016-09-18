/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/Console.h"
#include "CarbonEngine/Platform/ConsoleCommand.h"
#include "CarbonEngine/Platform/PlatformEvents.h"
#include "CarbonEngine/Platform/PlatformInterface.h"

namespace Carbon
{

Console::Console() : textInput_(currentText_)
{
    events().addHandler<CharacterInputEvent>(this, true);
    events().addHandler<KeyDownEvent>(this, true);
    events().addHandler<KeyUpEvent>(this, true);

    Logfile::addOutputSink(this);
}

Console::~Console()
{
    events().removeHandler(this);

    Logfile::removeOutputSink(this);
}

void Console::processLogfileOutput(Logfile::OutputType type, const UnicodeString& line)
{
    // Print warnings and errors to the console
    if (type == Logfile::Debug || type == Logfile::Warning || type == Logfile::Error)
        print(line);
    else if (type == Logfile::Console)
        print(line.substr(22));    // Strip off the timestamp
}

bool Console::processEvent(const Event& e)
{
    if (e.as<UpdateEvent>())
    {
        if (state_ == ConsoleScrollingDown || state_ == ConsoleScrollingUp)
        {
            // It takes 250ms for the console to drop down
            auto delta = platform().getTimePassed() % 0.25f;

            if (state_ == ConsoleScrollingDown)
            {
                expansion_ += delta;

                if (expansion_ >= 1.0f)
                {
                    expansion_ = 1.0f;
                    state_ = ConsoleShowing;
                    events().removeHandler<UpdateEvent>(this);
                }
            }
            else
            {
                expansion_ -= delta;

                if (expansion_ <= 0.0f)
                {
                    expansion_ = 0.0f;
                    state_ = ConsoleHidden;
                    events().removeHandler<UpdateEvent>(this);
                }
            }
        }
    }
    else if (auto kde = e.as<KeyDownEvent>())
    {
        auto isConsoleActivationKey = kde->getKey() == KeyGraveAccent || kde->getKey() == KeyKanji;

        if (isConsoleActivationKey && state_ == ConsoleHidden && isEnabled() && commands_.size())
            show();
        else if ((isConsoleActivationKey || kde->getKey() == KeyEscape) && state_ == ConsoleShowing)
        {
            hide();
            return false;
        }

        if (isConsoleActivationKey)
            return false;

        if (state_ != ConsoleHidden)
        {
            processKeyDownEvent(*kde);
            return false;
        }
    }
    else if (e.as<KeyUpEvent>())
    {
        if (state_ != ConsoleHidden)
            return false;
    }
    else if (auto cie = e.as<CharacterInputEvent>())
    {
        if (state_ != ConsoleHidden)
        {
            if (cie->getKey() != KeyGraveAccent && !cie->getInput().startsWith("`"))
                processCharacterInputEvent(*cie);

            return false;
        }
    }

    return true;
}

void Console::processKeyDownEvent(const KeyDownEvent& kde)
{
    if (textInput_.onKeyDownEvent(kde))
        events().dispatchEvent(ConsoleTextChangedEvent());

    // When holding shift the arrow keys can be used to scroll the console history
    if (platform().isKeyPressed(KeyLeftShift, true) || platform().isKeyPressed(KeyRightShift, true))
    {
        if (kde.getKey() == KeyLeftArrow && historyOffsetX_ > 0)
            historyOffsetX_--;
        if (kde.getKey() == KeyRightArrow)
            historyOffsetX_++;
        if (kde.getKey() == KeyDownArrow && historyOffsetY_ > 0)
            historyOffsetY_--;
        if (kde.getKey() == KeyUpArrow)
            historyOffsetY_++;

        // Clamp the history offsets to sensible boundaries
        historyOffsetX_ = std::min(historyOffsetX_, UnicodeString::longestString(history_) - 1);
        historyOffsetY_ = std::min(historyOffsetY_, history_.size());

        return;
    }

    if (kde.getKey() == KeyUpArrow)
    {
        if (commandHistoryIndex_ == -1)
        {
            if (commandHistory_.size())
            {
                commandHistoryIndex_ = commandHistory_.size() - 1;
                setCurrentText(commandHistory_[commandHistoryIndex_]);
            }
        }
        else if (commandHistoryIndex_ > 0)
        {
            commandHistoryIndex_--;
            setCurrentText(commandHistory_[commandHistoryIndex_]);
        }
    }
    else if (kde.getKey() == KeyDownArrow)
    {
        if (commandHistory_.empty() || commandHistoryIndex_ == -1)
            ;
        else if (commandHistoryIndex_ < int(commandHistory_.size() - 1))
        {
            commandHistoryIndex_++;
            setCurrentText(commandHistory_[commandHistoryIndex_]);
        }
        else if (uint(commandHistoryIndex_) == commandHistory_.size() - 1)
        {
            commandHistoryIndex_ = -1;
            setCurrentText(UnicodeString::Empty);
        }
    }
    else if (kde.getKey() == KeyEnter || kde.getKey() == KeyNumpadEnter)
    {
        if (currentText_.length())
        {
            commandHistory_.append(currentText_);

            // Make sure the command history doesn't get too big
            while (commandHistory_.size() > 100)
                commandHistory_.erase(0);
        }

        commandHistoryIndex_ = -1;

        execute(currentText_);
        setCurrentText(UnicodeString::Empty);
    }
    else if (kde.getKey() == KeyTab)
    {
        // Tab completion

        auto currentText = currentText_.trimmedLeft();

        if (currentText.length() == 0)
        {
            // If there is nothing entered and tab is pressed act as though a "List" command was given
            if (!kde.isRepeat() && getRegisteredCommands().has("List"))
                execute("List");

            return;
        }

        auto command = pointer_to<ConsoleCommand>::type();
        auto partial = UnicodeString();
        auto completions = Vector<UnicodeString>();

        if (currentText.count(' ') == 0)
        {
            partial = currentText;
            for (auto c : commands_)
                completions.append(c->getName());
        }
        else
        {
            command = findCommand(currentText.substr(0, currentText.findFirstOf(" ")));
            if (command)
            {
                auto pieces = currentText.getTokens();
                auto parameterIndex = pieces.size() - 1;

                if (!currentText.endsWith(" "))
                {
                    partial = pieces.back();
                    parameterIndex--;
                }

                command->getTabCompletions(parameterIndex, completions);
            }
        }

        // Cut out irrelevant completions
        completions.eraseIf([&](const UnicodeString& c) { return !c.asLower().startsWith(partial.asLower()); });

        if (completions.empty())
        {
            if (command)
                LOG_CONSOLE << command->getName() << " " << command->getParametersDescription();
        }
        else if (completions.size() == 1)
        {
            // If there's only one completion it can just be filled in
            auto index = currentText.findLastOf(" ");
            setCurrentText(currentText.substr(0, index == -1 ? 0 : (index + 1)) + completions[0] + " ");
        }
        else if (completions.size() > 1)
        {
            // If all the completions are the same up to a certain point then autocomplete up to that point
            auto completionLength = 0U;
            while (true)
            {
                if (completions.has([&](const UnicodeString& c) {
                        return completionLength >= c.length() ||
                            c.at(completionLength) != completions[0].at(completionLength);
                    }))
                {
                    break;
                }

                completionLength++;
            }

            auto index = currentText.findLastOf(" ");
            setCurrentText(currentText.substr(0, index == -1 ? 0 : (index + 1)) +
                           completions[0].substr(0, completionLength));

            LOG_CONSOLE << prompt_ << currentText;
            if (command)
                printInColumns(completions, false, 1);
            else
            {
                auto longest = UnicodeString::longestString(completions);

                // The completion list is full of command names, so add parameter information as well
                for (auto& completion : completions)
                {
                    auto c = findCommand(completion);
                    if (c)
                        completion = completion.padToLength(longest + 4) + c->getParametersDescription();

                    LOG_CONSOLE << completion;
                }
            }
        }
    }
}

void Console::processCharacterInputEvent(const CharacterInputEvent& cie)
{
    if (textInput_.onCharacterInputEvent(cie))
        events().dispatchEvent(ConsoleTextChangedEvent());
}

void Console::execute(const UnicodeString& string)
{
    // Reset console history offsets when a command is executed on the console
    historyOffsetX_ = 0;
    historyOffsetY_ = 0;

    // Trim the command
    auto s = string.trimmed();
    if (!s.length())
        return;

    LOG_CONSOLE << prompt_ << string;

    auto command = UnicodeString();
    auto parameters = Vector<UnicodeString>();

    auto firstSpace = s.findFirstOf(" ");
    if (firstSpace == -1)
        command = s;
    else
    {
        command = s.substr(0, firstSpace);
        parameters = s.substr(firstSpace + 1).getTokens();
    }

    // Check the command is registered
    auto consoleCommand = findCommand(command);
    if (!consoleCommand)
    {
        LOG_CONSOLE << "Error: command '" << command << "' not found";
        return;
    }

    if (!consoleCommand->areParametersValid(parameters))
    {
        LOG_CONSOLE << "Error: incorrect number of parameters for console command " << consoleCommand->getName();
        return;
    }

    consoleCommand->run(parameters);
}

void Console::setEnabled(bool enabled)
{
    isEnabled_ = enabled;

    // Disabling the console also hides it
    if (!isEnabled_)
        hide();
}

void Console::print(const UnicodeString& string)
{
    if (string == UnicodeString::Newline)
        return;

    history_.append(string);

    // Don't let the history get too large
    while (history_.size() > maximumHistorySize_)
        history_.popFront();

    events().dispatchEvent(ConsoleTextChangedEvent());
}

void Console::clearHistory()
{
    history_.clear();
    historyOffsetX_ = 0;
    historyOffsetY_ = 0;

    events().dispatchEvent(ConsoleTextChangedEvent());
}

void Console::setCurrentText(const UnicodeString& text)
{
    currentText_ = text;
    textInput_.setCursorPosition(currentText_.length());

    events().dispatchEvent(ConsoleTextChangedEvent());
}

void Console::setScreenFraction(float fraction)
{
    screenFraction_ = Math::clamp01(fraction);
}

unsigned int Console::calculateOutputLineCount(float lineHeight)
{
    lastOutputLineCount_ = 0;

    if (lineHeight > 0.0f)
    {
        auto count = uint(platform().getWindowHeight() * 0.95f * screenFraction_ / lineHeight);
        lastOutputLineCount_ = (count == 0) ? 0 : (count - 1);
    }

    return lastOutputLineCount_;
}

const UnicodeString& Console::getHistoryItem(unsigned int index) const
{
    if (index >= history_.size())
        return UnicodeString::Empty;

    return history_[index];
}

void Console::setMaximumHistorySize(unsigned int size)
{
    maximumHistorySize_ = size;

    while (history_.size() > maximumHistorySize_)
        history_.popFront();
}

void Console::show()
{
    if (state_ == ConsoleShowing)
        return;

    state_ = ConsoleScrollingDown;
    platform().setAllowIsKeyPressed(false);
    events().addHandler<UpdateEvent>(this);
}

void Console::hide()
{
    if (state_ == ConsoleHidden)
        return;

    state_ = ConsoleScrollingUp;
    platform().setAllowIsKeyPressed(true);
    events().addHandler<UpdateEvent>(this);
}

Vector<UnicodeString> Console::getRegisteredCommands() const
{
    return commands_.map<UnicodeString>([](const ConsoleCommand* c) { return c->getName(); });
}

void Console::printInColumns(const Vector<UnicodeString>& items, bool sort, unsigned int rowsAbove)
{
    const auto& theItems = sort ? items.sorted() : items;

    auto lineCount = lastOutputLineCount_ - rowsAbove;

    if (theItems.size() <= lineCount || lastOutputLineCount_ == 0 || state_ != ConsoleShowing)
    {
        for (auto& item : theItems)
            LOG_CONSOLE << item;
    }
    else
    {
        auto columns = Vector<Vector<UnicodeString>>(theItems.size() / lineCount + 1);
        auto longests = Vector<unsigned int>(columns.size(), 0);

        for (auto i = 0U; i < theItems.size(); i++)
        {
            auto column = i / lineCount;
            columns[column].append(theItems[i]);

            if (theItems[i].length() > longests[column])
                longests[column] = theItems[i].length();
        }

        for (auto i = 0U; i < lineCount; i++)
        {
            auto row = UnicodeString();

            for (auto j = 0U; j < columns.size(); j++)
            {
                if (columns[j].size() <= i)
                    break;

                auto item = columns[j][i];
                item.resize(longests[j] + 4, ' ');
                row << item;
            }

            LOG_CONSOLE << row;
        }
    }
}

void Console::registerCommand(ConsoleCommand* command)
{
    if (findCommand(command->getName()))
        return;

    if (!command->getName().isAlphaNumeric())
    {
        LOG_ERROR << "Invalid console command name: " << command->getName();
        return;
    }

    if (command->getParametersDescription().count('<') != command->getParametersDescription().count('>') ||
        command->getParametersDescription().count('[') != command->getParametersDescription().count(']'))
    {
        LOG_ERROR << "Invalid console command parameters description: " << command->getParametersDescription();
        return;
    }

    commands_.append(command);
}

ConsoleCommand* Console::findCommand(const UnicodeString& name)
{
    return commands_.detect([&](const ConsoleCommand* c) { return c->getName().asLower() == name.asLower(); }, nullptr);
}

}
