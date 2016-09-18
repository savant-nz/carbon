/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/Entity.h"

namespace Carbon
{

/**
 * ComplexEntity is an entity that can have child entities, this is what the scene graph is built with.
 */
class CARBON_API ComplexEntity : public Entity
{
public:

    ComplexEntity() { clear(); }
    ~ComplexEntity() override;

    /**
     * Returns the number of child entities that this ComplexEntity has.
     */
    unsigned int getChildCount() const { return children_.size(); }

    /**
     * Returns the child entity at the given index.
     */
    const Entity* getChild(unsigned int index) const { return children_[index]; }

    /**
     * Returns the child entity at the given index.
     */
    Entity* getChild(unsigned int index) { return children_[index]; }

    /**
     * Returns the child entity at the given index cast to the specified entity type, or null if the cast is not
     * possible.
     */
    template <typename ChildEntityType> const ChildEntityType* getChild(unsigned int index) const
    {
        return dynamic_cast<const ChildEntityType*>(children_[index]);
    }

    /**
     * Returns the child entity at the given index cast to the specified entity type, or null if the cast is not
     * possible.
     */
    template <typename ChildEntityType> ChildEntityType* getChild(unsigned int index)
    {
        return dynamic_cast<ChildEntityType*>(children_[index]);
    }

    /**
     * Returns the child entity of the specified type and with the given name, or null if such a child entity is not
     * found.
     */
    template <typename ChildEntityType> const ChildEntityType* getChild(const String& name) const
    {
        for (auto child : children_)
        {
            if (child->getName() == name)
            {
                auto result = dynamic_cast<const ChildEntityType*>(child);
                if (result)
                    return result;
            }
        }

        return nullptr;
    }

    /**
     * Returns the child entity of the specified type and with the given name, or null if such a child entity is not
     * found.
     */
    template <typename ChildEntityType> ChildEntityType* getChild(const String& name)
    {
        for (auto child : children_)
        {
            if (child->getName() == name)
            {
                auto result = dynamic_cast<const ChildEntityType*>(child);
                if (result)
                    return result;
            }
        }

        return nullptr;
    }

    /**
     * Returns the internal vector of the child entities of this complex entity.
     */
    const Vector<Entity*> getChildren() const { return children_; }

    /**
     * Returns all the child entities that are of the specified type.
     */
    template <typename ChildEntityType> Vector<ChildEntityType*> getChildren(bool includeInternalEntities = false)
    {
        auto children = Vector<ChildEntityType*>();

        for (auto child : children_)
        {
            auto result = dynamic_cast<ChildEntityType*>(child);
            if (result && (includeInternalEntities || !child->isInternalEntity()))
                children.append(result);
        }

        return children;
    }

    /**
     * Returns all the child entities of this complex entity.
     */
    Vector<Entity*> getChildren(bool includeInternalEntities = false)
    {
        return getChildren<Entity>(includeInternalEntities);
    }

    /**
     * Returns whether this complex entity has the specified child entity.
     */
    bool hasChild(const Entity* entity) const;

    /**
     * Adds an entity as a child of this complex entity, whole entity hierarchies will automatically be added in one
     * call if the passed entity is a complex entity that has children. If the passed entity is already a child of this
     * complex entity then nothing will be done and true will be returned. If the passed entity is already in the same
     * scene as this complex entity then this method will update the location in the scene graph hierarchy of the passed
     * entity and any child entities it has, this behavior allows individual entities, as well as whole entity subtrees,
     * to be quickly and efficiently relocated to a new position in the scene graph hierarchy. Returns success flag.
     */
    virtual bool addChild(Entity* entity);

    /**
     * Creates a new entity of the specified type and adds it as a child of this complex entity, returning the new
     * entity instance. The name of the new child entity is specified by \a name, and the new entity's `initialize()`
     * method will be called with any additional arguments that are passed.
     */
    template <typename EntityType, typename... ArgTypes> EntityType* addChild(const String& name, ArgTypes&&... args)
    {
        auto entity = SubclassRegistry<Entity>::create<EntityType>();
        if (!entity || !addChild(entity))
        {
            SubclassRegistry<Entity>::destroy(entity);
            return nullptr;
        }

        entity->setName(name);
        initializeIfArgsPassed(entity, std::forward<ArgTypes>(args)...);

        return entity;
    }

    /**
     * Creates a new entity of the specified type and adds it as a child of this complex entity, returning the new
     * entity instance.
     */
    template <typename EntityType> EntityType* addChild() { return addChild<EntityType>(String::Empty); }

    /**
     * Removes a child of this complex entity, if this complex entity is in a scene and the passed child is a complex
     * entity with children then all the children will also be removed from the scene. If there is no hierarchy under
     * the passed entity, this complex entity is in a scene, and the passed entity was created by the engine through
     * Scene::addEntity<>() or ComplexEntity::addChild<>(), then the entity will be automatically deleted following its
     * removal from the scene. Returns success flag.
     */
    virtual bool removeChild(Entity* entity);

    /**
     * Removes all children of this complex entity by repeatedly calling ComplexEntity::removeChild().
     */
    void removeAllChildren();

    /**
     * Returns whether local space culling of child entities is enabled on this complex entity, defaults to false. This
     * can be used to improve culling speed when this complex entity is being moved every frame (or very frequently) but
     * when the majority of its children do not move relative to it. Normal culling procedure would require the world
     * space transform of the child entities to be recalculated every frame in order for culling to be done, however if
     * local space child culling is enabled then the culling of child entities will instead be done by this complex
     * entity in local space which avoids the calculation of the child world space transforms. Note that this means that
     * the culling uses the child's local extents as returned by Entity::getLocalExtents() as the complex entity needs
     * to determine the bounds around the entire entity subtree rooted at each child entity in order to safely cull that
     * branch.
     */
    bool isLocalSpaceChildCullingEnabled() const { return isLocalSpaceChildCullingEnabled_; }

    /**
     * Sets whether local space culling of child entities is enabled on this complex entity, see
     * Entity::isLocalSpaceChildCullingEnabled() for details.
     */
    void setLocalSpaceChildCullingEnabled(bool enabled) { isLocalSpaceChildCullingEnabled_ = enabled; }

    void clear() override;
    void intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry) override;
    bool gatherGeometry(GeometryGather& gather) override;
    void precache() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;
    operator UnicodeString() const override;
    bool invalidateWorldTransform(const String& attachmentPoint = String::Empty) override;
    void invalidateIsVisibleIgnoreAlpha() override;
    void invalidateFinalAlpha() override;

private:

    Vector<Entity*> children_;

    bool isLocalSpaceChildCullingEnabled_ = false;

    void calculateLocalExtents() const override;
    void calculateWorldExtents() const override;

    friend class Scene;
};

}
