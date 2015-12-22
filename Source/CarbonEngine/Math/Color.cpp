/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/Color.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

const Color Color::Zero(0.0f, 0.0f, 0.0f, 0.0f);
const Color Color::Black(0.0f, 0.0f, 0.0f, 1.0f);
const Color Color::White(1.0f, 1.0f, 1.0f, 1.0f);
const Color Color::Red(1.0f, 0.0f, 0.0f, 1.0f);
const Color Color::Green(0.0f, 1.0f, 0.0f, 1.0f);
const Color Color::Blue(0.0f, 0.0f, 1.0f, 1.0f);

void Color::clamp(float lower, float upper)
{
    r = Math::clamp(r, lower, upper);
    g = Math::clamp(g, lower, upper);
    b = Math::clamp(b, lower, upper);
    a = Math::clamp(a, lower, upper);
}

#ifdef WINDOWS

COLORREF Color::toCOLORREF() const
{
    return RGB(r * 255.0f, g * 255.0f, b * 255.0f);
}

#endif

Color Color::normalized() const
{
    auto largest = std::max(std::max(r, g), b);

    auto f = 1.0f / largest;

    return {r * f, g * f, b * f, a};
}

Color Color::random()
{
    return {Math::random(0.0f, 1.0f), Math::random(0.0f, 1.0f), Math::random(0.0f, 1.0f), Math::random(0.0f, 1.0f)};
}

Color Color::randomRGB()
{
    return {Math::random(0.0f, 1.0f), Math::random(0.0f, 1.0f), Math::random(0.0f, 1.0f), 1.0f};
}

}
