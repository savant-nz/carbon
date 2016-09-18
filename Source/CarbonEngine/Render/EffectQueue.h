/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Render/RenderQueueItemArray.h"
#include "CarbonEngine/Render/Texture/Texture.h"

namespace Carbon
{

/**
 * This class holds specifies an Effect, a set of parameters to render that effect with, and a RenderQueueItemArray that
 * specifies what should be rendered. This class is the basis of the EffectQueueArray class which is what scenes
 * populate when they are queried for visible geometry during rendering.
 */
class CARBON_API EffectQueue : private Noncopyable
{
public:

    /**
     * Constructs this effect queue with the specified priority, effect and internal parameters.
     */
    EffectQueue(int priority, Effect* effect, const ParameterArray& internalParams = ParameterArray::Empty)
        : priority_(priority), effect_(effect), internalParams_(internalParams)
    {
        assert(effect && "Effect must not be null");
    }

    /**
     * Copy constructor (not implemented).
     */
    EffectQueue(const EffectQueue& other);

    /**
     * Returns the priority for this effect queue.
     */
    int getPriority() const { return priority_; }

    /**
     * Returns the effect used by this effect queue.
     */
    const Effect* getEffect() const { return effect_; }

    /**
     * Returns the effect used by this effect queue.
     */
    Effect* getEffect() { return effect_; }

    /**
     * Returns the effect parameters for this effect queue.
     */
    const ParameterArray& getParams() const { return *params_; }

    /**
     * Returns the RenderQueueItemArray for this effect queue.
     */
    const RenderQueueItemArray& getItems() const { return items_; }

    /**
     * Returns the RenderQueueItemArray for this effect queue.
     */
    RenderQueueItemArray& getItems() { return items_; }

    /**
     * Tells this effect queue to use the specified ParameterArray for its main parameters, the caller is responsible
     * for ensuring that the passed \a params instance stays valid until this effect queue instance destructs.
     */
    void useParams(const ParameterArray& params) { params_ = &params; }

    /**
     * Sets the value of a custom parameter on this queue's parameters, this will overwrite any existing parameter of
     * the same name or lookup.
     */
    void setCustomParameter(const ParameterArray::Lookup& lookup, const Parameter& value)
    {
        if (!hasCustomParams_)
        {
            customParams_ = *params_;
            params_ = &customParams_;
            hasCustomParams_ = true;
        }

        customParams_.set(lookup, value);
    }

    /**
     * Returns whether this queue's parameters are a custom temporary set that will be deallocated when this queue
     * destructs.
     */
    bool hasCustomParams() const { return hasCustomParams_; }

    /**
     * Returns the internal parameters that are set and controlled by the engine internally.
     */
    const ParameterArray& getInternalParams() const { return internalParams_; }

    /**
     * Returns the render sort key for this queue. Used by the renderer to sort queues with the same effect by their
     * parameters, see Shader::getSortKey() for details
     */
    unsigned int getSortKey() const { return sortKey_; }

    /**
     * Sets the render sort key for this queue.
     */
    void setSortKey(unsigned int key) const { sortKey_ = key; }

    /**
     * Adds a texture animation update to this queue.
     */
    void addTextureAnimation(const Texture* texture, unsigned int frame) { textureAnimations_.emplace(texture, frame); }

    /**
     * Applies all added texture animation updates to the underlying texture objects.
     */
    void applyTextureAnimations() const
    {
        for (auto& animation : textureAnimations_)
            const_cast<Texture*>(animation.first)->setCurrentFrame(animation.second);
    }

private:

    const int priority_ = 0;
    Effect* const effect_ = nullptr;

    const ParameterArray* params_ = &ParameterArray::Empty;

    bool hasCustomParams_ = false;
    ParameterArray customParams_;

    const ParameterArray& internalParams_;

    Vector<std::pair<const Texture*, unsigned int /* frame */>> textureAnimations_;

    RenderQueueItemArray items_;

    mutable unsigned int sortKey_ = 0;
};

}
