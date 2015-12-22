/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Math/Color.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Quaternion.h"
#include "CarbonEngine/Math/Vec2.h"
#include "CarbonEngine/Math/Vec3.h"

namespace Carbon
{

/**
 * Holds a value that can be a boolean, integer, float, Vec2, Vec3, Quaternion, Color, Float4, String or untyped pointer. Only
 * one of these types is held at any given time. The ParameterArray class is used to manage a collection of named Parameter
 * instances in a key => value style.
 */
class CARBON_API Parameter
{
public:

    /**
     * An empty parameter.
     */
    static const Parameter Empty;

    /**
     * Enumeration of the supported types of parameter.
     */
    enum Type
    {
        NullParameter,
        BooleanParameter,
        IntegerParameter,
        FloatParameter,
        Vec2Parameter,
        Vec3Parameter,
        QuaternionParameter,
        ColorParameter,
        Float4Parameter,
        StringParameter,
        PointerParameter
    };

    Parameter() { clear(); }

    /**
     * Copy constructor.
     */
    Parameter(const Parameter& other);

    /**
     * Constructor that initializes this parameter to the given boolean value.
     */
    Parameter(bool value) : Parameter() { setBoolean(value); }

    /**
     * Constructor that initializes this parameter to the given integer value.
     */
    Parameter(int value) : Parameter() { setInteger(value); }

    /**
     * Constructor that initializes this parameter to the given unsigned integer value.
     */
    Parameter(unsigned int value) : Parameter() { setInteger(value); }

    /**
     * Constructor that initializes this parameter to the given integer value.
     */
    Parameter(int64_t value) : Parameter() { setInteger(value); }

    /**
     * Constructor that initializes this parameter to the given unsigned integer value.
     */
    Parameter(uint64_t value) : Parameter() { setInteger(int64_t(value)); }

    /**
     * Constructor that initializes this parameter to the given floating point value.
     */
    Parameter(float value) : Parameter() { setFloat(value); }

    /**
     * Constructor that initializes this parameter to the given Vec2 value.
     */
    Parameter(const Vec2& value) : Parameter() { setVec2(value); }

    /**
     * Constructor that initializes this parameter to the given Vec3 value.
     */
    Parameter(const Vec3& value) : Parameter() { setVec3(value); }

    /**
     * Constructor that initializes this parameter to the given Quaternion value.
     */
    Parameter(const Quaternion& value) : Parameter() { setQuaternion(value); }

    /**
     * Constructor that initializes this parameter to the given Color value.
     */
    Parameter(const Color& value) : Parameter() { setColor(value); }

    /**
     * Constructor that initializes this parameter to the given Float4 value.
     */
    Parameter(float f0, float f1, float f2, float f3) : Parameter() { setFloat4(f0, f1, f2, f3); }

    /**
     * Constructor that initializes this parameter to the given string value.
     */
    Parameter(const char* value) : Parameter() { setString(value); }

    /**
     * Constructor that initializes this parameter to the given string value.
     */
    Parameter(const String& value) : Parameter() { setString(value); }

    /**
     * Constructor that initializes this parameter to the given pointer value.
     */
    Parameter(const void* pointer) : Parameter() { setPointer(pointer); }

    /**
     * Assignment operator.
     */
    Parameter& operator=(Parameter other)
    {
        swap(*this, other);
        return *this;
    }

    /**
     * Swaps the contents of two Parameter instances.
     */
    friend void swap(Parameter& first, Parameter& second);

    /**
     * Clears the value held by this parameter.
     */
    void clear()
    {
        boolean_.isCached = false;
        integer_.isCached = false;
        float4_.isCached = false;
        string_.isCached = false;
        pointer_.isCached = false;

        string_.value.clear();
        masterValue_ = &string_;
    }

    /**
     * Returns the boolean value of this parameter.
     */
    bool getBoolean() const
    {
        if (masterValue_ && !boolean_.isCached)
            boolean_.set(masterValue_->toString().asBoolean());

        return boolean_.value;
    }

    /**
     * Returns the integer value of this parameter, note that this is a cast-down version of the true 64-bit integer that is
     * stored internally, to get the full 64-bit value call Parameter::getInteger64().
     */
    int getInteger() const { return int(getInteger64()); }

