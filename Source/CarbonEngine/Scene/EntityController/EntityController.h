/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/SubclassRegistry.h"
#include "CarbonEngine/Platform/TimeValue.h"

namespace Carbon
{

/**
 * An entity controller is an object that is attached to an entity instance which it then alters the behavior of, this could
 * involve manipulating its transform, alpha, interactivity, or some other novel behavior not present on the original entity
 * instance. Subclasses of this class can do whatever they like to the entity being controlled, and multiple controllers can be
 * used on the same entity instance. Subclasses have to implement EntityController::update(), and should also implement
 * EntityController::save() and EntityController::load() if they have custom data that needs to be persisted.
 */
class CARBON_API EntityController : private Noncopyable
{
public:

    virtual ~EntityController() {}

    /**
     * Returns whether or not this entity controller is enabled, disabled entity controllers have no effect on the entity that
     * they are owned by. Entity controllers are enabled by default.
     */
    bool isEnabled() const { return isEnabled_; }

    /**
     * Sets whether or not this entity controller is enabled, disabled entity controllers have no effect on the entity that they
     * are owned by. Entity controllers are enabled by default.
     */
    virtual void setEnabled(bool enabled) { isEnabled_ = enabled; }

    /**
     * Toggles the enabled flag on this entity controller.
     */
    void toggleEnabled() { setEnabled(!isEnabled()); }

    /**
     * Updates this entity controller for the given timestep, this must be implemented by subclasses. If this entity controller
     * is both enabled and applied to an entity that is in a scene then this method will be called automatically once every
     * frame. The \a time parameter specifies the update timestep.
     *
     * A return value of false indicates that this entity controller has completed and so it will be subsequently deleted and
     * removed from the entity it was acting on. This is useful for automatically cleaning up entity controllers that are
     * designed to do one-off tasks.
     */
    virtual bool update(TimeValue time) = 0;

    /**
     * Saves this entity controller to a file stream. Subclasses should extend this method to persist any custom data they use.
     */
    virtual void save(FileWriter& file) const { file.write(isEnabled_); }

    /**
     * Loads this entity controller from a file stream. Subclasses should extend this method to persist any custom data they
     * use.
     */
    virtual void load(FileReader& file) { file.read(isEnabled_); }

protected:

    friend class Entity;

    /**
     * Returns the entity that this entity controller is owned by and acts on.
     */
    Entity* getEntity() { return entity_; }

    /**
     * \copydoc EntityController::getEntity()
     */
    const Entity* getEntity() const { return entity_; }

    /**
     * Returns the entity that this entity controller is owned by and acts on, cast to the specified \a EntityType.
     */
    template <typename EntityType> EntityType* getEntity() { return dynamic_cast<EntityType*>(entity_); }

    /**
     * This is called in order to set the entity that this entity controller acts on. The return value indicates whether the
     * passed entity is able to be controlled by this entity controller, and subclasses can extend this to check the type of
     * entity they are being used with, in case they are only able to work on specific types of entities. By default all
     * entities will be accepted.
     */
    virtual bool setEntity(Entity* entity)
    {
        assert(entity && "Entity to be controlled should not be null");

        entity_ = entity;
        return true;
    }

    /**
     * Returns the scene that this entity controller's entity is in.
     */
    Scene* getScene();

    /**
     * Returns the scene that this entity controller's entity is in.
     */
    const Scene* getScene() const;

private:

    Entity* entity_ = nullptr;

    bool isEnabled_ = true;

    friend class SubclassRegistry<EntityController>;
    bool wasCreatedThroughSubclassRegistry_ = false;
};

/**
 * This macro should be put in the primary source file for every entity controller subclass in order to register the subclass
 * type.
 */
#define CARBON_REGISTER_ENTITY_CONTROLLER_SUBCLASS(EntityControllerSubclassType) \
    CARBON_REGISTER_SUBCLASS(EntityControllerSubclassType, Carbon::EntityController)
}

#include "CarbonEngine/Scene/Entity.h"
