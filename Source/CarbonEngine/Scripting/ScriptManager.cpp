/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Scripting/ScriptManager.h"

namespace Carbon
{

const UnicodeString ScriptManager::ScriptDirectory = "Scripts/";
const UnicodeString ScriptManager::ScriptExtension = ".script";

}

#ifdef CARBON_INCLUDE_ANGELSCRIPT

#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Scripting/AngelScriptIncludeWrapper.h"

#ifdef _MSC_VER
    #pragma comment(lib, "AngelScript" CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX)
#endif

namespace Carbon
{

class ScriptManager::Members : public EventHandler
{
public:

    asIScriptEngine* engine = nullptr;

    struct ActiveScript
    {
        String moduleName;
        asIScriptContext* context = nullptr;

        bool isWaitingForRestart = false;
        TimeValue wakeTime;

        ActiveScript() {}
        ActiveScript(String moduleName_, asIScriptContext* context_)
            : moduleName(std::move(moduleName_)), context(context_)
        {
        }

        void release()
        {
            auto scriptEngine = context->GetEngine();
            context->Release();
            context = nullptr;
            scriptEngine->DiscardModule(moduleName.cStr());
        }
    };
    Vector<ActiveScript> activeScripts;

    ActiveScript* findActiveScript(asIScriptContext* context);

    std::unordered_map<int, std::pair<void*, void*>> globalFunctionCallbackPointers;

    static void messageCallback(const asSMessageInfo* msg, void* param);
    static String errorToString(int error);

    void registerStringType();
    void registerStringBehavior(asEBehaviours behavior, const char* declaration, const asSFuncPtr& function);
    void registerStringMethod(const char* declaration, const asSFuncPtr& function, asDWORD callConv);