    /**
     * Returns the 64-bit integer value of this parameter.
     */
    int64_t getInteger64() const
    {
        if (masterValue_ && !integer_.isCached)
            integer_.set(masterValue_->toString().asInteger());

        return integer_.value;
    }

    /**
     * Returns the floating point value of this parameter.
     */
    float getFloat() const
    {
        if (masterValue_ && !float4_.isCached)
            updateFloat4();

        return float4_.value.f[0];
    }

    /**
     * Returns the Vec2 value of this parameter.
     */
    const Vec2& getVec2() const
    {
        if (masterValue_ && !float4_.isCached)
            updateFloat4();

        return *reinterpret_cast<const Vec2*>(float4_.value.f.data());
    }

    /**
     * Returns the Vec3 value of this parameter.
     */
    const Vec3& getVec3() const
    {
        if (masterValue_ && !float4_.isCached)
            updateFloat4();

        return *reinterpret_cast<const Vec3*>(float4_.value.f.data());
    }

    /**
     * Returns the Quaternion value of this parameter.
     */
    const Quaternion& getQuaternion() const
    {
        if (masterValue_ && !float4_.isCached)
            updateFloat4();

        return *reinterpret_cast<const Quaternion*>(float4_.value.f.data());
    }

    /**
     * Returns the Color value of this parameter.
     */
    const Color& getColor() const
    {
        if (masterValue_ && !float4_.isCached)
            updateFloat4();

        return *reinterpret_cast<const Color*>(float4_.value.f.data());
    }

    /**
     * Returns the Float4 value of this parameter.
     */
    const float* getFloat4() const
    {
        if (masterValue_ && !float4_.isCached)
            updateFloat4();

        return float4_.value.f.data();
    }

    /**
     * Returns the string value of this parameter.
     */
    const String& getString() const
    {
        if (masterValue_ && !string_.isCached)
            string_.set(masterValue_->toString());

        return string_.value;
    }

    /**
     * Returns the string value of this parameter.
     */
    operator UnicodeString() const { return getString(); }

    /**
     * Returns the pointer value of this parameter cast to the specified type.
     */
    template <typename T> T* getPointer() const { return pointer_.isCached ? reinterpret_cast<T*>(pointer_.value) : nullptr; }

    /**
     * Sets this parameter to a boolean value.
     */
    void setBoolean(bool value) { setMasterValue(boolean_, value); }

    /**
     * Sets this parameter to an integer value.
     */
    void setInteger(int64_t value) { setMasterValue(integer_, value); }

    /**
     * Sets this parameter to a floating point value.
     */
    void setFloat(float value) { setMasterValue(float4_, Float4(value)); }

    /**
     * Sets this parameter to a Vec2 value.
     */
    void setVec2(const Vec2& v) { setMasterValue(float4_, Float4(v.x, v.y)); }

    /**
     * Sets this parameter to a Vec3 value.
     */
    void setVec3(const Vec3& v) { setMasterValue(float4_, Float4(v.x, v.y, v.z)); }

    /**
     * Sets this parameter to a Quaternion value.
     */
    void setQuaternion(const Quaternion& q) { setMasterValue(float4_, Float4(q.x, q.y, q.z, q.w)); }

    /**
     * Sets this parameter to a Color value.
     */
    void setColor(const Color& c) { setMasterValue(float4_, Float4(c.r, c.g, c.b, c.a)); }

    /**
     * Sets this parameter to a Float4 value.
     */
    void setFloat4(float f0, float f1, float f2, float f3) { setMasterValue(float4_, Float4(f0, f1, f2, f3)); }

    /**
     * Sets this parameter to a String value.
     */
    void setString(const String& value) { setMasterValue(string_, value); }

    /**
     * Sets this parameter to an untyped pointer value.
     */
    template <typename T = void> void setPointer(const T* value)
    {
        setMasterValue(pointer_, reinterpret_cast<void*>(const_cast<T*>(value)));
    }

    /**
     * Saves this parameter to a file stream. Throws an Exception if an error occurs.
     */
    void save(FileWriter& file) const { file.write(masterValue_ ? masterValue_->toString() : String::Empty); }

