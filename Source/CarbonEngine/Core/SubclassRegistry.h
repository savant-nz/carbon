/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Templated static class that manages global registration of the subclasses of a specific superclass type. Each
 * subclass is registered as a factory that is used for creation and destruction of that subclass type and can be looked
 * up by a public name or by a std::type_info. This class is currently used with Carbon::Entity and
 * Carbon::EntityController in order to allow their subclasses to be instantiated/destroyed by the engine as needed even
 * when those subclasses are defined inside client applications.
 */
template <typename SuperclassType> class CARBON_API SubclassRegistry
{
public:

    /**
     * Defines a simple factory interface for this superclass type, this is then implemented to create factories for
     * individual subclass types.
     */
    class Factory
    {
    public:

        /**
         * Constructs this factory with the specified public name and std::type_info.
         */
        Factory(String publicName, const std::type_info& typeInfo)
            : publicName_(std::move(publicName)), typeInfoName_(typeInfo.name())
        {
        }

        virtual ~Factory() {}

        /**
         * Returns the public name for the subclass type created/destroyed by this factory. This is used whenever the
         * type name needs to be persisted.
         */
        const String& getPublicName() const { return publicName_; }

        /**
         * Returns the std::type_info name for the subclass type created/destroyed by this factory. The format of this
         * varies by toolchain and so should never be persisted.
         */
        const String& getTypeInfoName() const { return typeInfoName_; }

        /**
         * Creates an instance.
         */
        virtual SuperclassType* create() = 0;

        /**
         * Destroys an instance.
         */
        virtual void destroy(SuperclassType* instance) = 0;

    private:

        const String publicName_;
        const String typeInfoName_;
    };

    /**
     * Adds the specified factory to this registry. This method is normally used through the CARBON_REGISTER_SUBCLASS()
     * macro rather than being called directly.
     */
    static void registerFactory(Factory* factory)
    {
        if (!getFactoryByPublicName(factory->getPublicName()) && !getFactoryByTypeInfoName(factory->getTypeInfoName()))
            factories_.append(factory);
    }

    /**
     * Returns the public type name for the given superclass instance, or an empty string if the subclass type is
     * unknown.
     */
    static const String& getPublicTypeName(const SuperclassType* instance)
    {
        auto factory = getFactoryByTypeInfoName(typeid(*instance).name());

        return factory ? factory->getPublicName() : String::Empty;
    }

    /**
     * Instantiates an instance of the specified subclass type, or null if the specified public type name is unknown.
     */
    static SuperclassType* create(const String& publicName)
    {
        auto factory = getFactoryByPublicName(publicName);

        auto newInstance = factory ? factory->create() : nullptr;

        if (newInstance)
            newInstance->wasCreatedThroughSubclassRegistry_ = true;

        return newInstance;
    }

    /**
     * Instantiates an instance of the specified subclass type, or null if the specified subclass type is unknown.
     */
    template <typename SubclassType> static SubclassType* create()
    {
        auto factory = getFactoryByTypeInfoName(typeid(SubclassType).name());

        auto newInstance = factory ? factory->create() : nullptr;

        if (newInstance)
            newInstance->wasCreatedThroughSubclassRegistry_ = true;

        return static_cast<SubclassType*>(newInstance);
    }

    /**
     * Destroys an instance of a subclass that was created through SubclassRegistry::create(). If the passed pointer is
     * null or points to an instance of a type that was not created through this SubclassRegistry then this method does
     * nothing. Returns success flag.
     */
    static bool destroy(SuperclassType* instance)
    {
        if (instance && instance->wasCreatedThroughSubclassRegistry_)
        {
            auto factory = getFactoryByTypeInfoName(typeid(*instance).name());

            if (factory)
            {
                factory->destroy(instance);
                return true;
            }
        }

        return false;
    }

private:

    static Vector<Factory*> factories_;

    static Factory* getFactoryByPublicName(const String& publicName)
    {
        return factories_.detect([&](const Factory* f) { return f->getPublicName() == publicName; }, nullptr);
    }

    static Factory* getFactoryByTypeInfoName(const String& typeInfoName)
    {
        return factories_.detect([&](const Factory* f) { return f->getTypeInfoName() == typeInfoName; }, nullptr);
    }
};

/**
 * \file
 */

#ifdef _MSC_VER
    #define CARBON_DECLARE_SUBCLASS_REGISTRY(SuperclassType)
#else
    /**
     * This macro declares a SubclassRegistry for the given superclass, declaring the static member that it needs.
     */
    #define CARBON_DECLARE_SUBCLASS_REGISTRY(SuperclassType) \
        template <> Vector<SubclassRegistry<SuperclassType>::Factory*> SubclassRegistry<SuperclassType>::factories_
#endif

/**
 * This macro defines a SubclassRegistry for the given superclass, instantiating the static member it needs so that it
 * can be linked against.
 */
#define CARBON_DEFINE_SUBCLASS_REGISTRY(SuperclassType) \
    typedef SubclassRegistry<SuperclassType> Registry;  \
    template <> Vector<Registry::Factory*> Registry::factories_ = Vector<Registry::Factory*>();

/**
 * Adds the specified \a SubclassType to the SubclassRegistry for the specified \a SuperclassType.
 */
#define CARBON_REGISTER_SUBCLASS(SubclassType, SuperclassType)                                                         \
    CARBON_UNIQUE_NAMESPACE                                                                                            \
    {                                                                                                                  \
        static class Factory : public Carbon::SubclassRegistry<SuperclassType>::Factory                                \
        {                                                                                                              \
        public:                                                                                                        \
            Factory() : Carbon::SubclassRegistry<SuperclassType>::Factory(#SubclassType, typeid(SubclassType)) {}      \
                                                                                                                       \
            SuperclassType* create() override                                                                          \
            {                                                                                                          \
                try                                                                                                    \
                {                                                                                                      \
                    return new SubclassType;                                                                           \
                }                                                                                                      \
                catch (const std::bad_alloc&)                                                                          \
                {                                                                                                      \
                    return nullptr;                                                                                    \
                }                                                                                                      \
            }                                                                                                          \
                                                                                                                       \
            void destroy(SuperclassType* instance) override { delete instance; }                                       \
        } factory;                                                                                                     \
        static void registerSubclassFactory() { Carbon::SubclassRegistry<SuperclassType>::registerFactory(&factory); } \
        CARBON_REGISTER_STARTUP_FUNCTION(registerSubclassFactory, 0)                                                   \
    }                                                                                                                  \
    CARBON_UNIQUE_NAMESPACE_END
}
