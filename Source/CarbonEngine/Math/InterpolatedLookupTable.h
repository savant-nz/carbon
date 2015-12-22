/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * The interpolated lookup table class allows [index, value] pairs to be added and then quickly retrieved, index values for
 * which no specific data exists will have their value calculated based on interpolation of the closest data points that are
 * present.
 */
template <typename IndexType, typename ValueType> class InterpolatedLookupTable
{
public:

    /**
     * Typedef for a function that interpolates between two values.
     */
    typedef std::function<ValueType(const ValueType&, const ValueType&, float t)> InterpolateFunctionType;

    /**
     * Removes all entries from this interpolated lookup table.
     */
    void clear() { entries_.clear(); }

    /**
     * Adds an entry into this interpolated lookup table, each entry consists of a unique index and a corresponding value. Any
     * index can be looked up with InterpolatedLookupTable::lookup(), and the values of the two closest index values will be
     * interpolated to get the result of the lookup.
     */
    void add(const IndexType& index, const ValueType& value)
    {
        auto entryIndex = entries_.binarySearch(LookupEntry(index));

        if (entryIndex >= 0)
            entries_[entryIndex].second = value;
        else
            entries_.insert(-entryIndex - 1, LookupEntry(index, value));
    }

    /**
     * Looks up a value in this lookup table based on a given index. The two data points added using the
     * InterpolatedLookupTable::add() method that are closest to the given index value will be interpolated appropriately to get
     * the result of the lookup. Lookups are O(log N) where N is the number of data points that have been added. By default
     * linear interpolation will be used, but a different interpolation function can be supplied if needed.
     */
    ValueType lookup(const IndexType& index, InterpolateFunctionType fnInterpolate = Interpolate::linear<ValueType>) const
    {
        if (entries_.empty())
            return {};

        auto entryIndex = entries_.binarySearch(LookupEntry(index, ValueType()));
        if (entryIndex <= 0)
            entryIndex = -entryIndex - 1;

        auto& entry1 = getValue(entryIndex - 1);
        auto& entry2 = getValue(entryIndex);

        if (&entry1 == &entry2)
            return entry1.second;

        auto t = float(index - entry1.first) / float(entry2.first - entry1.first);

        return fnInterpolate(entry1.second, entry2.second, t);
    }

    /**
     * Returns a human-readable string containing all the data points present in this lookup table.
     */
    operator UnicodeString() const
    {
        auto result = entries_.template map<String>(
            [](const LookupEntry& entry) { return String() + "[" + entry.first + " => " + entry.second + "]"; });
        return UnicodeString(result);
    }

private:

    typedef std::pair<IndexType, ValueType> LookupEntry;

    const LookupEntry& getValue(int index) const { return entries_[Math::clamp<int>(index, 0, entries_.size() - 1)]; }

    Vector<LookupEntry> entries_;
};

}
