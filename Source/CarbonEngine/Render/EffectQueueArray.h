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
 * This class is what scenes populate when they are queried for visible geometry by the renderer during rendering, and
 * it just maintains a list of EffectQueue instances ordered by priority, see the EffectQueue class for details of the
 * structure of each queue.
 */
class EffectQueueArray : private Noncopyable
{
public:

    EffectQueueArray() {}

    /**
     * Copy constructor.
     */
    EffectQueueArray(const EffectQueueArray& other);

    ~EffectQueueArray() { clear(); }

    /**
     * Clears this effect queue array, all queues it holds are destructed.
     */
    void clear();

    /**
     * Creates a new effect queue with the given priority, effect and internal parameters. Returns the new effect queue.
     */
    EffectQueue* create(int priority, Effect* effect, const ParameterArray& internalParams = ParameterArray::Empty);

    /**
     * Returns the vector of effect queues that have been created through EffectQueueArray::create(), this list is
     * sorted by priority.
     */
    Vector<EffectQueue*>& getQueues() { return queues_; }

    /**
     * Returns the number of queues in this effect queue array.
     */
    unsigned int size() const { return queues_.size(); }

    /**
     * Returns the queue at the specified index.
     */
    EffectQueue* operator[](unsigned int index) { return queues_[index]; }

    /**
     * Returns the queue at the specified index.
     */
    const EffectQueue* operator[](unsigned int index) const { return queues_[index]; }

    /**
     * Prints this scene render queues to the main logfile.
     */
    void debugTrace() const;

private:

    Vector<EffectQueue*> queues_;
};

}
