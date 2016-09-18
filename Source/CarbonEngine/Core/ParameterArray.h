/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Manages a collection of named Parameter instances in a key => value setup. Lookups can be done by the name string or
 * alternatively the ParameterArray::Lookup helper subclass can be used to speed up parameter lookups to constant time.
 */
class CARBON_API ParameterArray
{
public:

    /**
     * An empty parameter array.
     */
    static const ParameterArray Empty;

    /**
     * A Lookup object is a small class that is used to speed up the process of looking up a Parameter in a
     * ParameterArray. A Lookup object is created to be a lookup of a specified parameter name, e.g. "diffuseColor",
     * which it does by assigning a unique index value to each parameter name. The same Lookup object can be used to
     * look up its parameter in any ParameterArray instance.
     */
    class Lookup
    {
    public:

        /**
         * Constructs this Lookup to be a lookup for the specified parameter name.
         */
        explicit Lookup(const String& name);

        /**
         * Returns the name of the parameter that this Lookup instance actually looks up in a ParameterArray, i.e. the
         * \a name that was passed to its constructor.
         */
        const String& getName() const;

        /**
         * Automatic conversion to unsigned integer that returns the index value to be used for this lookup.
         */
        operator unsigned int() const { return index_; }

    private:

        unsigned int index_ = 0;

        friend class ParameterArray;
        explicit Lookup(unsigned int index) : index_(index) {}
    };

    ParameterArray() {}

    /**
     * Copy constructor.
     */
    ParameterArray(const ParameterArray& other);

    /**
     * Move constructor.
     */
    ParameterArray(ParameterArray&& other) noexcept : entries_(std::move(other.entries_)), size_(other.size_)
    {
        other.size_ = 0;
    }

    ~ParameterArray() { clear(); }

    /**
     * Assignment operator.
     */
    ParameterArray& operator=(ParameterArray other)
    {
        swap(*this, other);
        return *this;
    }

    /**
     * Swaps the contents of two ParameterArray instances.
     */
    friend void swap(ParameterArray& first, ParameterArray& second);

    /**
     * Returns the number of parameters stored in this ParameterArray.
     */
    unsigned int size() const { return size_; }

    /**
     * Returns whether this ParameterArray is empty and has no parameters stored in it.
     */
    bool empty() const { return size_ == 0; }

    /**
     * Returns the parameter for the given Lookup. If no such parameter exists then a reference to Parameter::Empty is
     * returned.
     */
    const Parameter& operator[](const Lookup& lookup) const { return get(lookup); }

    /**
     * Returns the parameter with the given name. If no such parameter exists then a reference to Parameter::Empty is
     * returned.
     */
    const Parameter& operator[](const String& name) const { return get(Lookup(name)); }

    /**
     * Returns the parameter for the given lookup, If no such parameter exists then it will be created.
     */
    Parameter& operator[](const Lookup& lookup);

    /**
     * Returns the parameter with the given name. If no such parameter exists then it will be created.
     */
    Parameter& operator[](const String& name) { return operator[](Lookup(name)); }

    /**
     * Returns the parameter for the given Lookup. If no such parameter exists then Parameter::Empty is returned.
     */
    const Parameter& get(const Lookup& lookup) const;

    /**
     * Returns the parameter for the given Lookup. If no such parameter exists then the given fallback value is
     * returned.
     */
    const Parameter& get(const Lookup& lookup, const Parameter& fallback) const;

    /**
     * Returns the parameter with the given name. If no such parameter exists then the given fallback value is returned.
     */
    const Parameter& get(const String& name, const Parameter& fallback) const;

    /**
     * Returns the parameter with the given name. If no such parameter exists then Parameter::Empty is returned.
     */
    const Parameter& get(const String& name) const { return get(Lookup(name)); }

    /**
     * Sets the value of the specified parameter.
     */
    void set(const Lookup& lookup, const Parameter& value);

    /**
     * Sets the value of the specified parameter.
     */
    void set(const String& name, const Parameter& value);

    /**
     * Removes the parameter for the given Lookup. Returns success flag.
     */
    bool remove(const Lookup& lookup);

    /**
     * Removes the parameter with the given name. Returns success flag.
     */
    bool remove(const String& name) { return remove(Lookup(name)); }

    /**
     * Clears all the stored parameters.
     */
    void clear();

    /**
     * Takes a second ParameterArray class and merges its contents into this one. If there is a Parameter with the same
     * name in both this ParameterArray and the passed ParameterArray then the one passed in overwrites the one
     * currently stored in this ParameterArray.
     */
    void merge(const ParameterArray& parameters);

    /**
     * Returns whether there is a parameter for the given lookup in this parameter array.
     */
    bool has(const Lookup& lookup) const;

    /**
     * Returns whether there is a parameter with the given name in this parameter array.
     */
    bool has(const String& name) const { return has(Lookup(name)); }

    /**
     * Returns a vector of the names of all parameters in this parameter array.
     */
    Vector<String> getParameterNames() const;

    /**
     * Saves this parameter array to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const;

    /**
     * Loads this parameter array from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file);

    /**
     * Returns this parameter array as a comma separated 'key: value, key: value' string.
     */
    operator UnicodeString() const;

    /**
     * Forward iterator for ParameterArray.
     */
    class ForwardIterator
    {
    public:

#ifndef DOXYGEN
        class ValueType
        {
        public:

            const Lookup& getLookup() const { return lookup_; }
            const String& getName() const { return lookup_.getName(); }
            const Parameter& getValue() const { return value_; }

        private:

            Lookup lookup_;
            const Parameter& value_;

            ValueType(unsigned int index, const Parameter& value) : lookup_(index), value_(value) {}

            friend class ForwardIterator;
        };

        bool operator==(const ForwardIterator& other) const
        {
            return &entries_ == &other.entries_ && position_ == other.position_;
        }

        bool operator!=(const ForwardIterator& other) const
        {
            return &entries_ != &other.entries_ || position_ != other.position_;
        }

        ValueType operator*() const { return {position_, *entries_[position_]}; }

        ForwardIterator& operator++()
        {
            position_++;
            advanceToNextParameter();

            return *this;
        }

        // Typedefs to satisfy std::iterator_traits
        typedef std::forward_iterator_tag iterator_category;
        typedef ValueType value_type;
        typedef void difference_type;
        typedef ValueType* pointer;
        typedef ValueType& reference;
#endif

    private:

        const Vector<Parameter*>& entries_;
        unsigned int position_ = 0;

        ForwardIterator(const Vector<Parameter*>& entries, unsigned int position)
            : entries_(entries), position_(position)
        {
            advanceToNextParameter();
        }

        void advanceToNextParameter()
        {
            while (position_ < entries_.size() && !entries_[position_])
                position_++;
        }

        friend class ParameterArray;
    };

    /**
     * Returns a forward iterator located at the beginning of this parameter array.
     */
    ForwardIterator begin() const { return {entries_, 0}; }

    /**
     * Returns a forward iterator located at the end of this parameter array.
     */
    ForwardIterator end() const { return {entries_, entries_.size()}; }

private:

    Vector<Parameter*> entries_;

    unsigned int size_ = 0;
};

}

#include "CarbonEngine/Core/Parameter.h"
