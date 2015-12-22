/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Platform/Console.h"

namespace Carbon
{

/**
 * Base class for console commands that defeines an interface for executing a command and doing parameter validation and tab
 * completion. Console commands are registered through Console::registerCommand(). There are macros that handle registration of
 * console commands: CARBON_REGISTER_CONSOLE_COMMAND_SUBCLASS(), CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(),
 * CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE().
 */
class CARBON_API ConsoleCommand
{
public:

    /**
     * Constructs this console command with the given name and parameters description. The format of the parameters description
     * is used to automatically validate that the correct number of parameters have been given. Each parameter name must be
     * enclosed by angle brackets, and optional parameters should additionally be surrounded by square brackets.
     */
    ConsoleCommand(UnicodeString name, UnicodeString parametersDescription)
        : name_(std::move(name)), parametersDescription_(std::move(parametersDescription))
    {
        maximumParameterCount_ = parametersDescription_.count('<');
        minimumParameterCount_ = maximumParameterCount_ - parametersDescription_.count('[');
    }

    virtual ~ConsoleCommand() {}

    /**
     * Returns the name of this console command
     */
    const UnicodeString& getName() const { return name_; }

    /**
     * Returns a descrption of the parameters for this console command.
     */
    const UnicodeString& getParametersDescription() const { return parametersDescription_; }

    /**
     * Returns whether the given set of parameters is valid for this console command. Currently this only checks the number of
     * parameters, no type checking is done. The number of parameters is determiend based on the formatting of the parameters
     * description string.
     */
    bool areParametersValid(const Vector<UnicodeString>& parameters) const
    {
        return parameters.size() >= minimumParameterCount_ && parameters.size() <= maximumParameterCount_;
    }

    /**
     * This method is called to execute a console command with the given set of parameters.
     */
    virtual void run(const Vector<UnicodeString>& parameters) = 0;

    /**
     * Provides tab completion for the parameters in this console command. Can be implemented by subclasses.
     */
    virtual void getTabCompletions(unsigned int parameterIndex, Vector<UnicodeString>& completions) const {}

private:

    UnicodeString name_;
    UnicodeString parametersDescription_;

    unsigned int maximumParameterCount_ = 0;
    unsigned int minimumParameterCount_ = 0;
};

/**
 * \file
 */

/**
 * Automatically registers a static instance of a ConsoleCommand subclass for use.
 */
#define CARBON_REGISTER_CONSOLE_COMMAND_SUBCLASS(ConsoleCommandSubclass)    \
    CARBON_UNIQUE_NAMESPACE                                                 \
    {                                                                       \
        static void registerConsoleCommandSubclass()                        \
        {                                                                   \
            static ConsoleCommandSubclass instance;                         \
            Carbon::console().registerCommand(&instance);                   \
        }                                                                   \
        CARBON_REGISTER_STARTUP_FUNCTION(registerConsoleCommandSubclass, 0) \
    }                                                                       \
    CARBON_UNIQUE_NAMESPACE_END

/**
 * Registers a simple console command with a function to call when the command is run and a function to call when autocomplete
 * possibilities need to be enumerated.
 */
#define CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(Name, ParametersDescription)         \
    CARBON_UNIQUE_NAMESPACE                                                                           \
    {                                                                                                 \
        class Name##ConsoleCommand : public Carbon::ConsoleCommand                                    \
        {                                                                                             \
        public:                                                                                       \
            Name##ConsoleCommand() : ConsoleCommand(#Name, ParametersDescription) {}                  \
            ~Name##ConsoleCommand() override {}                                                       \
                                                                                                      \
            void run(const Carbon::Vector<Carbon::UnicodeString>& parameters) override                \
            {                                                                                         \
                run##Name##ConsoleCommand(parameters);                                                \
            }                                                                                         \
                                                                                                      \
            void getTabCompletions(unsigned int parameterIndex,                                       \
                                   Carbon::Vector<Carbon::UnicodeString>& completions) const override \
            {                                                                                         \
                autocomplete##Name##ConsoleCommand(parameterIndex, completions);                      \
            }                                                                                         \
        };                                                                                            \
        CARBON_REGISTER_CONSOLE_COMMAND_SUBCLASS(Name##ConsoleCommand)                                \
    }                                                                                                 \
    CARBON_UNIQUE_NAMESPACE_END

/**
 * Registers a simple console command with a function to call when the command is run. This is identical to
 * CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE() except for there being no autocomplete function.
 */
#define CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(Name, ParametersDescription)                                               \
    CARBON_UNIQUE_NAMESPACE                                                                                               \
    {                                                                                                                     \
        static void autocomplete##Name##ConsoleCommand(unsigned int parameter, Carbon::Vector<Carbon::UnicodeString>&) {} \
        CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND_WITH_AUTOCOMPLETE(Name, ParametersDescription)                             \
    }                                                                                                                     \
    CARBON_UNIQUE_NAMESPACE_END

/**
 * Creates a static instance of the Parameter class with the given \a ParameterName and hooks it up to a console command of the
 * same name. This allows the parameter's value to be directly observed and altered on the console at runtime. Intended for use
 * during debugging.
 */
#define CARBON_CREATE_CONSOLE_PARAMETER(ParameterName, InitialValue)                        \
    static Carbon::Parameter ParameterName(InitialValue);                                   \
    static void run##ParameterName##ConsoleCommand(const Vector<UnicodeString>& parameters) \
    {                                                                                       \
        if (parameters.empty())                                                             \
            LOG_CONSOLE << #ParameterName ": " << ParameterName;                            \
        else                                                                                \
            ParameterName.setString(A(parameters[0]));                                      \
    }                                                                                       \
    CARBON_REGISTER_SIMPLE_CONSOLE_COMMAND(ParameterName, "[<value>]")

// Make CARBON_REGISTER_CONSOLE_COMMAND_SUBCLASS do nothing if console commands aren't included in the build
#ifndef CARBON_INCLUDE_CONSOLE_COMMANDS
    #undef CARBON_REGISTER_CONSOLE_COMMAND_SUBCLASS
    #define CARBON_REGISTER_CONSOLE_COMMAND_SUBCLASS(Type)
#endif
}
