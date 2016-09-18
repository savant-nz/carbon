/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Provides a generic system for registering multiple implementations of the specified \a InterfaceClass, each of which
 * is assigned a priority. This is used for managing the various implementations of PlatformInterface,
 * GraphicsInterface, SoundInterface and PhysicsInterface, and plays a central role in providing API and platform
 * independence.
 *
 * An implementation is created by InterfaceRegistry::create() by looking at all registered implementations, ordering
 * them by priority, and then trying to create and initialize each one until an implementation is successfully
 * initialized. See CARBON_REGISTER_INTERFACE_IMPLEMENTATION() for more details. The priority system can be overridden
 * by setting InterfaceRegistry::OverrideImplementationName.
 */
template <typename InterfaceClass> class CARBON_API InterfaceRegistry
{
public:

    /**
     * Holds details on an interface implementation that has been registered using
     * InterfaceRegistry::registerImplementation().
     */
    class Implementation
    {
    public:

        virtual ~Implementation() {}

        /**
         * Sets up this implementation with the passed name and priority.
         */
        Implementation(String name, unsigned int priority) : name_(std::move(name)), priority_(priority) {}

        /**
         * Returns the publicly displayable name of this implementation.
         */
        const String& getName() const { return name_; }

        /**
         * Returns the current priority of this implementation.
         */
        unsigned int getPriority() const { return priority_; }

        /**
         * Sets the current priority of this implementation. Note that changes to priority will only take effect during
         * the next call to InterfaceRegistry::create(). The default behavior is for higher priorty implementations get
         * first go at being created/initialized, however if InterfaceRegistry::OverrideImplementationName is set then
         * the implementation with that name gets first go, followed by all remaining implementations in order of
         * priority.
         */
        void setPriority(unsigned int priority) { priority_ = priority; }

        /**
         * Returns the creation function for this implementation.
         */
        virtual InterfaceClass* create() = 0;

        /**
         * Destroys the passed interface using this implementation's destroy function.
         */
        virtual void destroy(InterfaceClass* i) = 0;

    private:

        String name_;
        unsigned int priority_ = 0;
    };

    /**
     * Registers an implementation so that it is able to be created by InterfaceRegistry::create().
     */
    static void registerImplementation(Implementation* implementation)
    {
        if (!implementations_)
            implementations_ = new Vector<Implementation*>;

        implementations_->append(implementation);
    }

    /**
     * Unregisters the implementation that uses the given creation function.
     */
    static void unregisterImplementation(Implementation* implementation)
    {
        if (!implementations_)
            return;

        implementations_->eraseValue(implementation);

        if (implementations_->empty())
        {
            delete implementations_;
            implementations_ = nullptr;
        }
    }

    /**
     * When this is set it overrides the priority system used to order the registered implementations, this means that
     * if the name of an implementation is specified by this variable then that implementation will be given top
     * priority when calling InterfaceRegistry::create(). Applications that need to force the selection of a specific
     * implementation or backend should not set this value directly and should instead use the
     * CARBON_USE_INTERFACE_IMPLEMENTATION() macro.
     */
    static char OverrideImplementationName[256];

    /**
     * Iterates through all registered implementations in order of priority and returns the first one that successfully
     * instantiates and initializes. If InterfaceRegistry::OverrideImplementationName is set then the standard priority
     * ordering can be circumvented.
     */
    static InterfaceClass* create()
    {
        if (activeImplementation_)
        {
            LOG_ERROR << "There is already an active implementation";
            return nullptr;
        }

        if (!implementations_)
            return nullptr;

        // Sort implementations by priority
        implementations_->sortBy(
            [](Implementation* i0, Implementation* i1) { return i0->getPriority() > i1->getPriority(); });

        // If an override implementation is specified then put it first in the list
        for (auto i = 0U; i < implementations_->size(); i++)
        {
            if ((*implementations_)[i]->getName() == OverrideImplementationName)
            {
                implementations_->prepend((*implementations_)[i]);
                implementations_->erase(i + 1);
                break;
            }
        }

        for (auto implementation : *implementations_)
        {
            auto instance = implementation->create();

            if (instance && setup(instance))
            {
                if (strlen(OverrideImplementationName) && implementation->getName() != OverrideImplementationName)
                {
                    LOG_WARNING_WITHOUT_CALLER << "The override implementation was not used: "
                                               << OverrideImplementationName;
                }

                activeImplementation_ = implementation;
                activeInstance_ = instance;

                return instance;
            }

            implementation->destroy(instance);
        }

        return nullptr;
    }

    /**
     * Destroys the currently active interface instance.
     */
    static void destroy()
    {
        if (!activeInstance_)
            return;

        activeImplementation_->destroy(activeInstance_);

        activeImplementation_ = nullptr;
        activeInstance_ = nullptr;
    }

    /**
     * Returns the number of registered implementations.
     */
    static Vector<Implementation*> getImplementations()
    {
        if (implementations_)
            return *implementations_;

        return {};
    }

    /**
     * Returns a vector containing the names of all the registered implementations.
     */
    static Vector<String> getImplementationNames()
    {
        if (!implementations_)
            return {};

        return implementations_->template map<String>([](Implementation* i) { return i->getName(); });
    }

    /**
     * Returns a pointer to the currently active implementation, or null if there is no active implementation.
     */
    static Implementation* getActiveImplementation() { return activeImplementation_; }

    /**
     * Returns a pointer to the created instance of the currently active implementation, or null if there is no active
     * implementation.
     */
    static InterfaceClass* getActiveInstance() { return activeInstance_; }

private:

    static Vector<Implementation*>* implementations_;
    static Implementation* activeImplementation_;
    static InterfaceClass* activeInstance_;

    // This method is called by InterfaceRegistry::create() after it creates a new implementation in order to determine
    // whether the implementation is usable. This method must be implemented for each type that this class is used with,
    // and if it returns false then InterfaceRegistry::create() will not use the passed interface.
    static bool setup(InterfaceClass* i);
};

