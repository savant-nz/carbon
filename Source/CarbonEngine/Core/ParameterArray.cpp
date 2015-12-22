/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Core/Parameter.h"
#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Core/Threads/Thread.h"

namespace Carbon
{

const ParameterArray ParameterArray::Empty;

static std::unordered_map<String, unsigned int>* parameterLookups = nullptr;

static struct CleanupParameterLookups
{
    ~CleanupParameterLookups()
    {
        delete parameterLookups;
        parameterLookups = nullptr;
    }
} cleanUpParameterLookups;

ParameterArray::Lookup::Lookup(const String& name)
{
    assert(Thread::isRunningInMainThread());

    if (!parameterLookups)
        parameterLookups = new std::unordered_map<String, unsigned int>;

    auto i = parameterLookups->find(name);
    if (i != parameterLookups->end())
        index_ = i->second;
    else
    {
        static auto nextLookupIndex = 0U;

        index_ = nextLookupIndex;
        (*parameterLookups)[name] = index_;

        nextLookupIndex++;

        if (!Parameter::isValidParameterName(name))
            Globals::debugLog("The parameter name '%s' is invalid", name.cStr());
    }
}

const String& ParameterArray::Lookup::getName() const
{
    assert(Thread::isRunningInMainThread());

    for (auto& parameterLookup : *parameterLookups)
    {
        if (parameterLookup.second == index_)
            return parameterLookup.first;
    }

    // Reaching here should be impossbile
    LOG_ERROR << "Failed finding parameter lookup name";
    assert(false && "Failed finding parameter lookup name");

    return String::Empty;
}

ParameterArray::ParameterArray(const ParameterArray& other) : size_(0)
{
    for (auto i = 0U; i < other.entries_.size(); i++)
    {
        if (other.entries_[i])
            set(Lookup(i), *other.entries_[i]);
    }
}

void swap(ParameterArray& first, ParameterArray& second)
{
    using std::swap;

    swap(first.entries_, second.entries_);
    swap(first.size_, second.size_);
}

void ParameterArray::clear()
{
    for (auto entry : entries_)
        delete entry;

    entries_.clear();
    size_ = 0;
}

Parameter& ParameterArray::operator[](const Lookup& lookup)
{
    if (entries_.size() <= lookup)
        entries_.resize(lookup + 1, nullptr);

    if (entries_[lookup])
        return *entries_[lookup];

    size_++;

    return *(entries_[lookup] = new Parameter);
}

const Parameter& ParameterArray::get(const Lookup& lookup) const
{
    return get(lookup, Parameter::Empty);
}

const Parameter& ParameterArray::get(const Lookup& lookup, const Parameter& fallback) const
{
    if (lookup < entries_.size() && entries_[lookup])
        return *entries_[lookup];

    return fallback;
}

const Parameter& ParameterArray::get(const String& name, const Parameter& fallback) const
{
    return get(Lookup(name), fallback);
}

void ParameterArray::set(const Lookup& lookup, const Parameter& value)
{
    operator[](lookup) = value;
}

void ParameterArray::set(const String& name, const Parameter& value)
{
    if (!Parameter::isValidParameterName(name))
        return;

    operator[](name) = value;
}

bool ParameterArray::remove(const Lookup& lookup)
{
    if (lookup >= entries_.size() || !entries_[lookup])
        return false;

    delete entries_[lookup];
    entries_[lookup] = nullptr;

    assert(size_);
    size_--;

    if (size_ == 0)
        entries_.clear();

    return true;
}

void ParameterArray::merge(const ParameterArray& parameters)
{
    for (auto i = 0U; i < parameters.entries_.size(); i++)
    {
        if (parameters.entries_[i])
            operator[](Lookup(i)) = *parameters.entries_[i];
    }
}

bool ParameterArray::has(const Lookup& lookup) const
{
    return lookup < entries_.size() && entries_[lookup];
}

Vector<String> ParameterArray::getParameterNames() const
{
    auto names = Vector<String>();

    for (auto i = 0U; i < entries_.size(); i++)
    {
        if (entries_[i])
            names.append(Lookup(i).getName());
    }

    return names;
}

void ParameterArray::save(FileWriter& file) const
{
    file.write(size_);

    for (auto i = 0U; i < entries_.size(); i++)
    {
        if (entries_[i])
            file.write(Lookup(i).getName(), *entries_[i]);
    }
}

void ParameterArray::load(FileReader& file)
{
    auto size = 0U;
    file.read(size);

    try
    {
        clear();

        auto parameterName = String();
        for (auto i = 0U; i < size; i++)
        {
            file.read(parameterName);
            file.read(operator[](parameterName));
        }
    }
    catch (const std::bad_alloc&)
    {
        throw Exception("Failed reading parameters");
    }
}

ParameterArray::operator UnicodeString() const
{
    return String(getParameterNames().sorted().map<String>([&](const String& s) { return s + ": " + get(s).getString(); }));
}

}