    /**
     * Loads this parameter from a file stream. Throws an Exception if an error occurs.
     */
    void load(FileReader& file)
    {
        auto s = String();
        file.read(s);
        setString(s);
    }

    /**
     * Converts a string to a parameter type enumeration value. The recognized parameter type strings are: "boolean", "integer",
     * "float", "vec2", "vec3", "quaternion", "color", "float4", "string" and "pointer". This method is not case sensitive.
     * Parameter::NullParameter will be returned if no parameter type string is recognized.
     */
    static Type getTypeFromString(const String& s);

    /**
     * Returns whether the passed parameter name is valid, valid names can only contain letters, numbers, and the following
     * special characters: . _ [ ].
     */
    static bool isValidParameterName(const String& name) { return name.isAlphaNumeric("._[]"); }

    /**
     * Returns the hidden parameter name for the given parameter name, this is simply the passed name with a period character
     * prepended.
     */
    static String getHiddenParameterName(const String& name) { return "." + name; }

    /**
     * Returns whether the passed name is for a hidden parameter, hidden parameters start with a period character.
     */
    static bool isHiddenParameterName(const String& name) { return name.length() && name.at(0) == '.'; }

private:

    // Interface for a cached value that can be turned into a string
    struct CachedValueBase
    {
        virtual ~CachedValueBase() {}
        virtual String toString() const { return {}; }
    };

    // Holds a cached value of a single type
    template <typename T> struct CachedValue : public CachedValueBase
    {
        mutable T value = T();
        mutable bool isCached = false;

        CachedValue() {}
        CachedValue(const CachedValue& other) : value(other.value), isCached(other.isCached) {}

        CachedValue<T>& operator=(CachedValue<T> other)
        {
            swap(*this, other);
            return *this;
        }

        friend void swap(CachedValue<T>& first, CachedValue<T>& second)
        {
            using std::swap;

            swap(first.value, second.value);
            swap(first.isCached, second.isCached);
        }

        void set(const T& newValue) const
        {
            value = newValue;
            isCached = true;
        }

        String toString() const override { return value; }
    };

    // This Float4 type is used to cover float, Vec2, Vec3, Quaternion and Color types
    struct Float4
    {
        std::array<float, 4> f = {};
        unsigned int usedValueCount = 0;

        Float4() {}
        Float4(float f0) : f{{f0, 0.0f, 0.0f, 0.0f}}, usedValueCount(1) {}
        Float4(float f0, float f1) : f{{f0, f1, 0.0f, 0.0f}}, usedValueCount(2) {}
        Float4(float f0, float f1, float f2) : f{{f0, f1, f2, 0.0f}}, usedValueCount(3) {}
        Float4(float f0, float f1, float f2, float f3) : f{{f0, f1, f2, f3}}, usedValueCount(4) {}

        operator String() const
        {
            auto s = String();

            for (auto i = 0U; i < usedValueCount; i++)
            {
                s << f[i];
                if (i != usedValueCount - 1)
                    s << " ";
            }

            return s;
        }
    };

    // Instantiate CachedValue for every supported type
    CachedValue<bool> boolean_;
    CachedValue<int64_t> integer_;
    CachedValue<Float4> float4_;
    CachedValue<String> string_;
    CachedValue<void*> pointer_;

    // The master value will either be null or point to the above CachedValue instance that was explicitly set most recently
    CachedValueBase* masterValue_;

    template <typename ValueType> void setMasterValue(CachedValue<ValueType>& cachedValue, const ValueType& value)
    {
        clear();
        cachedValue.set(value);
        masterValue_ = &cachedValue;
    }

    // If a cached value is made up of a number of individual floating point values then this method can be used to update it
    // based on the current contents of the master value.
    void updateFloat4() const
    {
        // Default to 0,0,0,1
        float4_.value.f[0] = 0.0f;
        float4_.value.f[1] = 0.0f;
        float4_.value.f[2] = 0.0f;
        float4_.value.f[3] = 1.0f;

        auto values = masterValue_->toString().getTokens();
        if (values.size() > 4)
            values.resize(4);

        for (auto i = 0U; i < values.size(); i++)
            float4_.value.f[i] = values[i].asFloat();

        float4_.isCached = true;
    }

public:

#ifndef DOXYGEN