/**
 * \file
 */

/**
 * This macro instantiates an InterfaceRegistry for the given \a Interface class, the static members it needs are
 * defined so that they can be linked against. The macro should be followed immediately by the definition of the
 * InterfaceRegistry::setup() method that is specialized for the given \a Interface class.
 */
#define CARBON_DEFINE_INTERFACE_REGISTRY(InterfaceClass)                                                   \
    typedef Carbon::InterfaceRegistry<InterfaceClass> RegistryClass;                                       \
    template <> Carbon::Vector<RegistryClass::Implementation*>* RegistryClass::implementations_ = nullptr; \
    template <> RegistryClass::Implementation* RegistryClass::activeImplementation_ = nullptr;             \
    template <> InterfaceClass* RegistryClass::activeInstance_ = nullptr;                                  \
    template <> char RegistryClass::OverrideImplementationName[256] = {};                                  \
    template <> bool RegistryClass::setup(InterfaceClass* i)

/**
 * Registers an interface implementation with the relevant InterfaceRegistry so that it can be instantiated by
 * InterfaceRegistry::create().
 */
#define CARBON_REGISTER_INTERFACE_IMPLEMENTATION(InterfaceClass, ImplementationClass, Priority) \
    CARBON_UNIQUE_NAMESPACE                                                                     \
    {                                                                                           \
        typedef Carbon::InterfaceRegistry<InterfaceClass> RegistryClass;                        \
        static class Factory : public RegistryClass::Implementation                             \
        {                                                                                       \
        public:                                                                                 \
            Factory() : RegistryClass::Implementation(#ImplementationClass, Priority)           \
            {                                                                                   \
                RegistryClass::registerImplementation(this);                                    \
            }                                                                                   \
            ~Factory() { RegistryClass::unregisterImplementation(this); }                       \
            InterfaceClass* create() override { return new ImplementationClass; }               \
            void destroy(InterfaceClass* i) override { delete i; }                              \
        } implementation;                                                                       \
    }                                                                                           \
    CARBON_UNIQUE_NAMESPACE_END

/**
 * If an application wants to skip the default selection of an interface implementation based on priority ordering and
 * instead just specify the implementation to use then it can do so using this CARBON_USE_INTERFACE_IMPLEMENTATION()
 * macro. The macro should be placed in the application's main source file, e.g. alongside its include of
 * `CarbonEngine/EntryPoint.h`.
 */
#define CARBON_USE_INTERFACE_IMPLEMENTATION(InterfaceClass, ImplementationClass)        \
    CARBON_UNIQUE_NAMESPACE                                                             \
    {                                                                                   \
        typedef Carbon::InterfaceRegistry<InterfaceClass> RegistryClass;                \
        static struct UseInterfaceImplementation                                        \
        {                                                                               \
            UseInterfaceImplementation()                                                \
            {                                                                           \
                memcpy(RegistryClass::OverrideImplementationName, #ImplementationClass, \
                       strlen(#ImplementationClass) + 1);                               \
            }                                                                           \
        } useInterfaceImplementation;                                                   \
    }                                                                                   \
    CARBON_UNIQUE_NAMESPACE_END
}
