/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/HashFunctions.h"

namespace Carbon
{

/**
 * This container holds a set of unique pointers with no defined order. Internally a hash table is used for performance.
 */
template <typename T> class UnorderedPointerSet
{
public:

    UnorderedPointerSet() { clear(); }

    /**
     * Returns the number of entries in this container.
     */
    unsigned int size() const { return size_; }

    /**
     * Returns whether or not there are any entries in this container.
     */
    bool empty() const { return size_ == 0; }

    /**
     * Removes all entries from this container.
     */
    void clear()
    {
        data_.clear();
        data_.resize(InitialHashTableSize);
        size_ = 0;
    }

    /**
     * Adds a entry to this container, duplicates are not checked for and null entries should not be added.
     */
    void insert(T* entry)
    {
        assert(entry && "Attempted to put a null entry into an UnorderedPointerSet");

        hash(entry).append(entry);
        size_++;

        // If this container is too densely populated then double the size of the hash table
        if (size_ > data_.size() * AutoExpandDensity)
            growHashTable();
    }

    /**
     * Removes an entry from this container, the return value indicates whether or not the entry was present in this
     * container and thus able to be erased.
     */
    bool erase(T* entry)
    {
        if (!hash(entry).unorderedEraseValue(entry))
            return false;

        assert(size_);
        size_--;

        return true;
    }

    /**
     * Returns whether or not the passed entry is currently in this container.
     */
    bool has(T* entry) const { return hash(entry).has(entry); }

    /**
     * Forward iterator for UnorderedPointerSet.
     */
    template <typename EntryType> class Iterator
    {
    public:

#ifndef DOXYGEN
        // Typedefs to satisfy std::iterator_traits
        typedef std::forward_iterator_tag iterator_category;
        typedef EntryType* value_type;
        typedef void difference_type;
        typedef EntryType** pointer;
        typedef EntryType*& reference;

        bool operator==(const Iterator& other) const
        {
            return data_ == other.data_ && hashLine_ == other.hashLine_ && hashLineEntry_ == other.hashLineEntry_;
        }

        bool operator!=(const Iterator& other) const
        {
            return data_ != other.data_ || hashLine_ != other.hashLine_ || hashLineEntry_ != other.hashLineEntry_;
        }

        EntryType* operator*() { return (*data_)[hashLine_][hashLineEntry_]; }

        Iterator<EntryType>& operator++()
        {
            hashLineEntry_++;

            if (hashLineEntry_ >= (*data_)[hashLine_].size())
            {
                hashLine_++;
                hashLineEntry_ = 0;

                advanceToNonEmptyHashLine();
            }

            return *this;
        }
#endif

    private:

        const Vector<Vector<T*>>* data_ = nullptr;

        unsigned int hashLine_ = 0;
        unsigned int hashLineEntry_ = 0;

        Iterator(const Vector<Vector<T*>>& data, unsigned int hashLine) : data_(&data), hashLine_(hashLine)
        {
            advanceToNonEmptyHashLine();
        }

        void advanceToNonEmptyHashLine()
        {
            while (hashLine_ < data_->size() && (*data_)[hashLine_].empty())
                hashLine_++;
        }

        friend class UnorderedPointerSet;
    };

    /**
     * Returns a begin iterator.
     */
    Iterator<T> begin() { return Iterator<T>(data_, 0); }

    /**
     * Returns an end iterator.
     */
    Iterator<T> end() { return Iterator<T>(data_, data_.size()); }

    /**
     * Returns a begin const iterator.
     */
    Iterator<const T> begin() const { return Iterator<const T>(data_, 0); }

    /**
     * Returns an end const iterator.
     */
    Iterator<const T> end() const { return Iterator<const T>(data_, data_.size()); }

private:

    static const auto InitialHashTableSize = 256U;
    static const auto AutoExpandDensity = 25U;

    unsigned int size_ = 0;
    Vector<Vector<T*>> data_;

    unsigned int hashLine(const T* entry) const { return HashFunctions::hash(entry) & (data_.size() - 1); }

    Vector<T*>& hash(T* entry) { return data_[hashLine(entry)]; }
    const Vector<T*>& hash(const T* entry) const { return data_[hashLine(entry)]; }

    void growHashTable()
    {
        // Get a list of all current entries
        auto entries = Vector<T*>();
        entries.reserve(size());
        for (auto entry : *this)
            entries.append(entry);

        // Clear current hash table
        size_ = 0;
        for (auto& hashLine : data_)
            hashLine.clear();

        // Double the size of the hash table
        data_.resize(data_.size() * 2);

        // Add all the entries back into the new larger hash table
        for (auto entry : entries)
            insert(entry);
    }
};

}