    bool processEvent(const Event& e) override;
};

ScriptManager::ScriptManager()
{
    m = new Members;
}

ScriptManager::~ScriptManager()
{
    // Clean up any active scripts
    for (auto& script : m->activeScripts)
        script.release();

    if (m->engine)
    {
        m->engine->Release();
        m->engine = nullptr;
    }

    events().removeHandler(m);

    delete m;
    m = nullptr;
}

ScriptManager::Members::ActiveScript* ScriptManager::Members::findActiveScript(asIScriptContext* context)
{
    for (auto& script : activeScripts)
    {
        if (script.context == context)
            return &script;
    }

    return nullptr;
}

void ScriptManager::Members::messageCallback(const asSMessageInfo* msg, void* param)
{
    auto message = UnicodeString(msg->section) + " (" + msg->row + ", " + msg->col + ") : " + msg->message;

    if (msg->type == asMSGTYPE_ERROR)
        LOG_ERROR_WITHOUT_CALLER << message;
    else if (msg->type == asMSGTYPE_WARNING)
        LOG_WARNING_WITHOUT_CALLER << message;
    else
        LOG_INFO << message;
}

String ScriptManager::Members::errorToString(int error)
{
#define HANDLE_ANGELSCRIPT_ERROR(Error) \
    if (error == as##Error)             \
        return #Error;

    HANDLE_ANGELSCRIPT_ERROR(SUCCESS)
    HANDLE_ANGELSCRIPT_ERROR(ERROR)
    HANDLE_ANGELSCRIPT_ERROR(CONTEXT_ACTIVE)
    HANDLE_ANGELSCRIPT_ERROR(CONTEXT_NOT_FINISHED)
    HANDLE_ANGELSCRIPT_ERROR(CONTEXT_NOT_PREPARED)
    HANDLE_ANGELSCRIPT_ERROR(INVALID_ARG)
    HANDLE_ANGELSCRIPT_ERROR(NO_FUNCTION)
    HANDLE_ANGELSCRIPT_ERROR(NOT_SUPPORTED)
    HANDLE_ANGELSCRIPT_ERROR(INVALID_NAME)
    HANDLE_ANGELSCRIPT_ERROR(NAME_TAKEN)
    HANDLE_ANGELSCRIPT_ERROR(INVALID_DECLARATION)
    HANDLE_ANGELSCRIPT_ERROR(INVALID_OBJECT)
    HANDLE_ANGELSCRIPT_ERROR(INVALID_TYPE)
    HANDLE_ANGELSCRIPT_ERROR(ALREADY_REGISTERED)
    HANDLE_ANGELSCRIPT_ERROR(MULTIPLE_FUNCTIONS)
    HANDLE_ANGELSCRIPT_ERROR(NO_MODULE)
    HANDLE_ANGELSCRIPT_ERROR(NO_GLOBAL_VAR)
    HANDLE_ANGELSCRIPT_ERROR(INVALID_CONFIGURATION)
    HANDLE_ANGELSCRIPT_ERROR(INVALID_INTERFACE)
    HANDLE_ANGELSCRIPT_ERROR(CANT_BIND_ALL_FUNCTIONS)
    HANDLE_ANGELSCRIPT_ERROR(LOWER_ARRAY_DIMENSION_NOT_REGISTERED)
    HANDLE_ANGELSCRIPT_ERROR(WRONG_CONFIG_GROUP)
    HANDLE_ANGELSCRIPT_ERROR(CONFIG_GROUP_IS_IN_USE)
    HANDLE_ANGELSCRIPT_ERROR(ILLEGAL_BEHAVIOUR_FOR_TYPE)
    HANDLE_ANGELSCRIPT_ERROR(WRONG_CALLING_CONV)
    HANDLE_ANGELSCRIPT_ERROR(BUILD_IN_PROGRESS)
    HANDLE_ANGELSCRIPT_ERROR(INIT_GLOBAL_VARS_FAILED)

    return String() << "Unknown error code: " << error;
}

// Helper methods that wrap the String type through into AngelScript, used by registerStringType()
static String stringCreate(asUINT length, const char* s)
{
    return s;
}

static void stringDefaultConstruct(String* s)
{
    new (s) String();
}

template <typename ConstructType> static void stringConstruct(ConstructType value, String* thisPointer)
{
    new (thisPointer) String(value);
}

static void stringDestruct(String* s)
{
    s->~String();
}

static int stringCompare(const String& a, const String& b)
{
    return (a < b) ? -1 : (a > b ? 1 : 0);
}

template <typename AssignType> static String& stringAssign(AssignType value, String& string)
{
    string = value;
    return string;
}

void ScriptManager::Members::registerStringType()
{
    // Register the String type and factory
    engine->RegisterObjectType("String", sizeof(String), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
    engine->RegisterStringFactory("String", asFUNCTION(stringCreate), asCALL_CDECL);

    // Register String constructors and destructors
    registerStringBehavior(asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(stringDefaultConstruct));
    registerStringBehavior(asBEHAVE_CONSTRUCT, "void f(const String& in)", asFUNCTION(stringConstruct<const String&>));
    registerStringBehavior(asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(stringConstruct<int>));
    registerStringBehavior(asBEHAVE_CONSTRUCT, "void f(uint)", asFUNCTION(stringConstruct<unsigned int>));
    registerStringBehavior(asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(stringConstruct<float>));
    registerStringBehavior(asBEHAVE_DESTRUCT, "void f()", asFUNCTION(stringDestruct));

    // Register core String methods
    registerStringMethod("String& opAssign(const String& in)", asMETHODPR(String, operator=, (const String&), String&),
                         asCALL_THISCALL);
    registerStringMethod("String& opAssign(int)", asFUNCTION(stringAssign<int>), asCALL_CDECL_OBJLAST);
    registerStringMethod("String& opAssign(uint)", asFUNCTION(stringAssign<unsigned int>), asCALL_CDECL_OBJLAST);
    registerStringMethod("String& opAssign(float)", asFUNCTION(stringAssign<float>), asCALL_CDECL_OBJLAST);
    registerStringMethod("String& opAddAssign(const String& in)", asMETHOD(String, operator+=), asCALL_THISCALL);
    registerStringMethod("bool opEquals(const String& in) const", asMETHOD(String, operator==), asCALL_THISCALL);
    registerStringMethod("int opCmp(const String& in) const", asFUNCTION(stringCompare), asCALL_CDECL_OBJFIRST);
    registerStringMethod("String opAdd(const String& in) const", asMETHOD(String, operator+), asCALL_THISCALL);
    registerStringMethod("uint length() const", asMETHOD(String, length), asCALL_THISCALL);
}

void ScriptManager::Members::registerStringBehavior(asEBehaviours behavior, const char* declaration,
                                                    const asSFuncPtr& function)
{
    engine->RegisterObjectBehaviour("String", behavior, declaration, function, asCALL_CDECL_OBJLAST);
}

void ScriptManager::Members::registerStringMethod(const char* declaration, const asSFuncPtr& function, asDWORD callConv)
{
    engine->RegisterObjectMethod("String", declaration, function, callConv);
}

bool ScriptManager::Members::processEvent(const Event&)
{
    for (auto i = 0U; i < activeScripts.size(); i++)
    {
        auto& script = activeScripts[i];

        // If this script is sleeping on a timeout then resume it if nap time is over
        if (script.isWaitingForRestart && platform().getTime() >= script.wakeTime)
            scripts().resume(script.context);

        // Clean up the script unless it is suspended
        if (script.context->GetState() != asEXECUTION_SUSPENDED)
        {
            script.release();
            activeScripts.erase(i--);
        }
    }

    return true;
}

bool ScriptManager::setup()
{
    m->engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    if (!m->engine)
    {
        LOG_ERROR << "Failed creating scripting engine";
        return false;
    }

    // Register a message callback
    m->engine->SetMessageCallback(asFUNCTION(Members::messageCallback), nullptr, asCALL_CDECL);

    m->registerStringType();
    registerBuiltInFunctions();

    events().addHandler<UpdateEvent>(m);

    return true;
}

int ScriptManager::registerGlobalFunction(const String& declaration, VoidFunction function)
{
    auto result = m->engine->RegisterGlobalFunction(declaration.cStr(), asFUNCTION(function), asCALL_CDECL);
    if (result < 0)
    {
        LOG_ERROR << "Failed registering global function, error: " << m->errorToString(result);
        return -1;
    }

    return result;
}

void ScriptManager::setGlobalFunctionCallbackPointers(int functionID, void* instance, void* method)
{
    m->globalFunctionCallbackPointers[functionID].first = instance;

    if (method)
        m->globalFunctionCallbackPointers[functionID].second = method;
}

bool ScriptManager::getCurrentGlobalFunctionCallbackPointers(void** instance, void** method)
{
    auto functionID = asGetActiveContext()->GetSystemFunction()->GetId();

    std::tie(*instance, *method) = m->globalFunctionCallbackPointers.find(functionID)->second;

    return *instance && *method;
}

bool ScriptManager::registerGlobalVariable(const String& name, const String& type, void* variable)
{
    if (m->engine->RegisterGlobalProperty((type + String::Space + name).cStr(), variable) < 0)
    {
        LOG_ERROR << "Failed registering global script variable '" << type << " " << name << "'";
        return false;
    }

    return true;
}

ScriptManager::ScriptInstance ScriptManager::run(const String& scriptName)
{
    auto moduleName = Math::createGUID();

    auto module = pointer_to<asIScriptModule>::type();
    auto context = pointer_to<asIScriptContext>::type();

    auto error = 0;

    try
    {
        auto s = UnicodeString();
        if (!fileSystem().readTextFile(ScriptDirectory + scriptName + ScriptExtension, s))
            throw Exception("Failed reading script file");

        module = m->engine->GetModule(moduleName.cStr(), asGM_ALWAYS_CREATE);
        if ((error = module->AddScriptSection(scriptName.cStr(), A(s).cStr(), s.length())) < 0)
            throw Exception("Failed adding script to module");

        if ((error = module->Build()) < 0)
            throw Exception("Failed building script");

        auto entryPoint = module->GetFunctionByDecl("void main()");
        if (!entryPoint)
            throw Exception("Could not find 'void main()' entry point");

        context = m->engine->CreateContext();
        if ((error = context->Prepare(entryPoint)) < 0)
            throw Exception("Failed preparing execution context");

        m->activeScripts.emplace(moduleName, context);

        if ((error = context->Execute()) < 0)
        {
            m->activeScripts.popBack();
            throw Exception("Failed executing script");
        }

        return context;
    }
    catch (const Exception& e)
    {
        if (context)
            context->Release();
        if (module)
            m->engine->DiscardModule(moduleName.cStr());

        LOG_ERROR << scriptName << " - " << e << (error < 0 ? "(" + m->errorToString(error) + ")" : "");

        return nullptr;
    }
}

ScriptManager::ScriptInstance ScriptManager::getCurrentScript() const
{
    return asGetActiveContext();
}

bool ScriptManager::suspend(ScriptInstance script, float seconds)
{
    auto activeScript = m->findActiveScript(reinterpret_cast<asIScriptContext*>(script));
    if (!activeScript)
        return false;

    // Suspend only makes sense if the script is currently active or currently suspended
    if (activeScript->context->GetState() != asEXECUTION_ACTIVE &&
        activeScript->context->GetState() != asEXECUTION_SUSPENDED)
        return false;

    // Suspend the script
    activeScript->context->Suspend();

    // If a timeout has been set then store the needed values
    activeScript->isWaitingForRestart = (seconds > 0.0f);
    if (activeScript->isWaitingForRestart)
        activeScript->wakeTime = platform().getTime() + seconds;

    return true;
}

bool ScriptManager::resume(ScriptInstance script)
{
    auto activeScript = m->findActiveScript(reinterpret_cast<asIScriptContext*>(script));
    if (!activeScript)
        return false;

    // Resume only makes sense if the script is currently suspended
    if (activeScript->context->GetState() != asEXECUTION_SUSPENDED)
        return false;

    activeScript->isWaitingForRestart = false;

    auto result = activeScript->context->Execute();
    if (result < 0)
    {
        LOG_ERROR << "Failed resuming script, error: " << m->errorToString(result);
        return false;
    }

    return true;
}

bool ScriptManager::terminate(ScriptInstance script)
{
    auto activeScript = m->findActiveScript(reinterpret_cast<asIScriptContext*>(script));
    if (!activeScript)
        return false;

    auto result = activeScript->context->Abort();
    if (result < 0)
    {
        LOG_ERROR << "Failed terminating script, error: " << m->errorToString(result);
        return false;
    }

    // The relevant entry in active scripts will be cleaned up on the next UpdateEvent

    return true;
}

}

#else

// No scripting support in this build

namespace Carbon
{

ScriptManager::ScriptManager()
{
}

ScriptManager::~ScriptManager()
{
}

bool ScriptManager::setup()
{
    return true;
}

int ScriptManager::registerGlobalFunction(const String& declaration, VoidFunction function)
{
    return -1;
}

void ScriptManager::setGlobalFunctionCallbackPointers(int functionID, void* instance, void* method)
{
}

bool ScriptManager::getCurrentGlobalFunctionCallbackPointers(void** instance, void** method)
{
    return false;
}

ScriptManager::ScriptInstance ScriptManager::run(const String& scriptName)
{
    return nullptr;
}

ScriptManager::ScriptInstance ScriptManager::getCurrentScript() const
{
    return nullptr;
}

bool ScriptManager::suspend(ScriptInstance script, float seconds)
{
    return false;
}

bool ScriptManager::resume(ScriptInstance script)
{
    return false;
}

bool ScriptManager::terminate(ScriptInstance script)
{
    return false;
}

}

#endif
