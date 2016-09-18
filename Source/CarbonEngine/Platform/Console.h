/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/EventHandler.h"
#include "CarbonEngine/Platform/TextInput.h"

namespace Carbon
{

/**
 * Handles the dispatch of console command events and the global state of the console. The console rendering is done
 * directly in the renderer based on this class.
 */
class CARBON_API Console : public EventHandler, public Logfile::OutputSink, private Noncopyable
{
public:

    /**
     * Processes console input.
     */
    bool processEvent(const Event& e) override;

    /**
     * Forces the console to process the given key down event as if it were currently showing and accepting input.
     */
    void processKeyDownEvent(const KeyDownEvent& kde);

    /**
     * Forces the console to process the given character input event as if it were currently showing and accepting
     * input.
     */
    void processCharacterInputEvent(const CharacterInputEvent& cie);

    /**
     * Executes the passed string on the console, this method is called when enter is pressed while the console is
     * showing.
     */
    void execute(const UnicodeString& string);

    /**
     * Returns whether the console is enabled.
     */
    bool isEnabled() const { return isEnabled_; }

    /**
     * Sets whether the console should be enabled. When the console is disabled it is not accessible at runtime through
     * the normal '~' key, but it can still be shown down programatically using the Console::show() method. Disabling
     * the console hides it if it is currently showing. Note that even when the console is disabled it can still be
     * shown using the hidden shortcut of holding down the left control and shift keys and pressing '~'.
     */
    void setEnabled(bool enabled);

    /**
     * Adds the passed string to the front of the displayed recent output. This can be used to output messages on the
     * console, however normally LOG_CONSOLE should be used to output to the console as it is more flexible and will
     * call any registered logfile output sinks.
     */
    void print(const UnicodeString& string);

    /**
     * Sets the current text in the input line of the console.
     */
    void setCurrentText(const UnicodeString& text);

    /**
     * Returns the current text in the input line of the console.
     */
    const UnicodeString& getCurrentText() const { return currentText_; }

    /**
     * Sets the fraction of the available vertical screen space the console should take up, in the range 0 - 1.
     */
    void setScreenFraction(float fraction);

    /**
     * Returns the fraction of the available vertical screen space the console will take up when visible, in the range 0
     * - 1.
     */
    float getScreenFraction() const { return screenFraction_; }

    /**
     * Calculates the number of lines of console output that are visible given the specified line height. This is
     * calculated based on the current resolution and the current screen fraction (see Console::getScreenFraction()).
     */
    unsigned int calculateOutputLineCount(float lineHeight);

    /**
     * Returns the internal TextInput object used for the input line.
     */
    const TextInput& getTextInput() const { return textInput_; }

    /**
     * Returns a value between 0 and 1 indicating the current expansion of the console. 0 is fully retracted, 1 is fully
     * expanded.
     */
    float getExpansion() const { return expansion_; }

    /**
     * Clears the recent output and command history on the console.
     */
    void clearHistory();

    /**
     * Returns the number of items currently in the console history. The maximum number of console history items can be
     * set using Console::setMaximumHistorySize()
     */
    unsigned int getHistorySize() const { return history_.size(); }

    /**
     * Returns a given console history item, if the passed index is out of range then an empty string is returned. The
     * oldest console history item is at index 0.
     */
    const UnicodeString& getHistoryItem(unsigned int index) const;

    /**
     * Returns the maximum number of entries that can currently be stored in the console history. Once this limit is
     * reached old console history entries will be erased. Defaults to 500 items.
     */
    unsigned int getMaximumHistorySize() const { return maximumHistorySize_; }

    /**
     * Sets the maximum number of entries the can be stored in the console history. Once this limit is reached old
     * console history entries will be erased. Defaults to 500 items.
     */
    void setMaximumHistorySize(unsigned int size);

    /**
     * Returns the number of characters that should be ignored at the front of each history item when rendering the
     * console history. This is used to allow the user to scroll the console history horizontally using the arrow keys
     * whilst holding shift.
     */
    unsigned int getHistoryOffsetX() const { return historyOffsetX_; }

    /**
     * Returns the number of most recent history items that should be ignore when rendering the console history. This is
     * used to allow the user to scroll the console history vertically using the arrow keys whilst holding shift.
     */
    unsigned int getHistoryOffsetY() const { return historyOffsetY_; }

    /**
     * Returns the console prompt string.
     */
    const UnicodeString& getPrompt() const { return prompt_; }

    /**
     * Returns whether or not the console is currently visible.
     */
    bool isVisible() const { return state_ != ConsoleHidden; }

    /**
     * Shows the console if it is not already visible.
     */
    void show();

    /**
     * Hides the console if it is currently visible.
     */
    void hide();

    /**
     * Registers a ConsoleCommand subclass instance for use.
     */
    void registerCommand(ConsoleCommand* command);

    /**
     * Returns a vector containing the names of all console commands that have been registered.
     */
    Vector<UnicodeString> getRegisteredCommands() const;

    /**
     * Takes an array of strings and prints them in aligned columns in the console.
     */
    void printInColumns(const Vector<UnicodeString>& items, bool sort, unsigned int rowsAbove = 0);

private:

    Console();
    ~Console() override;
    friend class Globals;

    bool isEnabled_ = true;

    UnicodeString currentText_;
    TextInput textInput_;

    unsigned int maximumHistorySize_ = 500;
    Vector<UnicodeString> history_;

    UnicodeString prompt_ = "> ";
    float screenFraction_ = 0.3f;
    unsigned int lastOutputLineCount_ = 0;

    enum ConsoleState
    {
        ConsoleHidden,
        ConsoleScrollingDown,
        ConsoleScrollingUp,
        ConsoleShowing
    } state_ = ConsoleHidden;

    float expansion_ = 0.0f;

    Vector<UnicodeString> commandHistory_;
    int commandHistoryIndex_ = -1;

    unsigned int historyOffsetX_ = 0;
    unsigned int historyOffsetY_ = 0;

    Vector<ConsoleCommand*> commands_;
    ConsoleCommand* findCommand(const UnicodeString& name);

    void processLogfileOutput(Logfile::OutputType type, const UnicodeString& line) override;
};

}
