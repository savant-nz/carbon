/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/HashFunctions.h"

namespace Carbon
{

template <> const String String::Empty = "";
template <> const String String::Space = " ";
template <> const String String::Period = ".";
template <> const String String::Newline = "\n";
template <> const String String::CarriageReturn = "\r";
template <> const String String::TrimCharacters = " \t\r\n";

template <> const UnicodeString UnicodeString::Empty = "";
template <> const UnicodeString UnicodeString::Space = " ";
template <> const UnicodeString UnicodeString::Period = ".";
template <> const UnicodeString UnicodeString::Newline = "\n";
template <> const UnicodeString UnicodeString::CarriageReturn = "\r";
template <> const UnicodeString UnicodeString::TrimCharacters = " \t\r\n";

template <> StringBase<char>::StringBase(int64_t value) : StringBase(std::to_string(value).c_str())
{
}

template <> StringBase<char>::StringBase(uint64_t value) : StringBase(std::to_string(value).c_str())
{
}

template <> StringBase<char>::StringBase(double value) : StringBase(std::to_string(value).c_str())
{
    auto index = find(".");
    if (index != -1)
        trimRight("0", index + 2);
}

template <> StringBase<UnicodeCharacter>::StringBase(int64_t value) : StringBase(std::to_string(value).c_str())
{
}

template <> StringBase<UnicodeCharacter>::StringBase(uint64_t value) : StringBase(std::to_string(value).c_str())
{
}

template <> StringBase<UnicodeCharacter>::StringBase(double value) : StringBase(std::to_string(value).c_str())
{
    auto index = find(".");
    if (index != -1)
        trimRight("0", index + 2);
}

template <> unsigned int String::hash() const
{
    return HashFunctions::hash(cStr(), length() * sizeof(char));
}

template <> unsigned int UnicodeString::hash() const
{
    return HashFunctions::hash(cStr(), length() * sizeof(UnicodeCharacter));
}

template <> String String::toASCII() const
{
    return *this;
}

template <> String UnicodeString::toASCII() const
{
    auto result = String(0, length());

    for (auto i = 0U; i < result.length(); i++)
        result.at(i) = at(i) < 0x80 ? char(at(i)) : '?';

    return result;
}

template <> Vector<byte_t> String::toUTF8(bool includeNullTerminator) const
{
    auto result = Vector<byte_t>();

    for (auto i = 0U; i < length(); i++)
        result.append(at(i) > 0 ? at(i) : '?');

    if (includeNullTerminator)
        result.append(0);

    return result;
}

template <> Vector<byte_t> UnicodeString::toUTF8(bool includeNullTerminator) const
{
    auto result = Vector<byte_t>();

    for (auto i = 0U; i < length(); i++)
    {
        auto c = at(i);

        if (c < 0x80)
            result.append(byte_t(c));
        else if (c < 0x800)
        {
            result.append(0xC0 | ((c >> 6) & 0x1F));
            result.append(0x80 | (c & 0x3F));
        }
        else if (c < 0x10000)
        {
            result.append(0xE0 | ((c >> 12) & 0x0F));
            result.append(0x80 | ((c >> 6) & 0x3F));
            result.append(0x80 | (c & 0x3F));
        }
        else if (c < 0x110000)
        {
            result.append(0xF0 | ((c >> 18) & 0x07));
            result.append(0x80 | ((c >> 12) & 0x3F));
            result.append(0x80 | ((c >> 6) & 0x3F));
            result.append(0x80 | (c & 0x3F));
        }
    }

    if (includeNullTerminator)
        result.append(0);

    return result;
}

template <> Vector<uint16_t> String::toUTF16(bool includeNullTerminator) const
{
    auto result = Vector<uint16_t>();

    for (auto i = 0U; i < length(); i++)
        result.append(at(i) > 0 ? at(i) : '?');

    if (includeNullTerminator)
        result.append(0);

    return result;
}

template <> Vector<uint16_t> UnicodeString::toUTF16(bool includeNullTerminator) const
{
    auto result = Vector<uint16_t>();

    for (auto i = 0U; i < length(); i++)
    {
        auto c = at(i);

        if (c <= 0xD7FF || (c >= 0xE000 && c <= 0xFFFD))
            result.append(uint16_t(c));
        else if (c < 0x110000)
        {
            result.append(0xD800 | ((c >> 10) & 0x3FF));    // High surrogate, bits 10-19
            result.append(0xDC00 | (c & 0x3FF));            // Low surrogate, bits 0-9
        }
    }

    if (includeNullTerminator)
        result.append(0);

    return result;
}

template <> String::operator UnicodeString() const
{
    return cStr();
}

#ifdef _MSC_VER

template <> UnicodeString::operator UnicodeString() const
{
    return *this;
}

#endif

CARBON_API String& operator<<(String& s1, const char* s2)
{
    auto s1Length = s1.length();
    auto s2Length = uint(strlen(s2));

    s1.resize(s1Length + s2Length);
    memcpy(&s1.at(s1Length), s2, s2Length);

    return s1;
}

CARBON_API String& operator<<(String& s1, const String& s2)
{
    return s1 += s2;
}

CARBON_API String& operator<<(String& s1, String& s2)
{
    return s1 += s2;
}

CARBON_API String& operator<<(String& s1, String&& s2)
{
    return s1 += s2;
}

CARBON_API UnicodeString& operator<<(UnicodeString& s1, const char* s2)
{
    auto s1Length = s1.length();
    auto s2Length = uint(strlen(s2));

    s1.resize(s1Length + s2Length);
    for (auto i = 0U; i < s2Length; i++)
        s1.at(s1Length + i) = UnicodeCharacter(s2[i]);

    return s1;
}

CARBON_API UnicodeString& operator<<(UnicodeString& s1, const UnicodeString& s2)
{
    return s1 += s2;
}

CARBON_API UnicodeString& operator<<(UnicodeString& s1, UnicodeString& s2)
{
    return s1 += s2;
}

CARBON_API UnicodeString& operator<<(UnicodeString& s1, UnicodeString&& s2)
{
    return s1 += s2;
}

CARBON_API UnicodeString& operator<<(UnicodeString& s1, const String& s2)
{
    return s1 += s2;
}

CARBON_API UnicodeString& operator<<(UnicodeString& s1, String& s2)
{
    return s1 += s2;
}

CARBON_API UnicodeString& operator<<(UnicodeString& s1, String&& s2)
{
    return s1 += s2;
}

CARBON_API UnicodeString& operator<<(UnicodeString& s, const Exception& e)
{
    return s += e;
}

static bool isUTF8TrailingByte(byte_t c)
{
    return (c >> 6) == 2;
}

CARBON_API UnicodeString fromUTF8(const byte_t* data, unsigned int size)
{
    auto result = UnicodeString();

    if (!data)
        return result;

    // If the UTF-8 data starts with a U+FEFF byte order mark then just skip over it
    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
    {
        data += 3;
        size -= 3;
    }

    result.resize(size);

    auto characterCount = 0U;

    for (auto i = 0U; i < size; i++)
    {
        auto cp = UnicodeCharacter(data[i]);

        if (cp <= 0x7F)
            ;
        else if ((cp >> 5) == 0x6)
        {
            // 2 bytes encode this code point
            if (i + 1 < size && isUTF8TrailingByte(data[i + 1]))
            {
                cp = ((cp & 0x1F) << 6) | (data[i + 1] & 0x3F);
                i++;
            }
            else
            {
                LOG_ERROR << "Incomplete or invalid 2-byte UTF-8 code point";
                break;
            }
        }
        else if ((cp >> 4) == 0xE)
        {
            // 3 bytes encode this code point
            if (i + 2 < size && isUTF8TrailingByte(data[i + 1]) && isUTF8TrailingByte(data[i + 2]))
            {
                cp = ((cp & 0xF) << 12) | ((data[i + 1] & 0x3F) << 6) | (data[i + 2] & 0x3F);
                i += 2;
            }
            else
            {
                LOG_ERROR << "Incomplete or invalid 3-byte UTF-8 code point";
                break;
            }
        }
        else if ((cp >> 3) == 0x1E)
        {
            // 4 bytes encode this code point
            if (i + 3 < size && isUTF8TrailingByte(data[i + 1]) && isUTF8TrailingByte(data[i + 2]) &&
                isUTF8TrailingByte(data[i + 3]))
            {
                cp = ((cp & 0x7) << 18) | ((data[i + 1] & 0x3F) << 12) | ((data[i + 2] & 0x3F) << 6) |
                    (data[i + 3] & 0x3F);
                i += 3;
            }
            else
            {
                LOG_ERROR << "Incomplete or invalid 4-byte UTF-8 code point";
                break;
            }
        }
        else
        {
            LOG_ERROR << "Invalid UTF-8 lead character";
            break;
        }

        if (!cp)
            break;

        result.at(characterCount++) = cp;
    }

    result.resize(characterCount);

    return result;
}

CARBON_API UnicodeString fromUTF8(const char* string)
{
    if (!string)
        return {};

    return fromUTF8(reinterpret_cast<const byte_t*>(string), uint(strlen(string)));
}

CARBON_API UnicodeString fromUTF16(const uint16_t* data, unsigned int size)
{
    if (!data)
        return {};

    auto result = UnicodeString();

    for (auto i = 0U; i < size; i++)
    {
        auto c = data[i];

        if (c <= 0xD7FF || (c >= 0xE000 && c <= 0xFFFD))
            result.append(c);
        else if (c <= 0xDBFF)
        {
            // This is a high surrogate, so decode it as a pair along with the next character
            if (i + 1 < size && data[i + 1] >= 0xDC00 && data[i + 1] <= 0xDFFF)
            {
                result.append(0x10000 + ((c & 0x03FF) << 10) + (data[i + 1] & 0x03FF));
                i++;    // Move past low surrogate
            }
            else
            {
                LOG_ERROR << "Invalid UTF-16, high surrogate without a following low surrogate";
                break;
            }
        }
        else
        {
            LOG_ERROR << "Invalid UTF-16 character, value: " << c;
            break;
        }
    }

    return result;
}

CARBON_API UnicodeString fromUTF16(const wchar_t* string)
{
    return string ? fromUTF16(reinterpret_cast<const uint16_t*>(string), uint(wcslen(string))) : UnicodeString::Empty;
}

CARBON_API String A(const UnicodeString& s)
{
    return s.toASCII();
}

CARBON_API Vector<String> A(const Vector<UnicodeString>& v)
{
    return v.map<String>([](const UnicodeString& s) { return A(s); });
}

CARBON_API Vector<UnicodeString> U(const Vector<String>& v)
{
    return v.map<UnicodeString>([](const String& s) { return s; });
}

template <> void String::save(FileWriter& file) const
{
    file.write(toUTF8(false));
}

template <> void UnicodeString::save(FileWriter& file) const
{
    file.write(toUTF8(false));
}

template <> void String::load(FileReader& file)
{
    auto length = 0U;
    file.read(length);

    try
    {
        resize(length);
    }
    catch (const std::bad_alloc&)
    {
        throw Exception() << "Failed allocating memory for string, length: " << length;
    }

    // Read UTF-8 as ASCII
    file.readBytes(storage_, length);
}

template <> void UnicodeString::load(FileReader& file)
{
    // Read the UTF-8 data
    auto utf8 = Vector<byte_t>();
    file.read(utf8);

    // Convert from UTF-8
    *this = fromUTF8(utf8.getData(), utf8.size());
}

// Explicitly instantiate the string classes to ensure that all of their methods are defined
template class StringBase<char>;
template class StringBase<UnicodeCharacter>;

}

namespace std
{

size_t hash<Carbon::String>::operator()(const Carbon::String& s) const
{
    return s.hash();
}

size_t hash<Carbon::UnicodeString>::operator()(const Carbon::UnicodeString& s) const
{
    return s.hash();
}

}