    // Following are all the ParameterArray lookups used by the engine, they are stored here for ease of access. Lookups are
    // much faster when not using strings, i.e. params[Parameter::diffuseColor] is preferred over params["diffuseColor"] even
    // though both return the same result.

    static const ParameterArray::Lookup diffuseMap;
    static const ParameterArray::Lookup normalMap;
    static const ParameterArray::Lookup glossMap;
    static const ParameterArray::Lookup lightMap;
    static const ParameterArray::Lookup heightfieldTexture;
    static const ParameterArray::Lookup baseMap;
    static const ParameterArray::Lookup detailMap;
    static const ParameterArray::Lookup normalAndHeightMap;
    static const ParameterArray::Lookup edgeLookupMap;
    static const ParameterArray::Lookup opacityMap;
    static const ParameterArray::Lookup specularShiftMap;
    static const ParameterArray::Lookup specularNoiseMap;
    static const ParameterArray::Lookup shadowMap;
    static const ParameterArray::Lookup reflectanceMap;
    static const ParameterArray::Lookup inputTexture;
    static const ParameterArray::Lookup depthTexture;
    static const ParameterArray::Lookup addTexture;

    static const ParameterArray::Lookup scaleAndOffset;
    static const ParameterArray::Lookup isLightingAllowed;
    static const ParameterArray::Lookup lightColor;
    static const ParameterArray::Lookup lightDirection;
    static const ParameterArray::Lookup lightPosition;
    static const ParameterArray::Lookup lightOrientation;
    static const ParameterArray::Lookup lightRadius;
    static const ParameterArray::Lookup lightAmbient;
    static const ParameterArray::Lookup lightViewProjectionMatrix;
    static const ParameterArray::Lookup color;
    static const ParameterArray::Lookup diffuseColor;
    static const ParameterArray::Lookup isSpecularEnabled;
    static const ParameterArray::Lookup specularIntensity;
    static const ParameterArray::Lookup specularColor;
    static const ParameterArray::Lookup specularExponent;
    static const ParameterArray::Lookup scale;
    static const ParameterArray::Lookup scales;
    static const ParameterArray::Lookup finalScale;
    static const ParameterArray::Lookup blurType;
    static const ParameterArray::Lookup exposure;
    static const ParameterArray::Lookup addTextureFactor;
    static const ParameterArray::Lookup turbidity;
    static const ParameterArray::Lookup rayleighCoefficient;
    static const ParameterArray::Lookup mieCoefficient;
    static const ParameterArray::Lookup g;
    static const ParameterArray::Lookup clipmapSize;
    static const ParameterArray::Lookup clipmapOrigin;
    static const ParameterArray::Lookup clipmapCameraPosition;
    static const ParameterArray::Lookup bloomFactor;
    static const ParameterArray::Lookup blurStandardDeviation;
    static const ParameterArray::Lookup easing;
    static const ParameterArray::Lookup depthWrite;
    static const ParameterArray::Lookup blend;
    static const ParameterArray::Lookup blendSourceFactor;
    static const ParameterArray::Lookup blendDestinationFactor;
    static const ParameterArray::Lookup minimumConeAngle;
    static const ParameterArray::Lookup maximumConeAngle;
    static const ParameterArray::Lookup projectionCubemap;
    static const ParameterArray::Lookup projectionTexture;
    static const ParameterArray::Lookup tilingFactor;
    static const ParameterArray::Lookup reflectionDistortion;
    static const ParameterArray::Lookup refractionDistortion;
    static const ParameterArray::Lookup boneCount;
    static const ParameterArray::Lookup boneTransforms;
    static const ParameterArray::Lookup weightsPerVertex;
    static const ParameterArray::Lookup useVertexColor;
    static const ParameterArray::Lookup stereo;
    static const ParameterArray::Lookup distortionCoefficients;
    static const ParameterArray::Lookup lensCenter;
    static const ParameterArray::Lookup chromaticAberration;

#endif
};

}
