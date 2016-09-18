/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

namespace States
{

/**
 * The blending factors available when doing alpha blending.
 */
enum BlendFactor
{
    Zero,
    One,
    SourceColor,
    OneMinusSourceColor,
    DestinationColor,
    OneMinusDestinationColor,
    SourceAlpha,
    OneMinusSourceAlpha,
    DestinationAlpha,
    OneMinusDestinationAlpha
};

/**
 * Converts a human readable string into a blending factor, the string must be one of Zero, One, SourceColor,
 * OneMinusSourceColor, DestinationColor, OneMinusDestinationColor, SourceAlpha, OneMinusSourceAlpha, DestinationAlpha,
 * OneMinusDestinationAlpha. Case insensitive. If the string isn't recognized then an error is reported and One is
 * returned.
 */
extern CARBON_API BlendFactor convertBlendFactorStringToEnum(const String& blendFactor);

/**
 * Comparision functions available when doing depth testing and alpha testing, the name describes what comparison is
 * done on the incoming value to determine whether the test passes or fails.
 */
enum CompareFunction
{
    CompareLess,
    CompareLessEqual,
    CompareEqual,
    CompareGreater,
    CompareNotEqual,
    CompareGreaterEqual
};

/**
 * Converts a string to a CompareFunction enum value. Valid values are Less, LessEqual, Equal, Greater, NotEqual and
 * GreaterEqual. Case insensitive. If the string isn't recognied then an error is reported and CompareEqual is returned.
 */
extern CARBON_API CompareFunction convertCompareFunctionStringToEnum(const String& compareFunction);

/**
 * Available culling modes that can be used when rendering, see GraphicsInterface::setCullingMode() for details.
 */
enum CullingMode
{
    /**
     * No culling, both front and back faces will be rendered.
     */
    CullingDisabled,

    /**
     * Front faces will be culled.
     */
    CullFrontFaces,

    /**
     * Back faces will be culled, this is the most commonly used culling mode in normal rendering.
     */
    CullBackFaces
};

/**
 * Describes a standard blending state that consists of source and destination blending factors.
 */
class CARBON_API BlendFunctionSetup
{
public:

    BlendFunctionSetup() {}

    /**
     * Initializes this blend function setup.
     */
    BlendFunctionSetup(BlendFactor sourceFactor, BlendFactor destinationFactor)
        : sourceFactor_(sourceFactor), destinationFactor_(destinationFactor)
    {
    }

    /**
     * Returns the source blending factor, defaults to \a One.
     */
    BlendFactor getSourceFactor() const { return sourceFactor_; }

    /**
     * Returns the destination blending factor, defaults to \a Zero.
     */
    BlendFactor getDestinationFactor() const { return destinationFactor_; }

    /**
     * Inequality operator.
     */
    bool operator!=(const BlendFunctionSetup& other) const
    {
        return sourceFactor_ != other.sourceFactor_ || destinationFactor_ != other.destinationFactor_;
    }

    /**
     * Converts this blend function setup into a human-readable string.
     */
    operator UnicodeString() const { return UnicodeString() << sourceFactor_ << " " << destinationFactor_; }

private:

    BlendFactor sourceFactor_ = One;
    BlendFactor destinationFactor_ = Zero;
};

/**
 * Describes a stencil testing state, i.e. a compare function and a reference value and mask to use in the comparison.
 */
class CARBON_API StencilTestSetup
{
public:

    StencilTestSetup() {}

    /**
     * Initializes this stencil test setup.
     */
    StencilTestSetup(CompareFunction compareFunction, unsigned int referenceValue, unsigned int mask)
        : compareFunction_(compareFunction), referenceValue_(referenceValue), mask_(mask)
    {
    }

    /**
     * Returns the compare function to use, defaults to CompareEqual.
     */
    CompareFunction getCompareFunction() const { return compareFunction_; }

    /**
     * Returns the reference value to use in the stencil test comparison, the value in the stencil buffer is compared to
     * this reference value using the chosen comparison function and the result of that test determines whether the
     * stencil test passes or fails.
     */
    unsigned int getReferenceValue() const { return referenceValue_; }

    /**
     * Returns the mask that is bitwise ANDed with both the reference value and the value in the stencil buffer prior to
     * doing any comparison that may be specified by the compare function.
     */
    unsigned int getMask() const { return mask_; }

    /**
     * Inequality operator.
     */
    bool operator!=(const StencilTestSetup& other) const
    {
        return compareFunction_ != other.compareFunction_ || referenceValue_ != other.referenceValue_ ||
            mask_ != other.mask_;
    }

