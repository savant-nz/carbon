/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Graphics/States/StateTypes.h"

namespace Carbon
{

namespace States
{

CARBON_API BlendFactor convertBlendFactorStringToEnum(const String& blendFactor)
{
    auto s = blendFactor.asLower();

    if (s == "zero")
        return Zero;
    if (s == "one")
        return One;
    if (s == "sourcecolor")
        return SourceColor;
    if (s == "oneminussourcecolor")
        return OneMinusSourceColor;
    if (s == "destinationcolor")
        return DestinationColor;
    if (s == "oneminusdestinationcolor")
        return OneMinusDestinationColor;
    if (s == "sourcealpha")
        return SourceAlpha;
    if (s == "oneminussourcealpha")
        return OneMinusSourceAlpha;
    if (s == "destinationalpha")
        return DestinationAlpha;
    if (s == "oneminusdestinationalpha")
        return OneMinusDestinationAlpha;

    LOG_ERROR << "Invalid blend factor: " << blendFactor;

    return One;
}

CARBON_API CompareFunction convertCompareFunctionStringToEnum(const String& compareFunction)
{
    auto s = compareFunction.asLower();

    if (s == "less")
        return CompareLess;
    if (s == "lessequal")
        return CompareLessEqual;
    if (s == "equal")
        return CompareEqual;
    if (s == "greater")
        return CompareGreater;
    if (s == "notequal")
        return CompareNotEqual;
    if (s == "greaterequal")
        return CompareGreaterEqual;

    LOG_ERROR << "Invalid compare function: " << compareFunction;

    return CompareEqual;
}

CARBON_API UnicodeString& operator<<(UnicodeString& s, BlendFactor blendFactor)
{
    switch (blendFactor)
    {
        case Zero:
            return s << "Zero";
        case One:
            return s << "One";
        case SourceColor:
            return s << "SourceColor";
        case OneMinusSourceColor:
            return s << "OneMinusSourceColor";
        case DestinationColor:
            return s << "DestinationColor";
        case OneMinusDestinationColor:
            return s << "OneMinusDestinationColor";
        case SourceAlpha:
            return s << "SourceAlpha";
        case OneMinusSourceAlpha:
            return s << "OneMinusSourceAlpha";
        case DestinationAlpha:
            return s << "DestinationAlpha";
        case OneMinusDestinationAlpha:
            return s << "OneMinusDestinationAlpha";
    };

    return s;
}

CARBON_API UnicodeString& operator<<(UnicodeString& s, CompareFunction compareFunction)
{
    switch (compareFunction)
    {
        case CompareLess:
            return s << "<";
        case CompareLessEqual:
            return s << "<=";
        case CompareEqual:
            return s << "==";
        case CompareGreater:
            return s << ">";
        case CompareNotEqual:
            return s << "!=";
        case CompareGreaterEqual:
            return s << ">=";
    }

    return s;
}

CARBON_API UnicodeString& operator<<(UnicodeString& s, StencilBufferOperation stencilBufferOperation)
{
    switch (stencilBufferOperation)
    {
        case KeepStencilBufferValue:
            return s << "KeepStencilBufferValue";
        case ZeroStencilBufferValue:
            return s << "ZeroStencilBufferValue";
        case ReplaceStencilBufferValueWithReferenceValue:
            return s << "ReplaceStencilBufferValueWithReferenceValue";
        case IncrementStencilBufferValue:
            return s << "IncrementStencilBufferValue";
        case DecrementStencilBufferValue:
            return s << "DecrementStencilBufferValue";
        case IncrementStencilBufferValueAllowingWrapAround:
            return s << "IncrementStencilBufferValueAllowingWrapAround";
        case DecrementStencilBufferValueAllowingWrapAround:
            return s << "DecrementStencilBufferValueAllowingWrapAround";
        case InvertStencilBufferValue:
            return s << "InvertStencilBufferValue";
    }

    return s;
}

}

}
