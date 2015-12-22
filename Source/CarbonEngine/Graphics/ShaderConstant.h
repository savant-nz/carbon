/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Math/Matrix4.h"
#include "CarbonEngine/Math/SimpleTransform.h"

namespace Carbon
{

/**
 * Manages a single constant used by a shader program. This class is subclassed by graphics interface implementations that
 * support specific shader languages, and instances of the subclass are then returned by ShaderProgram::getConstant() for
 * assignment. This class handles caching of shader constant values to avoid unnecessary updates.
 */
class CARBON_API ShaderConstant
{
public:

    /**
     * Constructs this shader constant with the given parameter name. The parameter name is used by the ShaderConstant::set*()
     * methods that take a ParameterArray reference, which use it to look up the parameter to assign to this shader constant.
     */
    ShaderConstant(const String& parameterName) : parameterName_(parameterName), parameterLookup_(parameterName) {}

    virtual ~ShaderConstant() {}

    /**
     * Sets the value of this shader constant.
     */
    void setFloat(float f0)
    {
        if (current_.f[0] != f0)
            setFloatUncached(current_.f[0] = f0);
    }

    /**
     * Sets the value of this shader constant.
     */
    void setFloat2(float f0, float f1)
    {
        if (current_.f[0] != f0 || current_.f[1] != f1)
            setFloat2Uncached(current_.f[0] = f0, current_.f[1] = f1);
    }

    /**
     * Sets the value of this shader constant.
     */
    void setFloat3(float f0, float f1, float f2)
    {
        if (current_.f[0] != f0 || current_.f[1] != f1 || current_.f[2] != f2)
            setFloat3Uncached(current_.f[0] = f0, current_.f[1] = f1, current_.f[2] = f2);
    }

    /**
     * Sets the value of this shader constant.
     */
    void setFloat4(float f0, float f1, float f2, float f3)
    {
        if (current_.f[0] != f0 || current_.f[1] != f1 || current_.f[2] != f2 || current_.f[3] != f3)
            setFloat4Uncached(current_.f[0] = f0, current_.f[1] = f1, current_.f[2] = f2, current_.f[3] = f3);
    }

    /**
     * Sets the value of this shader constant.
     */
    void setInteger(int i0)
    {
        if (current_.i[0] != i0)
            setIntegerUncached(current_.i[0] = i0);
    }

    /**
     * Sets the value of this shader constant. The component count indicates the number of individual float components in each
     * item of the array. The item count indicates the number of items in the array. Note that array shader constants are not
     * cached and so calling this method will always result in a hardware update by the graphics interface.
     */
    virtual void setArray(unsigned int componentCount, unsigned int itemCount, const float* data)
    {
        LOG_ERROR << "Float arrays are not supported by this shader language";
    }

    /**
     * Sets the value of this shader constant.
     */
    void setMatrix3(const Matrix3& m)
    {
        if (current_.matrix3 != m)
            setMatrix3Uncached(current_.matrix3 = m);
    }

    /**
     * Sets the value of this shader constant as the 3x3 matrix represented by the given quaternion and scale. See
     * setMatrix3(const Matrix3 &) for details.
     */
    void setMatrix3(const Quaternion& orientation, const Vec3& scale = Vec3::One)
    {
        auto matrix = orientation.getMatrix3();

        if (scale != Vec3::One)
            matrix.scale(scale);

        setMatrix3(matrix);
    }

    /**
     * Sets the value of this shader constant.
     */
    void setMatrix4(const Matrix4& m)
    {
        if (current_.matrix4 != m)
            setMatrix4Uncached(current_.matrix4 = m);
    }

    /**
     * Sets the value of this shader constant as the 4x4 matrix represented by the given position, quaternion and scale. See
     * setMatrix4(const Matrix4 &) for details.
     */
    void setMatrix4(const SimpleTransform& transform, const Vec3& scale = Vec3::One)
    {
        auto matrix = transform.getMatrix();

        if (scale != Vec3::One)
            matrix.scale(scale);

        setMatrix4(matrix);
    }

