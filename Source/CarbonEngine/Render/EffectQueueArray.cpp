/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Render/Effect.h"
#include "CarbonEngine/Render/EffectQueue.h"
#include "CarbonEngine/Render/EffectQueueArray.h"

namespace Carbon
{

void EffectQueueArray::clear()
{
    for (auto queue : queues_)
        delete queue;

    queues_.clear();
}

EffectQueue* EffectQueueArray::create(int priority, Effect* effect, const ParameterArray& internalParams)
{
    auto index = queues_.binarySearch<int>(priority, [](const EffectQueue* queue) { return queue->getPriority(); });
    if (index < 0)
        index = -index - 1;

    queues_.insert(index, new EffectQueue(priority, effect, internalParams));

    return queues_[index];
}

void EffectQueueArray::debugTrace() const
{
    for (auto i = 0U; i < queues_.size(); i++)
    {
        auto queue = queues_[i];

        LOG_DEBUG << "Primary render queue " << (i + 1) << "/" << queues_.size()
                  << ", priority: " << queue->getPriority()
                  << ", effect: " << (queue->getEffect() ? queue->getEffect()->getName() : "null")
                  << ", items: " << queue->getItems().size();

        if (queue->getParams().size())
            LOG_DEBUG << "    " << queue->getParams();
        if (queue->getInternalParams().size())
            LOG_DEBUG << "    (internal) " << queue->getInternalParams();

        queue->getItems().debugTrace();
    }
}

}
