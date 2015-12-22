/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/RandomNumberGenerator.h"

namespace Carbon
{

/**
 * Vector class for managing a dynamic array.
 */
template <typename T> class Vector
{
public:

    Vector() {}

    /**
     * Constructor that sets the initial size of the vector and sets each item to the given value.
     */
    explicit Vector(unsigned int initialSize, const T& initialValue = T()) { resize(initialSize, initialValue); }

    /**
     * Copy constructor.
     */
    Vector(const Vector<T>& other)
    {
        reserve(other.size());

        for (auto& item : other)
            append(item);
    }

    /**
     * Move constructor.
     */
    Vector(Vector<T>&& other) noexcept : size_(other.size_), capacity_(other.capacity_), data_(other.data_)
    {
        other.size_ = 0;
        other.capacity_ = 0;
        other.data_ = nullptr;
    }

    /**
     * Initializer list constructor.
     */
    Vector(const std::initializer_list<T>& initializerList) : Vector()
    {
        reserve(uint(initializerList.size()));

        for (auto& item : initializerList)
            append(item);
    }

    /**
     * Constructs this vector from the contents of the passed `std::array` instance.
     */
    template <size_t N> Vector(const std::array<T, N>& array)
    {
        reserve(uint(array.size()));

        for (auto& item : array)
            append(item);
    }

    ~Vector() { clear(); }

    /**
     * Assignment operator.
     */
    Vector<T>& operator=(Vector<T> other)
    {
        swap(*this, other);
        return *this;
    }

    /**
     * Swaps the contents of two Vector instances.
     */
    friend void swap(Vector<T>& first, Vector<T>& second) noexcept
    {
        std::swap(first.size_, second.size_);
        std::swap(first.capacity_, second.capacity_);
        std::swap(first.data_, second.data_);
    }

    /**
     * Equality operator.
     */
    bool operator==(const Vector<T>& other) const
    {
        if (size() != other.size())
            return false;

        for (auto i = 0U; i < size(); i++)
        {
            if (!(data_[i] == other[i]))
                return false;
        }

        return true;
    }

    /**
     * Inequality operator.
     */
    bool operator!=(const Vector<T>& other) const { return !operator==(other); }

    /**
     * Returns the number of items in this vector.
     */
    unsigned int size() const { return size_; }

    /**
     * Returns whether this vector is empty, i.e. it contains no items.
     */
    bool empty() const { return size() == 0; }

    /**
     * Returns an iterator located at the start of this vector.
     */
    T* begin() { return data_; }

    /**
     * Returns a const iterator located at the start of this vector.
     */
    const T* begin() const { return data_; }

    /**
     * Returns an iterator located at the end of this vector.
     */
    T* end() { return data_ + size_; }

    /**
     * Returns a const iterator located at the end of this vector.
     */
    const T* end() const { return data_ + size_; }

    /**
     * Appends a new item to this vector.
     */
    void append(const T& value) { resize(size() + 1, value); }

    /**
     * Appends the passed vector to this vector.
     */
    void append(const Vector<T>& v)
    {
        reserve(size() + v.size());

        for (auto& item : v)
            append(item);
    }

    /**
     * Appends a new item to this vector directly constructed from the passed arguments.
     */
    template <typename... ArgTypes> void emplace(ArgTypes&&... args)
    {
        reserve(size() + 1);

        placement_new<T>(&data_[size()], std::forward<ArgTypes>(args)...);

        size_++;
    }

    /**
     * Prepends the passed item onto this vector.
     */
    void prepend(const T& value) { insert(0, value); }

    /**
     * Resizes this vector, if the size is increased then the value to initialize new items to is given by \a newValue. If the
     * vector size is being increased and an internal allocation fails then `std::bad_alloc` is thrown. If the vector size is
     * being reduced then this method will never fail.
     */
    void resize(unsigned int newSize, const T& newValue = T())
    {
        if (newSize > capacity_)
        {
            // The new size can't be accommodated, need a new allocation

            auto newVector = Vector<T>();
            newVector.reserve(std::max(newSize, capacity_ * 2));

            // Copy items into the new storage
            for (const auto& item : *this)
                newVector.append(item);

            // Run constructors for the new items
            for (auto i = size(); i < newSize; i++)
                newVector.append(newValue);

            swap(newVector, *this);
        }
        else if (newSize < size())
        {
            // Run destructors for the items being cut off the end
            for (auto i = newSize; i < size(); i++)
                data_[i].~T();

            size_ = newSize;

            // If the allocated size is enough for more than twice the number of items in this vector then reduce it to reclaim
            // memory. Space for at least 16 items is kept to reduce reallocations in smaller vectors.
            if (size() < capacity_ / 2 && size() > 16)
            {
                try
                {
                    auto newVector = Vector<T>();
                    newVector.reserve(capacity_ / 2);

                    for (const auto& item : *this)
                        newVector.append(item);

                    swap(newVector, *this);
                }
                catch (...)
                {
                    // Something went wrong while reducing this vector's memory usage so stick with the current allocation
                    // rather than moving to a smaller one.
                }
            }
        }
        else
        {
            // No need for any allocation changes, just run constructors for the new items
            for (auto i = size(); i < newSize; i++)
                placement_new<T>(&data_[i], newValue);

            size_ = newSize;
        }
    }

    /**
     * Increases the number of items in this vector by the specified amount.
     */
    void enlarge(unsigned int amount, const T& newValue = T()) { resize(size() + amount, newValue); }

    /**
     * Ensures that this vector has allocated space internally for at least the given number of items. Throws an exception if
     * an error occurs.
     */
    void reserve(unsigned int newCapacity)
    {
        if (newCapacity <= capacity_)
            return;

        auto newVector = Vector<T>();

        newVector.size_ = size();
        newVector.capacity_ = newCapacity;

#ifdef CARBON_INCLUDE_MEMORY_INTERCEPTOR
        MemoryInterceptor::start(__FILE__, __LINE__);
#endif

        newVector.data_ = MemoryInterceptor::allocate<T>(newCapacity);

        // Use move constructors if they are non-throwing, otherwise fall back to copy constructors that potentially throw
        if (std::is_nothrow_move_constructible<T>::value)
        {
            for (auto i = 0U; i < size(); i++)
                placement_new<T>(&newVector.at(i), std::move(at(i)));
        }
        else
        {
            auto i = 0;
            try
            {
                for (i = 0; i < int(size()); i++)
                    placement_new<T>(&newVector.at(i), at(i));
            }
            catch (...)
            {
                // Destruct any items that were successfully constructed and then pass the exception on
                while (--i >= 0)
                    newVector.at(i).~T();

                MemoryInterceptor::free(newVector.data_);
                newVector.data_ = nullptr;

                throw;
            }
        }

        swap(newVector, *this);
    }

    /**
     * Inserts an item into this vector at the given index.
     */
    void insert(unsigned int index, const T& value)
    {
        assert(index <= size() && "Index out of range");

        // Inserting at the end of the vector is just an append
        if (index == size())
        {
            append(value);
            return;
        }

        // Ensure there is space for the new item
        reserve(size() + 1);

        // Move everything after the insert position along one place
        placement_new<T>(data_ + size(), std::move(back()));
        for (auto i = int(size()) - 1; i > int(index); i--)
            at(i) = std::move(at(i - 1));

        // Assign value to the new item
        at(index) = value;

        size_++;
    }

    /**
     * Inserts the contents of the passed vector into this vector at the given index.
     */
    void insert(unsigned int index, const Vector<T>& vector)
    {
        for (auto& item : vector)
            insert(index++, item);
    }

    /**
     * Removes the first item from this vector and returns it.
     */
    T popFront()
    {
        assert(size() && "Vector is empty");

        auto value = std::move(data_[0]);
        erase(0);

        return value;
    }

    /**
     * Removes the last item from this vector and returns it.
     */
    T popBack()
    {
        assert(size() && "Vector is empty");

        auto result = std::move(back());
        resize(size() - 1);

        return result;
    }

    /**
     * Returns the number of items in this vector that return true from the given predicate.
     */
    unsigned int count(const std::function<bool(const T& item)>& predicate) const
    {
        auto result = 0U;

        for (auto& item : *this)
        {
            if (predicate(item))
                result++;
        }

        return result;
    }

    /**
     * Maps the contents of this vector to a new vector of the same size using the specified callback function.
     */
    template <typename ResultElementType>
    Vector<ResultElementType> map(const std::function<ResultElementType(const T& item)>& callback) const
    {
        auto result = Vector<ResultElementType>();

        result.reserve(size());

        for (auto& item : *this)
            result.append(callback(item));

        return result;
    }

    /**
     * Maps the contents of this vector to a new vector of the same size using automatic conversion to the specified \a
     * ResultElementType.
     */
    template <typename ResultElementType> Vector<ResultElementType> map() const
    {
        auto result = Vector<ResultElementType>();

        result.reserve(size());

        for (auto& item : *this)
            result.append(ResultElementType(item));

        return result;
    }

    /**
     * Calls \a predicate for each item in this vector and returns the first one that the predicate returns true for. If true
     * is never returned then \a fallback is returned.
     */
    const T& detect(const std::function<bool(const T& item)>& predicate, const T& fallback) const
    {
        for (auto& item : *this)
        {
            if (predicate(item))
                return item;
        }

        return fallback;
    }

    /**
     * Returns a new vector containing all the items from this vector that return true from the given predicate.
     */
    Vector<T> select(const std::function<bool(const T& item)>& predicate) const
    {
        auto result = Vector<T>();

        for (auto& item : *this)
        {
            if (predicate(item))
                result.append(item);
        }

        return result;
    }

    /**
     * Calls \a predicate for each item in this vector and returns the first one that the predicate returns true for. If true
     * is never returned then \a fallback is returned.
     */
    T& detect(const std::function<bool(const T& item)>& predicate, T& fallback)
    {
        for (auto& item : *this)
        {
            if (predicate(item))
                return item;
        }

        return fallback;
    }

    /**
     * Returns a copy of the portion of this vector starting at \a index and with the specified length. If \a length is -1 then
     * all items including and following \a index are returned.
     */
    Vector<T> slice(unsigned int index, int length = -1) const
    {
        auto result = Vector<T>();

        if (index < size())
        {
            if (length < 0 || length > int(size() - index))
                result.resize(size() - index);
            else
                result.resize(length);

            for (auto& item : result)
                item = at(index++);
        }

        return result;
    }

    /**
     * Returns the item at the given index in this vector.
     */
    const T& operator[](unsigned int index) const { return at(index); }

    /**
     * Returns the item at the given index in this vector.
     */
    T& operator[](unsigned int index) { return at(index); }

    /**
     * Returns the item at the given index in this vector.
     */
    const T& at(unsigned int index) const
    {
        assert(index < size() && "Index out of range");

        return data_[index];
    }

    /**
     * Returns the item at the given index in this vector.
     */
    T& at(unsigned int index)
    {
        assert(index < size() && "Index out of range");

        return data_[index];
    }

    /**
     * Clears all items out of this vector and frees all memory being used.
     */
    void clear()
    {
        // Call destructors
        for (auto i = 0U; i < size(); i++)
            data_[i].~T();

        size_ = 0;
        capacity_ = 0;

        MemoryInterceptor::free(data_);
        data_ = nullptr;
    }

    /**
     * Returns the last item in this vector.
     */
    const T& back() const
    {
        assert(size() && "Vector is empty");

        return data_[size() - 1];
    }

    /**
     * Returns the last item in this vector.
     */
    T& back()
    {
        assert(size() && "Vector is empty");

        return data_[size() - 1];
    }

    /**
     * Removes the item at the given index.
     */
    void erase(unsigned int index)
    {
        assert(index < size() && "Index out of range");

        for (; index < size() - 1; index++)
            data_[index] = std::move(data_[index + 1]);

        resize(size() - 1);
    }

    /**
     * Overwrites the item at the specified index with the item at the end of the vector and then decreases the size of the
     * vector by one. This is faster than Vector::erase() but can only be used when the ordering of items is not important.
     */
    void unorderedErase(unsigned int index)
    {
        assert(index < size() && "Index out of range");

        if (index != size() - 1)
            data_[index] = std::move(back());

        resize(size() - 1);
    }

    /**
     * Removes the first item in this vector that has the given value, the return value indicates whether an item was removed
     * from this vector.
     */
    bool eraseValue(const T& value)
    {
        auto index = find(value);
        if (index == -1)
            return false;

        erase(index);

        return true;
    }

    /**
     * Removes the first item in this vector that has the given value using Vector::unorderedErase() method, the return value
     * indicates whether an item was removed from this vector.
     */
    bool unorderedEraseValue(const T& value)
    {
        auto index = find(value);
        if (index == -1)
            return false;

        unorderedErase(index);

        return true;
    }

    /**
     * Removes all the items in this vector that return true from the given predicate. Returns the number of items that were
     * removed.
     */
    unsigned int eraseIf(const std::function<bool(const T& item)>& predicate)
    {
        auto removedCount = 0U;

        for (auto i = 0U; i < size(); i++)
        {
            if (predicate(data_[i]))
            {
                erase(i--);
                removedCount++;
            }
        }

        return removedCount;
    }

    /**
     * Returns the index of the first item in this vector that returns true from the callback function, otherwise returns -1.
     */
    int findBy(const std::function<bool(const T& item)>& predicate) const
    {
        for (auto i = 0U; i < size(); i++)
        {
            if (predicate(data_[i]))
                return int(i);
        }

        return -1;
    }

    /**
     * Searches for an item and returns the index of the first match found. Returns -1 if no items match.
     */
    template <typename CompareType = T> int find(const CompareType& value) const
    {
        for (auto i = 0U; i < size(); i++)
        {
            if (data_[i] == value)
                return int(i);
        }

        return -1;
    }

    /**
     * Returns whether or not the given item is present in this vector.
     */
    bool has(const T& value) const { return find(value) != -1; }

    /**
     * Returns whether or not the given item is present in this vector.
     */
    bool has(const std::function<bool(const T& item)>& predicate) const
    {
        for (auto& item : *this)
        {
            if (predicate(item))
                return true;
        }

        return false;
    }

#ifdef _MSC_VER
    // When T can be constructed from const char* the Visual Studio 2015 compiler can't decide between the two has() overloads
    // above. This appears to be a bug in that toolchain where it matches one of the std::function() constructors more liberally
    // than the C++11 specification allows. The following extra has() overload works around the issue.
    bool has(const char* value) const { return has(T(value)); }
#endif

    /**
     * Replaces all instances of the specified \a value in this vector with the specified \a replacement, the return value is
     * the number of replacements that were made.
     */
    unsigned int replace(const T& value, const T& replacement)
    {
        auto replacementCount = 0U;

        for (auto& item : *this)
        {
            if (item == value)
            {
                item = replacement;
                replacementCount++;
            }
        }

        return replacementCount;
    }

    /**
     * Sorts the items in this vector from least to greatest as determined by the specified comparison function. If no
     * comparison function is specified then a simple less-than comparison is used.
     */
    void sortBy(const std::function<bool(const T& first, const T& second)>& predicate = std::less<T>())
    {
        std::sort(data_, data_ + size(), predicate);
    }

    /**
     * Sorts the items in this vector from least to greatest as determined using a less-than comparison.
     */
    void sort() { sortBy(std::less<T>()); }

    /**
     * Returns a copy of this vector with the items sorted according to the specified binary predicate.
     */
    Vector<T> sorted(const std::function<bool(const T& first, const T& second)>& predicate = std::less<T>()) const
    {
        auto result = *this;
        result.sortBy(predicate);
        return result;
    }

    /**
     * Assumes this vector is sorted in ascending order and does a binary search on it for an item value. Returns the index if
     * found, or a negative value if not found. If the item is not found the return value indicates where the item should be
     * inserted into the vector in order to maintain the right ordering, get this index by negating the return value and
     * subtracting one.
     */
    template <typename CompareType = T> int binarySearch(const CompareType& value) const
    {
        return binarySearch<CompareType>(value, [&](const T& item) { return item; });
    }

    /**
     * Does a binary search the same as Vector::binarySearch() except item values used in the search are found by calling the
     * specified evaluator function that returns the value to compare for that item in the binary search.
     */
    template <typename CompareType>
    int binarySearch(const CompareType& value, const std::function<CompareType(const T& item)>& fnEvaluate) const
    {
        if (empty())
            return -1;

        if (value < fnEvaluate(at(0)))
            return -1;

        if (value > fnEvaluate(back()))
            return -int(size() + 1);

        auto low = 0;
        auto high = int(size() - 1);

        while (low <= high)
        {
            auto mid = (low + high) / 2;

            if (fnEvaluate(at(mid)) > value)
                high = mid - 1;
            else if (fnEvaluate(at(mid)) < value)
                low = mid + 1;
            else
                return mid;
        }

        return -(low + 1);
    }

    /**
     * Returns a random item in this vector.
     */
    const T& random() const
    {
        assert(size() && "Vector is empty");

        return data_[RandomNumberGenerator::run() % size()];
    }

    /**
     * Returns the average of all the items in this vector, this requires that the template type implement the division by float
     * and in-place addition operators.
     */
    T getAverage() const
    {
        auto result = T();

        if (empty())
            return result;

        for (auto& item : *this)
            result += item;

        return result / float(size());
    }

    /**
     * Reverses the contents of this vector.
     */
    void reverse()
    {
        using std::swap;

        auto halfSize = size() / 2;

        for (auto i = 0U; i < halfSize; i++)
            swap(data_[i], data_[size() - i - 1]);
    }

    /**
     * Returns the number of bytes that are currently allocated by this vector.
     */
    unsigned int getMemoryUsage() const { return capacity_ * sizeof(T); }

    /**
     * Forcibly casts this vector's raw data to another type.
     */
    template <typename TargetType> const TargetType* as() const { return reinterpret_cast<const TargetType*>(data_); }

    /**
     * Forcibly casts this vector's raw data to another type.
     */
    template <typename TargetType> TargetType* as() { return reinterpret_cast<TargetType*>(data_); }

    /**
     * Returns this vector's internal data pointer.
     */
    const T* getData() const { return data_; }

    /**
     * Returns this vector's internal data pointer.
     */
    T* getData() { return data_; }

    /**
     * Returns the number of bytes of data currently stored in this vector, the total amount of allocated memory may be larger
     * than this value.
     */
    unsigned int getDataSize() const { return size() * sizeof(T); }

private:

    unsigned int size_ = 0;
    unsigned int capacity_ = 0;

    T* data_ = nullptr;
};

}