    /**
     * Sets the value of this shader constant to the inverse of the passed matrix.
     */
    void setMatrix4Inverse(const Matrix4& m)
    {
        auto inverse = Matrix4();
        m.getInverse(inverse);
        setMatrix4(inverse);
    }

    /**
     * Sets the value of this shader constant.
     */
    void setFloat(const ParameterArray& params) { setFloat(params[parameterLookup_].getFloat()); }

    /**
     * Sets the value of this shader constant.
     */
    void setFloat2(const Vec2& v) { setFloat2(v.x, v.y); }

    /**
     * Sets the value of this shader constant.
     */
    void setFloat2(const ParameterArray& params) { setFloat2(params[parameterLookup_].getVec2()); }

    /**
     * Sets the value of this shader constant.
     */
    void setFloat3(const Color& color) { setFloat3(color.r, color.g, color.b); }

    /**
     * Sets the value of this shader constant.
     */
    void setFloat3(const Vec3& v) { setFloat3(v.x, v.y, v.z); }

    /**
     * Sets the value of this shader constant. The parameter name passed to the constructor is used to look up the given
     * parameter array to find the parameter to read the value from.
     */
    void setFloat3(const ParameterArray& params) { setFloat3(params[parameterLookup_].getVec3()); }

    /**
     * Sets the value of this shader constant, all four components are set to the same value.
     */
    void setFloat4(float f) { setFloat4(f, f, f, f); }

    /**
     * Sets the value of this shader constant to the specified vector, the w component defaults to one but can be customized if
     * needed.
     */
    void setFloat4(const Vec2& xy, const Vec2& zw) { setFloat4(xy.x, xy.y, zw.x, zw.y); }

    /**
     * Sets the value of this shader constant to the specified vector, the w component defaults to one but can be customized if
     * needed.
     */
    void setFloat4(const Vec3& v, float w = 1.0f) { setFloat4(v.x, v.y, v.z, w); }

    /**
     * Sets the value of this shader constant to the specified color.
     */
    void setFloat4(const Color& color) { setFloat4(color.r, color.g, color.b, color.a); }

    /**
     * Sets the value of this shader constant. The parameter name passed to the constructor is used to look up the given
     * parameter array to find the parameter to read the value from.
     */
    void setFloat4(const ParameterArray& params) { setFloat4(params[parameterLookup_].getColor()); }

    /**
     * Sets the value of this shader constant.
     */
    void setInteger(const ParameterArray& params) { setInteger(params[parameterLookup_].getInteger()); }

    /**
     * Sets the value of this shader constant.
     */
    void setMatrix4(const ParameterArray& params)
    {
        auto matrix = params[parameterLookup_].getPointer<Matrix4>();

        assert(matrix && "Matrix4 parameter is null");

        setMatrix4(*matrix);
    }

private:

    const String parameterName_;
    const ParameterArray::Lookup parameterLookup_;

    // Holds the current hardware values for this constant, these are used to avoid unnecessary updates of hardware shader
    // constants
    struct
    {
        float f[4] = {};
        int i[4] = {};
        Matrix3 matrix3;
        Matrix4 matrix4;
    } current_;

    // The following methods must be implemented by a subclass for the specific shader language backend being used
    virtual void setFloatUncached(float f) = 0;
    virtual void setFloat2Uncached(float f0, float f1) = 0;
    virtual void setFloat3Uncached(float f0, float f1, float f2) = 0;
    virtual void setFloat4Uncached(float f0, float f1, float f2, float f3) = 0;
    virtual void setIntegerUncached(int i) = 0;
    virtual void setMatrix3Uncached(const Matrix3& m) = 0;
    virtual void setMatrix4Uncached(const Matrix4& m) = 0;
};

}
