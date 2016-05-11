/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Image/Image.h"
#include "CarbonEngine/Math/MathCommon.h"

namespace Carbon
{

namespace sRGB
{

// This lookup converts an unsigned 8-bit value from sRGB into the linear colorspace
const auto toLinear = std::array<float, 256>{
    {0.0f,        0.000303527f, 0.000607054f, 0.000910581f, 0.00121411f, 0.00151763f, 0.00182116f, 0.00212469f, 0.00242822f,
     0.00273174f, 0.00303527f,  0.00334654f,  0.00367651f,  0.00402472f, 0.00439144f, 0.00477695f, 0.00518152f, 0.00560539f,
     0.00604883f, 0.00651209f,  0.00699541f,  0.00749903f,  0.00802319f, 0.00856812f, 0.00913406f, 0.00972122f, 0.0103298f,
     0.0109601f,  0.0116122f,   0.0122865f,   0.012983f,    0.0137021f,  0.0144438f,  0.0152085f,  0.0159963f,  0.0168074f,
     0.017642f,   0.0185002f,   0.0193824f,   0.0202886f,   0.021219f,   0.0221739f,  0.0231534f,  0.0241576f,  0.0251869f,
     0.0262412f,  0.0273209f,   0.028426f,    0.0295568f,   0.0307134f,  0.031896f,   0.0331048f,  0.0343398f,  0.0356013f,
     0.0368894f,  0.0382044f,   0.0395462f,   0.0409152f,   0.0423114f,  0.043735f,   0.0451862f,  0.0466651f,  0.0481718f,
     0.0497066f,  0.0512695f,   0.0528607f,   0.0544803f,   0.0561285f,  0.0578054f,  0.0595112f,  0.0612461f,  0.06301f,
     0.0648033f,  0.066626f,    0.0684782f,   0.0703601f,   0.0722719f,  0.0742136f,  0.0761854f,  0.0781874f,  0.0802198f,
     0.0822827f,  0.0843762f,   0.0865005f,   0.0886556f,   0.0908417f,  0.093059f,   0.0953075f,  0.0975874f,  0.0998987f,
     0.102242f,   0.104616f,    0.107023f,    0.109462f,    0.111932f,   0.114435f,   0.116971f,   0.119538f,   0.122139f,
     0.124772f,   0.127438f,    0.130136f,    0.132868f,    0.135633f,   0.138432f,   0.141263f,   0.144128f,   0.147027f,
     0.14996f,    0.152926f,    0.155926f,    0.158961f,    0.162029f,   0.165132f,   0.168269f,   0.171441f,   0.174647f,
     0.177888f,   0.181164f,    0.184475f,    0.187821f,    0.191202f,   0.194618f,   0.198069f,   0.201556f,   0.205079f,
     0.208637f,   0.212231f,    0.215861f,    0.219526f,    0.223228f,   0.226966f,   0.23074f,    0.234551f,   0.238398f,
     0.242281f,   0.246201f,    0.250158f,    0.254152f,    0.258183f,   0.262251f,   0.266356f,   0.270498f,   0.274677f,
     0.278894f,   0.283149f,    0.287441f,    0.291771f,    0.296138f,   0.300544f,   0.304987f,   0.309469f,   0.313989f,
     0.318547f,   0.323143f,    0.327778f,    0.332452f,    0.337164f,   0.341914f,   0.346704f,   0.351533f,   0.3564f,
     0.361307f,   0.366253f,    0.371238f,    0.376262f,    0.381326f,   0.38643f,    0.391573f,   0.396755f,   0.401978f,
     0.40724f,    0.412543f,    0.417885f,    0.423268f,    0.428691f,   0.434154f,   0.439657f,   0.445201f,   0.450786f,
     0.456411f,   0.462077f,    0.467784f,    0.473532f,    0.47932f,    0.48515f,    0.491021f,   0.496933f,   0.502887f,
     0.508881f,   0.514918f,    0.520996f,    0.527115f,    0.533276f,   0.53948f,    0.545725f,   0.552011f,   0.55834f,
     0.564712f,   0.571125f,    0.577581f,    0.584078f,    0.590619f,   0.597202f,   0.603827f,   0.610496f,   0.617207f,
     0.62396f,    0.630757f,    0.637597f,    0.64448f,     0.651406f,   0.658375f,   0.665387f,   0.672443f,   0.679543f,
     0.686685f,   0.693872f,    0.701102f,    0.708376f,    0.715694f,   0.723055f,   0.730461f,   0.737911f,   0.745404f,
     0.752942f,   0.760525f,    0.768151f,    0.775822f,    0.783538f,   0.791298f,   0.799103f,   0.806952f,   0.814847f,
     0.822786f,   0.83077f,     0.838799f,    0.846873f,    0.854993f,   0.863157f,   0.871367f,   0.879622f,   0.887923f,
     0.896269f,   0.904661f,    0.913099f,    0.921582f,    0.930111f,   0.938686f,   0.947307f,   0.955974f,   0.964686f,
     0.973445f,   0.982251f,    0.991102f,    1.0f}};

static byte_t fromLinear(float value)
{
    auto result = 0.0f;

    if (!std::isfinite(value))
        ;
    else if (value > 1.0f)
        result = 1.0f;
    else if (value < 0.0f)
        ;
    else if (value < 0.0031308f)
        result = value * 12.92f;
    else
        result = 1.055f * powf(value, (1.0f / 2.4f)) - 0.055f;

    return byte_t(floorf(255.0f * result + 0.5f));
}

}

Color Image::readSRGB8Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                            unsigned int y, unsigned int z)
{
    data += (width * height * z + width * y + x) * 3;

    return {sRGB::toLinear[data[0]], sRGB::toLinear[data[1]], sRGB::toLinear[data[2]], 1.0f};
}

void Image::writeSRGB8Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                            unsigned int y, unsigned int z, const Color& color)
{
    data += (width * height * z + width * y + x) * 3;

    data[0] = sRGB::fromLinear(color.r);
    data[1] = sRGB::fromLinear(color.g);
    data[2] = sRGB::fromLinear(color.b);
}

Color Image::readSRGBA8Pixel(const byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                             unsigned int y, unsigned int z)
{
    data += (width * height * z + width * y + x) * 4;

    return {sRGB::toLinear[data[0]], sRGB::toLinear[data[1]], sRGB::toLinear[data[2]], Math::byteToFloat(data[3])};
}

void Image::writeSRGBA8Pixel(byte_t* data, unsigned int width, unsigned int height, unsigned int depth, unsigned int x,
                             unsigned int y, unsigned int z, const Color& color)
{
    data += (width * height * z + width * y + x) * 4;

    data[0] = sRGB::fromLinear(color.r);
    data[1] = sRGB::fromLinear(color.g);
    data[2] = sRGB::fromLinear(color.b);
    data[3] = byte_t(color.a * 255.0f);
}

}
