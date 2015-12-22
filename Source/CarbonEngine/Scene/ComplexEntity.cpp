/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Scene/ComplexEntity.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/Scene.h"
#include "CarbonEngine/Scene/SceneEvents.h"

namespace Carbon
{

const auto ComplexEntityVersionInfo = VersionInfo(1, 0);

ComplexEntity::~ComplexEntity()
{
    onDestruct();
    removeAllChildren();
    clear();
}

void ComplexEntity::clear()
{
    isLocalSpaceChildCullingEnabled_ = false;

    Entity::clear();
}

bool ComplexEntity::hasChild(const Entity* entity) const
{
    return children_.binarySearch<const Entity*>(entity) >= 0;
}

bool ComplexEntity::addChild(Entity* entity)
{
    if (!entity)
        return false;

    // If the passed entity is already a child of this entity then do nothing
    if (entity->getParent() == this)
        return true;

    // Check this is a registered entity type
    if (!SubclassRegistry<Entity>::getPublicTypeName(entity).length())
    {
        LOG_ERROR << "Unregistered entity type '" << typeid(*entity).name() << "'";
        return false;
    }

    // Store the entity's current parent, this will be null if it is not currently in a scene
    auto parent = entity->getParent();

    // If the entity is already in a scene then this method can be used to change its position in the scene graph, but not to
    // move it to a different scene
    if (entity->getScene() && entity->getScene() != getScene())
    {
        LOG_ERROR << "Entity can't be moved into a different scene: " << *entity;
        return false;
    }

    // Add the passed entity as a child of this complex entity
    auto index = children_.binarySearch(entity);
    children_.insert(-index - 1, entity);
    entity->parent_ = this;

    // If the entity was already in this scene and is just changing its position in the scene graph then update the old parent
    if (parent)
    {
        parent->children_.erase(parent->children_.binarySearch(entity));
        parent->invalidateParentExtents();
    }
    else
    {
        // If this complex entity is in a scene then add the whole entity hierarchy rooted at the new child entity into the
        // scene
        if (getScene())
        {
            auto newSceneEntities = Vector<Entity*>{entity};
            for (auto i = 0U; i < newSceneEntities.size(); i++)
            {
                getScene()->addEntityToInternalArray(newSceneEntities[i]);

                if (auto complex = dynamic_cast<ComplexEntity*>(newSceneEntities[i]))
                    newSceneEntities.append(complex->children_);
            }
        }
    }

    // Any cached scene graph information is now invalid
    entity->invalidateWorldTransform();
    entity->invalidateParentExtents();
    entity->invalidateIsVisibleIgnoreAlpha();
    entity->invalidateFinalAlpha();

    return true;
}

bool ComplexEntity::removeChild(Entity* entity)
{
    auto index = children_.binarySearch(entity);
    if (index < 0)
    {
        LOG_ERROR << "Specified child does not exist: " << entity;
        return false;
    }

    // Remove entity from parent
    children_.erase(index);

    // Any cached scene graph information is now invalid
    entity->invalidateWorldTransform();
    entity->invalidateIsVisibleIgnoreAlpha();
    entity->invalidateFinalAlpha();
    entity->invalidateParentExtents();

    entity->parent_ = nullptr;

    if (getScene())
    {
        auto entitiesToRemove = Vector<Entity*>{entity};

        for (auto i = 0U; i < entitiesToRemove.size(); i++)
        {
            auto e = entitiesToRemove[i];

            // Go through all child entities and add them to the list of entities to be removed
            if (auto complex = dynamic_cast<ComplexEntity*>(e))
                entitiesToRemove.append(complex->children_);

            // Remove the entity
            getScene()->removeEntityFromInternalArray(e);
        }

        // If the entity being deleted is a non-complex entity or has no children then try and destroy it using the entity type
        // registry
        if (!entity->isEntityType<ComplexEntity>() || static_cast<ComplexEntity*>(entity)->getChildCount() == 0)
            SubclassRegistry<Entity>::destroy(entity);
    }

    return true;
}

void ComplexEntity::removeAllChildren()
{
    while (!children_.empty())
        removeChild(children_.back());
}

void ComplexEntity::intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry)
{
    if (!isVisible())
        return;

    Entity::intersectRay(ray, intersections, onlyWorldGeometry);

    for (auto child : children_)
        child->intersectRay(ray, intersections, onlyWorldGeometry);
}

bool ComplexEntity::gatherGeometry(GeometryGather& gather)
{
    if (!Entity::gatherGeometry(gather))
        return false;

    // Do local space child culling if enabled
    if (isLocalSpaceChildCullingEnabled())
    {
        // Calculate the local space culling frustum
        auto localSpaceFrustum = gather.getFrustum() * getWorldTransform().getInverse();

        // Loop over all child entities trying to cull each one in local space
        for (auto child : children_)
        {
            if (child->isVisibleIgnoreAlpha(false) && localSpaceFrustum.intersect(child->getLocalExtents()))
                child->gatherGeometry(gather);
        }
    }
    else
    {
        // Propagate scene pass through child entities
        for (auto child : children_)
            child->gatherGeometry(gather);
    }

    return true;
}

void ComplexEntity::precache()
{
    for (auto child : children_)
        child->precache();

    Entity::precache();
}

bool ComplexEntity::invalidateWorldTransform(const String& attachmentPoint)
{
    if (Entity::invalidateWorldTransform(attachmentPoint))
    {
        // Propagate the invalidation through all child entities
        for (auto child : children_)
            child->invalidateWorldTransform();

        return true;
    }

    return false;
}

void ComplexEntity::invalidateIsVisibleIgnoreAlpha()
{
    if (isCachedIsVisibleIgnoreAlphaDirty_)
        return;

    Entity::invalidateIsVisibleIgnoreAlpha();

    for (auto child : children_)
        child->invalidateIsVisibleIgnoreAlpha();
}

void ComplexEntity::invalidateFinalAlpha()
{
    if (cachedFinalAlpha_ == -1.0f)
        return;

    Entity::invalidateFinalAlpha();

    for (auto child : children_)
        child->invalidateFinalAlpha();
}

void ComplexEntity::save(FileWriter& file) const
{
    // Save entity data
    Entity::save(file);

    // Write header
    file.beginVersionedSection(ComplexEntityVersionInfo);

    // Save children
    file.write(children_.size());
    for (auto child : children_)
        getScene()->saveEntityReference(file, child);

    file.endVersionedSection();
}

void ComplexEntity::load(FileReader& file)
{
    try
    {
        Entity::load(file);

        file.beginVersionedSection(ComplexEntityVersionInfo);

        // Read children
        auto childCount = 0U;
        file.read(childCount);
        for (auto i = 0U; i < childCount; i++)
            children_.append(getScene()->loadEntityReference(file));
        children_.sort();

        file.endVersionedSection();
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

ComplexEntity::operator UnicodeString() const
{
    return Entity::operator UnicodeString() + (getChildCount() ? (String() + ", direct children: " + getChildCount()) : "");
}

void ComplexEntity::calculateLocalExtents() const
{
    Entity::calculateLocalExtents();

    for (auto child : children_)
        localExtents_.merge(child->getLocalExtents(), getLocalTransform());
}

void ComplexEntity::calculateWorldExtents() const
{
    Entity::calculateWorldExtents();

    for (auto child : children_)
        worldExtents_.merge(child->getWorldExtents());
}

}
