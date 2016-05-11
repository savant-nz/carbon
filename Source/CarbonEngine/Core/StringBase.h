/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * The base Unicode character type.
 */
typedef char32_t UnicodeCharacter;

/**
 * The StringBase class template implements string handling and is intended to be specialized on the char type for handling of
 * ASCII strings and on the UnicodeCharacter type for UTF-32 strings. There are String and UnicodeString typedefs for these two
 * specializations.
 *
 * The StringBase class template has two ways of storing its data: on the heap or in a local buffer. Use of the local buffer is
 * preferred because it means that no heap allocations need to be done and the string is more cache friendly. However, the local
 * buffer is of limited size and so can only be used when the contained string fits in the available space.
 *
 * The StringBase class template is defined as being 32 bytes in size for ASCII strings and 64 bytes in size for UTF-32 strings.
 * For ASCII strings this results in the local storage buffer being 24 bytes in size on 32-bit systems and 20 bytes on 64-bit
 * systems. This means that ASCII strings with 19 or fewer characters will be locally cached on all systems and those with 23 or
 * fewer characters will be locally cached on 32-bit systems but not 64-bit systems.
 *
 * Similarly, UTF-32 strings with 12 or fewer characters will be locally cached on 64-bit systems, and this increases to 13 or
 * fewer characters on 32-bit systems.
 */
