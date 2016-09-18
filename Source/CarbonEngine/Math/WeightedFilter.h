/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Template class that calculates a weighted average of values.
 */
template <typename T, unsigned int BufferSize> class CARBON_API WeightedFilter
{
public:

    /**
     * Controls how the filter averages the data. A value of one gives all values equal weight, a value below one gives
     * more importance to the more recent values the closer it gets to zero.
     */
    float weightModifier = 1.0f;

    WeightedFilter() : buffer_() {}

    /**
     * Sets all the values in the filter at once.
     */
    void setAll(const T& t)
    {
        for (auto& item : buffer_)
            item = t;
    }

    /**
     * Adds a new value to the front of the history buffer.
     */
    void add(const T& t)
    {
        for (auto i = buffer_.size() - 1; i > 0; i--)
            buffer_[i] = buffer_[i - 1];

        buffer_[0] = t;
    }

    /**
     * Calculates the weighted average of the values in the history buffer using the weight modifier.
     */
    T calculateWeightedAverage() const
    {
        auto average = T(0.0f);
        auto totalWeight = 0.0f;
        auto currentWeight = 1.0f;

        for (auto& item : buffer_)
        {
            average += item * currentWeight;
            totalWeight += currentWeight;
            currentWeight *= weightModifier;
        }

        return average / totalWeight;
    }

private:

    std::array<T, BufferSize> buffer_ = {};
};

}