    /**
     * Converts this stencil test setup into a human-readable string.
     */
    operator UnicodeString() const { return UnicodeString() << compareFunction_ << " " << referenceValue_; }

private:

    CompareFunction compareFunction_ = CompareEqual;
    unsigned int referenceValue_ = 0;
    unsigned int mask_ = ~0U;
};

/**
 * Available stencil buffer operations that can be executed when rendering with stenciling enabled, see
 * States::StencilOperations for details.
 */
enum StencilBufferOperation
{
    /**
     * Leaves the current value in the stencil buffer unchanged.
     */
    KeepStencilBufferValue,

    /**
     * Replaces the current value in the stencil buffer with zero.
     */
    ZeroStencilBufferValue,

    /**
     * Replaces the current value in the stencil buffer with the reference value specified in the current stencil test
     * setup.
     */
    ReplaceStencilBufferValueWithReferenceValue,

    /**
     * Increments the the current value in the stencil buffer by one, clamping to the maximum value if integer overflow
     * occurs.
     */
    IncrementStencilBufferValue,

    /**
     * Decrements the the current value in the stencil buffer by one, clamping to the minimum value if integer underflow
     * occurs.
     */
    DecrementStencilBufferValue,

    /**
     * Increments the the current value in the stencil buffer by one, wrapping around to zero if integer overflow
     * occurs.
     */
    IncrementStencilBufferValueAllowingWrapAround,

    /**
     * Decrements the the current value in the stencil buffer by one, wrapping around to the maximum value if integer
     * underflow occurs.
     */
    DecrementStencilBufferValueAllowingWrapAround,

    /**
     * Bitwise inverts all bits in the stencil buffer.
     */
    InvertStencilBufferValue
};

/**
 * Describes a set of three stencil operations that specify what operations to take in the event that a fragment fails
 * the stencil test, passes the stencil test but fails the depth test, and passes both the stencil test and the depth
 * test.
 */
class CARBON_API StencilOperations
{
public:

    StencilOperations() {}

    /**
     * Initializes this set of stencil operations.
     */
    StencilOperations(StencilBufferOperation stencilTestFailOperation, StencilBufferOperation depthTestFailOperation,
                      StencilBufferOperation bothTestsPassOperation)
        : stencilTestFailOperation_(stencilTestFailOperation),
          depthTestFailOperation_(depthTestFailOperation),
          bothTestsPassOperation_(bothTestsPassOperation)
    {
    }

    /**
     * Returns the stencil buffer operation to carry out when a fragment fails the stencil test.
     */
    StencilBufferOperation getStencilTestFailOperation() const { return stencilTestFailOperation_; }

    /**
     * Returns the stencil buffer operation to carry out when a fragment passes the stencil test but fails the depth
     * test.
     */
    StencilBufferOperation getDepthTestFailOperation() const { return depthTestFailOperation_; }

    /**
     * Returns the stencil buffer operation to carry out when a fragment passes both the stencil test and the depth
     * test.
     */
    StencilBufferOperation getBothTestsPassOperation() const { return bothTestsPassOperation_; }

    /**
     * Inequality operator.
     */
    bool operator!=(const StencilOperations& other) const
    {
        return stencilTestFailOperation_ != other.stencilTestFailOperation_ ||
            depthTestFailOperation_ != other.depthTestFailOperation_ ||
            bothTestsPassOperation_ != other.bothTestsPassOperation_;
    }

    /**
     * Converts these stencil operations into a human-readable string.
     */
    operator UnicodeString() const
    {
        return UnicodeString() << stencilTestFailOperation_ << " " << depthTestFailOperation_ << " "
                               << bothTestsPassOperation_;
    }

private:

    StencilBufferOperation stencilTestFailOperation_ = KeepStencilBufferValue;
    StencilBufferOperation depthTestFailOperation_ = KeepStencilBufferValue;
    StencilBufferOperation bothTestsPassOperation_ = KeepStencilBufferValue;
};

// These functions allow the enumerations defined in this file to be streamed directly onto a string in a human-readable
// format
extern CARBON_API UnicodeString& operator<<(UnicodeString& s, BlendFactor blendFactor);
extern CARBON_API UnicodeString& operator<<(UnicodeString& s, CompareFunction compareFunction);
extern CARBON_API UnicodeString& operator<<(UnicodeString& s, StencilBufferOperation stencilBufferOperation);

}

}