template <typename T> class CARBON_API StringBase
{
public:

    StringBase() : storage_(localStorage_.raw)
    {
        static_assert(sizeof(StringBase<T>) == SizeOfStringClass, "String class is not the expected size");

        storage_[0] = 0;
    }

    ~StringBase()
    {
        if (isUsingHeapStorage())
            MemoryInterceptor::free(storage_);
    }

    /**
     * Copy constructor.
     */
    StringBase(const StringBase<T>& other) : StringBase()
    {
        resize(other.length());
        memcpy(storage_, other.cStr(), other.length() * sizeof(T));
    }

    /**
     * Move constructor.
     */
    StringBase(StringBase<T>&& other) noexcept : length_(other.length())
    {
        if (other.isUsingLocalStorage())
        {
            storage_ = localStorage_.raw;
            memcpy(storage_, other.storage_, (other.length() + 1) * sizeof(T));
        }
        else
        {
            storage_ = other.storage_;
            localStorage_.heapAllocationSize = other.localStorage_.heapAllocationSize;
        }

        other.storage_ = other.localStorage_.raw;
        other.storage_[0] = 0;
        other.length_ = 0;
    }

    /**
     * Constructs the string from a null-terminated ASCII C string pointer.
     */
    StringBase(const char* str) : StringBase()
    {
        resize(uint(strlen(str)));

        for (auto i = 0U; i < length(); i++)
            at(i) = T(str[i]);
    }

    /**
     * Constructs the string from a single ASCII character value repeated the specified number of times (default is once).
     */
    explicit StringBase(char value, unsigned int count = 1) : StringBase()
    {
        resize(count);

        if (value < 0)
            value = '?';

        for (auto i = 0U; i < count; i++)
            at(i) = value;
    }

    /**
     * Constructs the string from a boolean, either 'true' or 'false'.
     */
    StringBase(bool value) : StringBase() { *this = value ? "true" : "false"; }

    /**
     * Constructs the string from a signed integer.
     */
    StringBase(int32_t value) : StringBase(int64_t(value)) {}

    /**
     * Constructs the string from an unsigned integer.
     */
    StringBase(uint32_t value) : StringBase(uint64_t(value)) {}

    /**
     * Constructs the string from a signed integer.
     */
    StringBase(int64_t value);

    /**
     * Constructs the string from an unsigned integer.
     */
    StringBase(uint64_t value);

#ifdef APPLE
    /**
     * Constructs the string from a size_t.
     */
    StringBase(size_t value) : StringBase(uint64_t(value)) {}
#endif

    /**
     * Constructs the string from a floating point value.
     */
    StringBase(float value) : StringBase(double(value)) {}

    /**
     * Constructs the string from a floating point value.
     */
    StringBase(double value);

#ifdef __OBJC__
    /**
     * On Mac OS X and iOS, constructs this string from the contents of the specified NSString.
     */
    StringBase(const NSString* nsString);
#endif

    /**
     * Constructs the string from a pointer, formatting it as a standard C hexadecimal value that is prefixed with '0x'.
     */
    StringBase(const void* pointer) : StringBase()
    {
        auto hex = std::array<byte_t, sizeof(void*)>();

        for (auto i = 0U; i < sizeof(void*); i++)
        {
            hex[i] = reinterpret_cast<byte_t*>(&pointer)
#ifdef CARBON_LITTLE_ENDIAN
                [sizeof(void*) - i - 1];
#else
                [i];
#endif
        }

        *this = StringBase<T>("0x") + toHex(hex.data(), hex.size(), false);
    }

    /**
     * Constructs this string from the passed vector's items. The default separator is a comma.
     */
    template <typename T2>
    explicit StringBase(const Vector<T2>& v, const StringBase<T>& separator = ", ", unsigned int startIndex = 0) : StringBase()
    {
        for (auto i = startIndex; i < v.size(); i++)
        {
            if (i > startIndex)
                *this << separator;

            *this << v[i];
        }
    }

    /**
     * Assignment operator.
     */
    StringBase<T>& operator=(const StringBase<T>& other)
    {
        if (this != &other)
        {
            resize(other.length());
            memcpy(storage_, other.cStr(), other.length() * sizeof(T));
        }

        return *this;
    }

    /**
     * Move assignment operator.
     */
    StringBase<T>& operator=(StringBase<T>&& other)
    {
        if (this != &other)
        {
            clear();

            if (other.isUsingLocalStorage())
            {
                memcpy(storage_, other.storage_, (other.length() + 1) * sizeof(T));
                length_ = other.length();
            }
            else
            {
                storage_ = other.storage_;
                length_ = other.length();
                localStorage_.heapAllocationSize = other.localStorage_.heapAllocationSize;

                other.storage_ = other.localStorage_.raw;
                other.storage_[0] = 0;
                other.length_ = 0;
            }
        }

        return *this;
    }

    /**
     * Swaps the contents of two StringBase instances.
     */
    friend void swap(StringBase<T>& first, StringBase<T>& second) noexcept
    {
        // If both strings are using their local storage buffers then swap their buffers
        if (first.isUsingLocalStorage() && second.isUsingLocalStorage())
        {
            auto longest = std::max(first.length(), second.length());

            for (auto i = 0U; i <= longest; i++)
                std::swap(first.localStorage_.raw[i], second.localStorage_.raw[i]);
        }

        // If both strings are on the heap then swap their heap allocations
        else if (first.isUsingHeapStorage() && second.isUsingHeapStorage())
        {
            std::swap(first.storage_, second.storage_);
            std::swap(first.localStorage_.heapAllocationSize, second.localStorage_.heapAllocationSize);
        }

        // Handle when one string is using the local storage buffer and the other is on the heap
        else
        {
            auto& localString = first.isUsingLocalStorage() ? first : second;
            auto& heapString = first.isUsingLocalStorage() ? second : first;

            auto heapAllocationSize = heapString.localStorage_.heapAllocationSize;

            for (auto i = 0U; i <= localString.length(); i++)
                heapString.localStorage_.raw[i] = localString.localStorage_.raw[i];

            localString.localStorage_.heapAllocationSize = heapAllocationSize;
            localString.storage_ = heapString.storage_;

            heapString.storage_ = heapString.localStorage_.raw;
        }

        std::swap(first.length_, second.length_);
    }

    /**
     * String equality.
     */
    bool operator==(const StringBase<T>& other) const
    {
        return length() == other.length() && memcmp(cStr(), other.cStr(), sizeof(T) * length()) == 0;
    }

    /**
     * String inequality.
     */
    bool operator!=(const StringBase<T>& other) const
    {
        return length() != other.length() || memcmp(cStr(), other.cStr(), sizeof(T) * length()) != 0;
    }

    /**
     * String concatenation.
     */
    StringBase<T> operator+(const StringBase<T>& other) const
    {
        auto result = StringBase<T>();

        result.resize(length() + other.length());

        memcpy(result.storage_, cStr(), length() * sizeof(T));
        memcpy(result.storage_ + length(), other.cStr(), other.length() * sizeof(T));

        return result;
    }

    /**
     * String concatenation in place.
     */
    StringBase<T>& operator+=(const StringBase<T>& other)
    {
        auto originalLength = length();

        resize(originalLength + other.length());
        memcpy(storage_ + originalLength, other.cStr(), other.length() * sizeof(T));

        return *this;
    }

    /**
     * String comparison.
     */
    bool operator<(const StringBase<T>& other) const
    {
        auto a = cStr();
        auto b = other.cStr();

        while (*a && (*a == *b))
        {
            a++;
            b++;
        }

        return *a < *b;
    }

    /**
     * String comparison.
     */
    bool operator>(const StringBase<T>& other) const
    {
        auto a = cStr();
        auto b = other.cStr();

        while (*a && (*a == *b))
        {
            a++;
            b++;
        }

        return *a > *b;
    }

    /**
     * Returns this string's internal data pointer. Note that because this is a pointer to internal data, it is invalidated when
     * any changes are made to this string instance.
     */
    const T* cStr() const { return storage_; }

    /**
     * Returns whether the specified character occurs in this string, starting at the specified offset. If no starting offset is
     * given then the whole string is searched.
     */
    bool has(T character, unsigned int start = 0) const
    {
        for (auto i = start; i < length(); i++)
        {
            if (at(i) == character)
                return true;
        }

        return false;
    }

    /**
     * Searches this string for the given string starting at the given index. If no index is given then the whole string is
     * searched. Returns -1 if the specified string is not found in this string.
     */
    int find(const StringBase<T>& s, unsigned int start = 0) const
    {
        if (start >= length() || start + s.length() > length())
            return -1;

        auto searchLength = s.length();

        auto sc = s.cStr();

        // The last location to search from
        auto end = cStr() + length() - searchLength;

        // Search through this string for the passed string
        for (auto c = cStr() + start; c <= end; c++)
        {
            auto i = 0U;
            for (i = 0; i < searchLength; i++)
            {
                if (c[i] != sc[i])
                    break;
            }

            if (i == searchLength)
                return int(c - cStr());
        }

        // No match found
        return -1;
    }

    /**
     * Returns the substring of this string starting at the given character.
     */
    StringBase<T> substr(unsigned int start) const { return substr(start, length() - start); }

    /**
     * Returns the substring of this string starting at the given character and of length \a count characters.
     */
    StringBase<T> substr(unsigned int start, unsigned int count) const
    {
        if (start >= length())
            return Empty;

        if (length() - start < count)
            count = length() - start;

        auto result = StringBase<T>(count);
        result.resize(count);

        memcpy(result.storage_, cStr() + start, count * sizeof(T));

        return result;
    }

    /**
     * Returns the length of this string.
     */
    unsigned int length() const { return length_; }

    /**
     * Clears the contents of this string.
     */
    void clear() { resize(0); }

    /**
     * Removes the character at the specified index from this string.
     */
    void erase(unsigned int index)
    {
        for (auto i = index + 1; i < length(); i++)
            at(i - 1) = at(i);

        resize(length() - 1);
    }

    /**
     * Converts this string to lower case, this only works on ASCII characters.
     */
    void toLower()
    {
        for (auto i = 0U; i < length(); i++)
        {
            if (at(i) >= 'A' && at(i) <= 'Z')
                at(i) += 32;
        }
    }

    /**
     * Returns this string in lower case, this only works on ASCII characters.
     */
    StringBase<T> asLower() const
    {
        auto s = *this;
        s.toLower();
        return s;
    }

    /**
     * Converts this string to upper case, this only works on ASCII characters.
     */
    void toUpper()
    {
        for (auto i = 0U; i < length(); i++)
        {
            if (at(i) >= 'a' && at(i) <= 'z')
                at(i) -= 32;
        }
    }

    /**
     * Returns this string in upper case, this only works on ASCII characters.
     */
    StringBase<T> asUpper() const
    {
        auto s = *this;
        s.toUpper();
        return s;
    }

    /**
     * Resizes this string to the specified length. If the string length is increased, the additional characters are given the
     * specified value. If an internal allocation fails then `std::bad_alloc` is thrown. Reducing the size of a string always
     * succeeds.
     */
    void resize(unsigned int newLength, T character = 0)
    {
        if (length() == newLength)
            return;

        if (newLength < LocalStorageSize / sizeof(T))
        {
            // The new length is short enough to use local storage, so switch it from heap storage if that's where the
            // string is currently located

            if (isUsingHeapStorage())
            {
                memcpy(localStorage_.raw, storage_, newLength * sizeof(T));
                MemoryInterceptor::free(storage_);
                storage_ = localStorage_.raw;
            }
        }
        else
        {
            // Need to use heap storage for a string of this length

            auto requiredAllocationSize = newLength + 1;
            auto idealAllocationSize = newLength * 3 / 2 + 1;

            // Work out whether we need to reallocate, if the existing heap storage size is big enough but not much too big then
            // keep it for the time being
            if (isUsingLocalStorage() || localStorage_.heapAllocationSize < requiredAllocationSize ||
                localStorage_.heapAllocationSize > idealAllocationSize)
            {
                try
                {
                    // Allocate and initialize new heap storage
                    auto newStorage = MemoryInterceptor::allocate<T>(idealAllocationSize);
                    memcpy(newStorage, storage_, std::min(length(), newLength) * sizeof(T));

                    // Free any previous heap storage
                    if (isUsingHeapStorage())
                        MemoryInterceptor::free(storage_);

                    storage_ = newStorage;
                    localStorage_.heapAllocationSize = idealAllocationSize;
                }
                catch (const std::bad_alloc&)
                {
                    // If there is no existing heap allocation of sufficient size then re-throw the std::bad_alloc
                    if (isUsingLocalStorage() || localStorage_.heapAllocationSize < newLength + 1)
                        throw;
                }
            }
        }

        // Set values for any new characters
        for (auto i = length(); i < newLength; i++)
            storage_[i] = character;

        // Set the null terminator
        storage_[newLength] = 0;

        length_ = newLength;
    }

    /**
     * Searches this string for the first occurrence of one of the characters in a given string, starting at the offset
     * specified. If no offset is specified then the whole string is searched. Returns the index of the character if found, or
     * -1 if there was no match.
     */
    int findFirstOf(const StringBase<T>& s, unsigned int start = 0) const
    {
        auto c = cStr();
        auto sc = s.cStr();

        for (auto i = start; i < length(); i++)
        {
            for (auto j = 0U; j < s.length(); j++)
            {
                if (c[i] == sc[j])
                    return int(i);
            }
        }

        return -1;
    }

    /**
     * Searches this string for the first occurence of a character that doesn't appear in the given string, starting at the
     * offset specified. Returns the index of the character if found, or -1 if there was no match.
     */
    int findFirstNotOf(const StringBase<T>& s, unsigned int start = 0) const
    {
        auto c = cStr();

        for (auto i = start; i < length(); i++)
        {
            if (s.findFirstOf(c[i]) == -1)
                return int(i);
        }

        return -1;
    }

    /**
     * Searches this string for the last occurrence of one of the characters in a given string. Returns the index of the
     * character if found, or -1 if there was no match.
     */
    int findLastOf(const StringBase<T>& s) const
    {
        if (length() == 0)
            return -1;

        return findLastOf(s, length() - 1);
    }

    /**
     * Searches this string for the last occurrence of one of the characters in a given string up to a given index. Returns the
     * index of the character if found, or -1 if there was no match.
     */
    int findLastOf(const StringBase<T>& s, unsigned int end) const
    {
        assert(end < length() && "Index out of range");

        auto c = cStr();
        auto sc = s.cStr();

        for (auto i = int(end); i >= 0; i--)
        {
            for (auto j = 0U; j < s.length(); j++)
            {
                if (c[i] == sc[j])
                    return int(i);
            }
        }

        return -1;
    }

    /**
     * Returns the character at the specified position in the string.
     */
    T at(unsigned int index) const
    {
        assert(index < length() && "Index out of range");
        return cStr()[index];
    }

    /**
     * Returns the character at the specified position in the string.
     */
    T& at(unsigned int index)
    {
        assert(index < length() && "Index out of range");
        return storage_[index];
    }

    /**
     * Returns the last character in this string, or zero if this string is empty.
     */
    T back() const { return length() ? at(length() - 1) : 0; }

    /**
     * Inserts a string into this string at the specified position.
     */
    void insert(unsigned int index, const StringBase<T>& s) { *this = substr(0, index) + s + substr(index); }

    /**
     * Appends the passed character to this string.
     */
    void append(T c) { resize(length() + 1, c); }

    /**
     * Prepends the passed character to this string.
     */
    bool prepend(T c)
    {
        resize(length() + 1);

        for (auto i = length() - 1; i > 0; i--)
            at(i) = at(i - 1);
        at(0) = c;

        return true;
    }

    /**
     * Returns whether this string contains only numeric characters, i.e. 0-9. Returns true for an empty string. A list of other
     * characters to allow can be specified if desired.
     */
    bool isNumeric(const StringBase<T>& allowed = StringBase<T>::Empty) const
    {
        auto c = cStr();

        for (auto i = 0U; i < length(); i++)
        {
            if ((c[i] < '0' || c[i] > '9') && !allowed.has(c[i]))
                return false;
        }

        return true;
    }

    /**
     * Returns whether this string contains only alphanumeric characters, i.e. a-z and 0-9. Returns true for an empty string. A
     * list of other characters can be specified if desired.
     */
    bool isAlphaNumeric(const StringBase<T>& allowed = StringBase<T>::Empty) const
    {
        auto s = cStr();

        for (auto i = 0U; i < length(); i++)
        {
            if ((s[i] < 'a' || s[i] > 'z') && (s[i] < 'A' || s[i] > 'Z') && (s[i] < '0' || s[i] > '9') && !allowed.has(s[i]))
                return false;
        }

        return true;
    }

    /**
     * Returns whether this string can be sensibly converted to a boolean value with StringBase::asBoolean(). The values that
     * will convert correctly are "true", "false", "yes", "no", "on", "off", "0", and "1".
     */
    bool isBoolean() const
    {
        auto b = asLower();

        return b == "true" || b == "false" || b == "yes" || b == "no" || b == "on" || b == "off" || b == "1" || b == "0";
    }

    /**
     * Converts this string to a boolean value. See StringBase::isBoolean() for details
     */
    bool asBoolean() const
    {
        auto b = asLower();

        return b == "true" || b == "yes" || b == "on" || b == "1";
    }

    /**
     * Returns whether this string can be sensibly converted to an integer value with StringBase::asInteger().
     */
    bool isInteger() const
    {
        if (asInteger() == 0)
        {
            auto s = trimmed();

            return s.length() && s.at(0) == '0';
        }

        return true;
    }

    /**
     * Returns whether this string can be sensibly converted to an integer value with StringBase::asInteger(). This also checks
     * that the integer value is in the given range, inclusive of the two bounding values.
     */
    bool isIntegerInRange(int lower, int upper) const
    {
        if (!isInteger())
            return false;

        auto value = asInteger();

        return value >= lower && value <= upper;
    }

    /**
     * Converts this string to an integer value. See StringBase::isInteger() for details.
     */
    int asInteger() const
    {
        auto s = cStr();

        // Skip whitespace
        while (*s == ' ')
            s++;

        // Read sign
        auto sign = 1;
        if (*s == '+')
        {
            s++;
            sign = 1;
        }
        else if (*s == '-')
        {
            s++;
            sign = -1;
        }

        // Read digits
        auto number = 0;
        while (*s >= '0' && *s <= '9')
            number = number * 10 + (*s++ - '0');

        return sign * number;
    }

    /**
     * Returns whether this string can be sensibly converted to a floating point value with StringBase::asFloat().
     */
    bool isFloat() const
    {
        if (asFloat() == 0.0f)
        {
            auto s = trimmed();
            return s.length() && s.at(0) == '0';
        }

        return true;
    }

    /**
     * Returns whether this string can be sensibly converted to a floating point value with StringBase::asFloat(). This also
     * checks that the floating point value is in the given range, inclusive of the two bounding values.
     */
    bool isFloatInRange(float lower, float upper) const
    {
        if (!isFloat())
            return false;

        auto value = asFloat();

        return value >= lower && value <= upper;
    }

    /**
     * Converts this string to a floating point value.
     */
    float asFloat() const { return float(atof(toASCII().cStr())); }

    /**
     * Replaces every instance of a particular character with another character.
     */
    void replace(T oldChar, T newChar)
    {
        if (!oldChar || !newChar)
            return;

        for (auto i = 0U; i < length(); i++)
            at(i) = (at(i) == oldChar) ? newChar : at(i);
    }

    /**
     * Replaces every instance of a particular substring in this string.
     */
    void replace(const StringBase<T>& oldValue, const StringBase<T>& newValue)
    {
        auto start = 0;

        while (true)
        {
            auto index = find(oldValue, start);
            if (index == -1)
                return;

            for (auto i = 0U; i < oldValue.length(); i++)
                erase(index);

            insert(index, newValue);

            start = index + newValue.length();
        }
    }

    /**
     * Trims characters from the left hand side of this string. Returns the number of characters that were removed.
     */
    unsigned int trimLeft(const StringBase<T>& trimCharacters = TrimCharacters)
    {
        auto c = cStr();

        auto i = 0U;
        for (i = 0; i < length(); i++)
        {
            if (!trimCharacters.has(c[i]))
                break;
        }

        *this = substr(i);

        return i;
    }

    /**
     * Trims characters from the right hand side of this string. Returns the number of characters that were removed.
     */
    unsigned int trimRight(const StringBase<T>& trimCharacters = TrimCharacters, unsigned int start = 0)
    {
        auto originalLength = length();

        auto newLength = length();

        while (true)
        {
            if (newLength <= start || !trimCharacters.has(at(newLength - 1)))
                break;

            newLength--;
        }

        resize(newLength);

        return originalLength - newLength;
    }

    /**
     * Trims characters from both sides of this string using StringBase::trimLeft() and StringBase::trimRight(). Returns the
     * number of characters that were removed.
     */
    unsigned int trim(const StringBase<T>& trimCharacters = TrimCharacters)
    {
        return trimLeft(trimCharacters) + trimRight(trimCharacters);
    }

    /**
     * Returns a copy of this string that has had StringBase::trim() called on it.
     */
    StringBase<T> trimmed(const StringBase<T>& trimCharacters = TrimCharacters) const
    {
        auto s = *this;
        s.trim(trimCharacters);
        return s;
    }

    /**
     * Returns a copy of this string that has had StringBase::trimLeft() called on it.
     */
    StringBase<T> trimmedLeft(const StringBase<T>& trimCharacters = TrimCharacters) const
    {
        auto s = *this;
        s.trimLeft(trimCharacters);
        return s;
    }

    /**
     * Returns a copy of this string that has had StringBase::trimRight() called on it.
     */
    StringBase<T> trimmedRight(const StringBase<T>& trimCharacters = TrimCharacters) const
    {
        auto s = *this;
        s.trimRight(trimCharacters);
        return s;
    }

    /**
     * If this string starts with the given prefix then this method will remove it.
     */
    void removePrefix(const StringBase<T>& prefix)
    {
        if (startsWith(prefix))
            *this = substr(prefix.length());
    }

    /**
     * Returns a copy of this string with the given prefix removed if it is present.
     */
    StringBase<T> withoutPrefix(const StringBase<T>& prefix) const
    {
        auto s = *this;
        s.removePrefix(prefix);
        return s;
    }

    /**
     * If this string ends with the given suffix then this method will remove it.
     */
    void removeSuffix(const StringBase<T>& suffix)
    {
        if (endsWith(suffix))
            *this = substr(0, length() - suffix.length());
    }

    /**
     * Returns a copy of this string with the given suffix removed if it is present.
     */
    StringBase<T> withoutSuffix(const StringBase<T>& suffix) const
    {
        auto s = *this;
        s.removeSuffix(suffix);
        return s;
    }

    /**
     * Splits this string into pieces using the given separators, note that consecutive separators in this string will result in
     * zero-length strings being returned.
     */
    Vector<StringBase<T>> split(const StringBase<T>& separators) const
    {
        auto result = Vector<StringBase<T>>();

        auto start = 0;
        auto len = int(length());

        while (start >= 0 && start < len)
        {
            auto stop = findFirstOf(separators, start);
            if (stop < 0 || stop > len)
                stop = len;

            auto pieceLength = stop - start;
            if (pieceLength > 0)
            {
                // Append a new piece
                result.append(Empty);
                auto& piece = result.back();

                // Resize and copy the piece in-place for speed, this is quite a bit faster than just pushing back
                // 'substr(start, pieceLength)'
                piece.resize(pieceLength);
                memcpy(&piece.at(0), cStr() + start, sizeof(T) * pieceLength);
            }
            else
                result.append(Empty);

            start = stop + 1;
        }

        // Add a final empty piece if this string ends with a separator
        if (start == len && len)
            result.append(Empty);

        return result;
    }

    /**
     * Splits this string into lines and puts the resulting strings into \a lines.
     */
    Vector<StringBase<T>> getLines(bool keepEmptyLines = true) const
    {
        auto lines = split(Newline);

        // Remove any stray CR characters that will be present if this was a text file created on Windows
        for (auto i = 0U; i < lines.size(); i++)
        {
            if (lines[i].trimRight(CarriageReturn) && !lines[i].length())
                lines.erase(i--);
        }

        // Handle line continuations when a line ends with a backslash
        for (auto i = 1U; i < lines.size(); i++)
        {
            if (lines[i - 1].endsWith("\\"))
            {
                lines[i - 1].erase(lines[i - 1].length() - 1);
                lines[i - 1] << lines[i].trimmedLeft();
                lines.erase(i--);
            }
        }

        if (!keepEmptyLines)
            lines.eraseIf([](const StringBase<T>& line) { return line.isWhitespace(); });

        return lines;
    }

    /**
     * Returns whether this string is just whitespace (i.e. spaces, tab characters and newline characters).
     */
    bool isWhitespace() const
    {
        for (auto i = 0U; i < length(); i++)
        {
            auto character = at(i);
            if (character != ' ' && character != '\t' && character != '\n')
                return false;
        }

        return true;
    }

    /**
     * Removes any Ruby and Python-style '#' comment from this string.
     */
    void removeComments()
    {
        auto index = findFirstOf("#");
        if (index != -1)
            *this = substr(0, index);
    }

    /**
     * Returns whether the start of this string matches the given string. If the two strings are identical then true is
     * returned. If \a start is an empty string then true is returned.
     */
    bool startsWith(const StringBase<T>& start) const
    {
        if (start.length() == 0)
            return true;

        if (length() < start.length())
            return false;

        return substr(0, start.length()) == start;
    }

    /**
     * Returns whether the end of this string matches the given string. If the two strings are identical then true is returned.
     * If \a end is an empty string then true is returned.
     */
    bool endsWith(const StringBase<T>& end) const
    {
        if (end.length() == 0)
            return true;

        if (length() < end.length())
            return false;

        return substr(length() - end.length()) == end;
    }

    /**
     * Returns a copy of this string padded out to the given length with the character provided. If the length of this string
     * equals or exceeds the given pad length then it is returned unaltered.
     */
    StringBase<T> padToLength(unsigned int length, T c = ' ') const
    {
        auto s = *this;

        if (s.length() < length)
            s.resize(length, c);

        return s;
    }

    /**
     * Returns a copy of this string padded on its left hand side with the character provided in order to reach the given
     * length. If the length of this string equals or exceeds the given pad length then it is returned unaltered.
     */
    StringBase<T> prePadToLength(unsigned int length, T c = ' ') const
    {
        auto s = *this;

        while (s.length() < length)
            s.prepend(c);

        return s;
    }

    /**
     * Returns this string enclosed in double quotes if it has any of the given characters, otherwise just returns this string.
     */
    StringBase<T> quoteIfHas(const StringBase<T>& characters) const
    {
        if (findFirstOf(characters) == -1)
            return *this;

        return StringBase<T>("\"") + *this + "\"";
    }

    /**
     * Returns this string enclosed in double quotes if it has any spaces in it.
     */
    StringBase<T> quoteIfHasSpaces() const { return quoteIfHas(" "); }

    /**
     * Creates a Vector of tokens from the contents of this string using space and tab characters as separators. This is
     * different to StringBase::split() in that it correctly handles quoting, e.g. If this string is set to 'A "B C" D' then
     * this method will return the following three tokens: "A", "B C", "D".
     */
    Vector<StringBase<T>> getTokens() const
    {
        auto tokens = Vector<StringBase<T>>();

        for (auto i = 0U; i < length(); i++)
        {
            while (at(i) == ' ' || at(i) == '\t')
            {
                i++;
                if (i == length())
                    return tokens;
            }

            auto stopCharacters = "";

            if (at(i) == '"')
            {
                stopCharacters = "\"";
                i++;
            }
            else
                stopCharacters = " \t";

            auto index = findFirstOf(stopCharacters, i);
            if (index != -1)
            {
                tokens.append(substr(i, index - i));
                i = index;
            }
            else
            {
                tokens.append(substr(i));
                break;
            }
        }

        return tokens;
    }

    /**
     * Returns the number of times the given character occurs in this string.
     */
    unsigned int count(T character) const
    {
        auto count = 0U;

        for (auto i = 0U; i < length(); i++)
        {
            if (at(i) == character)
                count++;
        }

        return count;
    }

    /**
     * If this string is in the format "<name>[<index>]" then this method returns the index value between the square brackets at
     * the end, if an error occurs parsing this string then -1 is returned.
     */
    int getIndexInBrackets() const
    {
        // Check for ending bracket
        if (back() != ']')
            return -1;

        // Search for matching leading bracket
        auto index = findLastOf("[");
        if (index == -1)
            return -1;

        // Return the contents between the brackets
        auto result = substr(index + 1, length() - index - 2);

        return result.isNumeric() ? result.asInteger() : -1;
    }

    /**
     * If this string is in the format "<name>[<index>]" then this method returns the "<name>" portion and chops off the index
     * in square brackets, if an error occurs parsing this string then a copy of this string is returned unchanged.
     */
    StringBase<T> withoutIndexInBrackets() const
    {
        // Check for ending bracket
        if (back() != ']')
            return *this;

        // Search for matching leading bracket
        auto index = findLastOf("[");
        if (index == -1)
            return *this;

        return substr(0, index);
    }

    /**
     * Saves this string to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const;

    /**
     * Loads this string from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file);

    /**
     * Hashes the contents of this string using HashFunctions::hash().
     */
    unsigned int hash() const;

    /**
     * Converts the given unsigned integer to a string formatted with a comma between each set of 3 digits.
     */
    static StringBase<T> prettyPrint(unsigned int n)
    {
        if (n < 1000)
            return n;

        return prettyPrint(n / 1000) + "," + StringBase<T>(n % 1000).prePadToLength(3, '0');
    }

    /**
     * Returns the given fraction formatted as a percentage string with the given number of decimal places.
     */
    template <typename T2> static StringBase<T> formatPercentage(T2 numerator, T2 denominator, unsigned int decimalPlaces = 1)
    {
        auto percentage = 100.0f * (float(numerator) / float(denominator));

        auto s = StringBase<T>(int(percentage));

        if (decimalPlaces > 0)
            s << "." << int((percentage - float(int(percentage))) * 10.0f * float(decimalPlaces));

        s << "%";

        return s;
    }

    /**
     * Returns the raw data for the passed variable formatted as a human-readable hexadecimal string.
     */
    template <typename DataType> static StringBase<T> toHex(const DataType& value)
    {
        return toHex(reinterpret_cast<const byte_t*>(&value), sizeof(value));
    }

    /**
     * Returns the passed data formatted as a human-readable hexadecimal string. If \a addSpacing is set to true then a single
     * space will be inserted every 4 bytes to improve readability.
     */
    static StringBase<T> toHex(const byte_t* data, unsigned int size, bool addSpacing = true)
    {
        if (!data)
            return "null";

        static const auto digits =
            std::array<T, 16>{{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'}};

        auto s = StringBase<T>();

        for (auto i = 0U; i < size; i++)
        {
            if (addSpacing && i != 0 && (i % 4) == 0)
                s.append(' ');

            s.append(digits[(data[i] >> 4) & 0xF]);
            s.append(digits[data[i] & 0xF]);
        }

        return s;
    }

    /**
     * An empty string.
     */
    static const StringBase<T> Empty;

    /**
     * A string that is equal to " ".
     */
    static const StringBase<T> Space;

    /**
     * A string that is equal to ".".
     */
    static const StringBase<T> Period;

    /**
     * A string that is equal to "\\n".
     */
    static const StringBase<T> Newline;

    /**
     * A string that is equal to "\\r".
     */
    static const StringBase<T> CarriageReturn;

    /**
     * The default set of characters used by the string trimming routines. There are four characters in this set: space, tab,
     * newline, carriage return.
     */
    static const StringBase<T> TrimCharacters;

    /**
     * Returns the length of the longest string in the given vector of strings.
     */
    static unsigned int longestString(const Vector<StringBase<T>>& strings)
    {
        auto longest = 0U;

        for (auto& string : strings)
            longest = std::max(longest, string.length());

        return longest;
    }

    /**
     * Converts this string to an ASCII string, any characters that can't be represented in ASCII are replaced with a '?'
     * character. This method is implemented for UnicodeString but not String.
     */
    StringBase<char> toASCII() const;

    /**
     * Converts this string to UTF-8. By default the returned vector includes a null terminator, but this can be overridden by
     * setting \a includeNullTerminator to false.
     */
    Vector<byte_t> toUTF8(bool includeNullTerminator = true) const;

    /**
     * Converts this string to UTF-16. By default the returned vector includes a null terminator, but this can be overridden by
     * setting \a includeNullTerminator to false.
     */
    Vector<uint16_t> toUTF16(bool includeNullTerminator = true) const;

#ifdef __OBJC__
    /**
     * On Mac OS X and iOS, converts this string to an NSString.
     */
    NSString* toNSString() const
    {
        const auto& utf8 = toUTF8();

        return [[NSString alloc] initWithData:[NSData dataWithBytes:utf8.getData() length:utf8.size() - 1]
                                     encoding:NSUTF8StringEncoding];
    }
#endif

    /**
     * Converts this string to UTF8 and copies the resulting bytes including a null terminator into the specified destination
     * buffer. If the buffer is too small then false is returned.
     */
    bool copyUTF8To(void* destination, unsigned int destinationSize) const
    {
        const auto& utf8 = toUTF8();

        if (utf8.size() > destinationSize)
            return false;

        memcpy(destination, utf8.getData(), utf8.size());

        return true;
    }

    /**
     * Returns whether or not the passed character is a printable ASCII character, i.e. whether it lies in the range 0x20 -
     * 0x7E.
     */
    static bool isPrintableASCII(int c) { return c >= 0x20 && c <= 0x7E; }

    /**
     * Implicit conversion to a UTF-32 string, only implemented for UTF-8 strings.
     */
    operator StringBase<UnicodeCharacter>() const;

private:

    // Depending on the length of the string this will either point to localStorage_.raw or a heap allocation
    T* storage_ = nullptr;

    unsigned int length_ = 0;

    // The size of the string class is 32 bytes for ASCII strings and 64 bytes for UTF-32 strings.
    static const auto SizeOfStringClass = (sizeof(T) == 1) ? 32 : 64;

    // The size of the other string class members determines how large the local storage buffer can be
    static const auto LocalStorageSize = SizeOfStringClass - sizeof(T*) - sizeof(unsigned int);

    union LocalStorage {
        T raw[LocalStorageSize / sizeof(T)];
        unsigned int heapAllocationSize;
    } localStorage_;

    bool isUsingLocalStorage() const { return length() < LocalStorageSize / sizeof(T); }
    bool isUsingHeapStorage() const { return length() >= LocalStorageSize / sizeof(T); }
};

/**
 * Typedef for the ASCII string type.
 */
typedef StringBase<char> String;

/**
 * Typedef for the Unicode string type.
 */
typedef StringBase<UnicodeCharacter> UnicodeString;

/**
 * Enable `const char *` to be added directly to a StringBase.
 */
template <typename T> StringBase<T> operator+(const char* s1, const StringBase<T>& s2)
{
    return StringBase<T>(s1) + s2;
}

/**
 * Enable `const char *` to be streamed directly to a StringBase.
 */
template <typename T> StringBase<T> operator<<(const char* s1, const StringBase<T>& s2)
{
    return StringBase<T>(s1) + s2;
}

/**
 * Stream concatenation for String instances.
 */
CARBON_API String& operator<<(String& s1, const char* s2);

/**
 * Stream concatenation for String instances.
 */
CARBON_API String& operator<<(String& s1, const String& s2);

/**
 * Stream concatenation for String instances.
 */
CARBON_API String& operator<<(String& s1, String& s2);

/**
 * Stream concatenation for String instances.
 */
CARBON_API String& operator<<(String& s1, String&& s2);

/**
 * Stream concatenation for UnicodeString instances.
 */
CARBON_API UnicodeString& operator<<(UnicodeString& s1, const char* s2);

/**
 * Stream concatenation for UnicodeString instances.
 */
CARBON_API UnicodeString& operator<<(UnicodeString& s1, const UnicodeString& s2);

/**
 * Stream concatenation for UnicodeString instances.
 */
CARBON_API UnicodeString& operator<<(UnicodeString& s1, UnicodeString& s2);

/**
 * Stream concatenation for UnicodeString instances.
 */
CARBON_API UnicodeString& operator<<(UnicodeString& s1, UnicodeString&& s2);

/**
 * Stream concatenation for UnicodeString instances.
 */
CARBON_API UnicodeString& operator<<(UnicodeString& s1, const String& s2);

/**
 * Stream concatenation for UnicodeString instances.
 */
CARBON_API UnicodeString& operator<<(UnicodeString& s1, String& s2);

/**
 * Stream concatenation for UnicodeString instances.
 */
CARBON_API UnicodeString& operator<<(UnicodeString& s1, String&& s2);

/**
 * Stream concatenation of Exception onto a UnicodeString.
 */
CARBON_API UnicodeString& operator<<(UnicodeString& s, const Exception& e);

/**
 * Stream concatenation onto StringBase for any type that can be passed to a StringBase constructor.
 */
template <typename T, typename ArgType> StringBase<T>& operator<<(StringBase<T>& s, ArgType&& arg)
{
    return s += StringBase<T>(std::forward<ArgType>(arg));
}

/**
 * Stream concatenation onto StringBase for any type that can be passed to a StringBase constructor.
 */
template <typename T, typename ArgType> StringBase<T>&& operator<<(StringBase<T>&& s, ArgType&& arg)
{
    return std::move(s += StringBase<T>(std::forward<ArgType>(arg)));
}

/**
 * Converts raw UTF-8 character data to a UnicodeString.
 */
CARBON_API UnicodeString fromUTF8(const byte_t* data, unsigned int size);

/**
 * Converts a null-terminated UTF-8 string to a UnicodeString.
 */
CARBON_API UnicodeString fromUTF8(const char* string);

/**
 * Converts raw UTF-16 character data to a UnicodeString.
 */
CARBON_API UnicodeString fromUTF16(const uint16_t* data, unsigned int size);

/**
 * Converts a null-terminated UTF-16 string to a UnicodeString. Intended for use on Windows where `wchar_t` is two bytes wide.
 */
CARBON_API UnicodeString fromUTF16(const wchar_t* string);

/**
 * Shorthand function to down-convert a Unicode string to an ASCII string, characters not part of ASCII are replaced with '?'.
 */
CARBON_API String A(const UnicodeString& s);

/**
 * Shorthand function to down-convert a Unicode string vector to an ASCII string vector, characters not part of ASCII are
 * replaced with '?'.
 */
CARBON_API Vector<String> A(const Vector<UnicodeString>& v);

/**
 * Shorthand function to up-convert an ASCII string vector to a UnicodeString vector.
 */
CARBON_API Vector<UnicodeString> U(const Vector<String>& v);

}

namespace std
{

template <> struct hash<Carbon::String>
{
    size_t operator()(const Carbon::String& s) const;
};

template <> struct hash<Carbon::UnicodeString>
{
    size_t operator()(const Carbon::UnicodeString& s) const;
};

}
