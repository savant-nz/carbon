/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Public interface for managing and executing scripts, the AngelScript library is used internally to implement
 * scripting support.
 */
class CARBON_API ScriptManager
{
public:

    /**
     * The directory which scripts are stored under, currently "Scripts/".
     */
    static const UnicodeString ScriptDirectory;

    /**
     * The file extension for materials, currently ".script".
     */
    static const UnicodeString ScriptExtension;

    /**
     * Holds a reference to an executable instance of a script, this can be used to suspend, resume and terminate the
     * script.
     */
    typedef void* ScriptInstance;

    ScriptManager();
    ~ScriptManager();

    /**
     * Initializes the AngelScript engine, this method is for internal use only. Returns success flag.
     */
    bool setup();

    /**
     * Registers a function that will be accessible to all scripts that run. \a declaration describes the return value
     * and function parameters and must be formatted correctly using AngelScript-style syntax, e.g. "void foo()", "int
     * foo(String &in)". Returns -1 on error, or the ID of the new global function on success.
     */
    int registerGlobalFunction(const String& declaration, VoidFunction function);

    /**
     * \copydoc registerGlobalFunction(const String &, VoidFunction function)
     */
    template <typename FunctionType> int registerGlobalFunction(const String& declaration, FunctionType function)
    {
        return registerGlobalFunction(declaration, reinterpret_cast<VoidFunction>(function));
    }

    /**
     * Registers a class method so that it will be accessible as a global function to all scripts that run. \a
     * declaration describes the return value and method parameters and must be formatted correctly using
     * AngelScript-style syntax, e.g. `void foo()`, `int foo(String& in)`. The \a instance parameter specifies the
     * instance of the class that the method will be run on. This instance pointer can be updated or nulled using
     * ScriptManager::setGlobalFunctionClassInstance() if the class instance is destructed or the application wants to
     * change it. Note that a null class instance pointer results in calls to the affected global function from within a
     * script being a no-op, which is useful because global functions can't be removed. The caller is responsible for
     * ensuring that the class instance for the global function remains valid. Returns the ID of the new global function
     * on success or -1 on failure.
     */
    template <typename ClassType, typename ReturnType, typename... ArgTypes>
    int registerGlobalFunction(const String& declaration, ReturnType (ClassType::*method)(ArgTypes...),
                               ClassType* instance)
    {
        return registerGlobalFunction(declaration, proxyGlobalFunctionToMethodCall<ClassType, ReturnType, ArgTypes...>,
                                      instance, method);
    }

    /**
     * When a global scripting function is hooked up to a method call on a class using one of the
     * ScriptManager::registerGlobalFunction() overloads a pointer to the class instance is stored, this method allows
     * that pointer to be changed so that the method call gets sent to a different instance of the class. If the
     * instance pointer is set to null then calls to the affected global function made by any scripts become no-ops. The
     * application is responsible for ensuring that the class instance is always either valid or null.
     */
    template <typename ClassType> void setGlobalFunctionClassInstance(int functionID, ClassType* instance)
    {
        setGlobalFunctionCallbackPointers(functionID, reinterpret_cast<void*>(instance));
    }

    /**
     * Registers a global variable that can be read from and written to by all scripts using the specified variable
     * name. The accepted types for \a var are: `bool&`, `int&`, `unsigned int&`, `float&` and `String&`. The caller is
     * responsible for ensuring that the passed variable reference is valid whenever a script accesses it. Returns
     * success flag.
     */
    template <typename VariableType> bool registerGlobalVariable(const String& name, VariableType& var)
    {
        return registerGlobalVariable(name, &var);
    }

    /**
     * Loads the specified script and executes it, the script will execute until it either returns from its `main()`
     * function or calls `sleep()` or `suspend()`. Returns the new script instance on success, or null on failure.
     */
    ScriptInstance run(const String& scriptName);

    /**
     * Returns a handle to the currently executing script, or null if no script is currently executing. This is only
     * ever non-null when called from inside a global function that was registered with
     * ScriptManager::registerGlobalFunction().
     */
    ScriptInstance getCurrentScript() const;

    /**
     * This method suspends execution of a script instance for the specified amount of time. If \a seconds is zero then
     * the script is suspended indefinitely until it is resumed with ScriptManager::resume(). Returns success flag.
     */
    bool suspend(ScriptInstance script, float seconds = 0.0f);

    /**
     * Resumes execution of the specified script instance. Returns success flag.
     */
    bool resume(ScriptInstance script);

    /**
     * Terminates execution of the specified script instance. Returns success flag.
     */
    bool terminate(ScriptInstance script);

private:

    class Members;
    Members* m = nullptr;

    void registerBuiltInFunctions();

    // Wrapper that creates a global function that hooks up directly to the specified method on the passed class
    // instance. The proxy function templates are defined below, these are static functions that take calls to the
    // global function done in a script and convert them to the corresponding method call.
    template <typename ClassType, typename MethodType, typename ProxyFunctionType>
    int registerGlobalFunction(const String& declaration, ProxyFunctionType fnProxy, ClassType* instance,
                               MethodType method)
    {
        auto result = registerGlobalFunction(declaration.cStr(), fnProxy);
        if (result != -1)
            setGlobalFunctionCallbackPointers(result, instance, *reinterpret_cast<void**>(&method));

        return result;
    }

    // These methods store and retrieve the class instance and method pointer for those global functions that are linked
    // directly up to an instance method on a class. The functions are indexed by the function ID returned by
    // registerGlobalFunction().
    void setGlobalFunctionCallbackPointers(int functionID, void* instance, void* method = nullptr);
    bool getCurrentGlobalFunctionCallbackPointers(void** instance, void** method);

    // Static proxy functions that allow an instance method of a class to be called from a script
    template <typename ClassType, typename ReturnType, typename... ArgTypes>
    static ReturnType proxyGlobalFunctionToMethodCall(ArgTypes&&... args)
    {
        typedef ReturnType (ClassType::*MethodType)(ArgTypes...);

        auto instance = pointer_to<ClassType>::type();
        auto method = MethodType();

        if (!scripts().getCurrentGlobalFunctionCallbackPointers(reinterpret_cast<void**>(&instance),
                                                                reinterpret_cast<void**>(&method)))
            return {};

        return (instance->*method)(std::forward<ArgTypes>(args)...);
    }

    bool registerGlobalVariable(const String& name, const String& type, void* var);
    bool registerGlobalVariable(const String& name, bool* var) { return registerGlobalVariable(name, "bool", &var); }
    bool registerGlobalVariable(const String& name, int* var) { return registerGlobalVariable(name, "int", &var); }
    bool registerGlobalVariable(const String& name, unsigned int* var)
    {
        return registerGlobalVariable(name, "uint", &var);
    }
    bool registerGlobalVariable(const String& name, float* var) { return registerGlobalVariable(name, "float", &var); }
    bool registerGlobalVariable(const String& name, String* var)
    {
        return registerGlobalVariable(name, "String", &var);
    }
};

}
