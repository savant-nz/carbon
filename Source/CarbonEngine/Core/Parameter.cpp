/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/Parameter.h"

namespace Carbon
{

const Parameter Parameter::Empty;

Parameter::Parameter(const Parameter& other)
    : boolean_(other.boolean_),
      integer_(other.integer_),
      float4_(other.float4_),
      string_(other.string_),
      pointer_(other.pointer_),
      masterValue_(nullptr)
{
    if (other.masterValue_ == &other.boolean_)
        masterValue_ = &boolean_;
    else if (other.masterValue_ == &other.integer_)
        masterValue_ = &integer_;
    else if (other.masterValue_ == &other.float4_)
        masterValue_ = &float4_;
    else if (other.masterValue_ == &other.string_)
        masterValue_ = &string_;
    else if (other.masterValue_ == &other.pointer_)
        masterValue_ = &pointer_;
    else
    {
        assert(false && "Tried to copy a corrupted parameter");

        masterValue_ = &string_;
    }
}

void swap(Parameter& first, Parameter& second)
{
    auto newFirstMasterValue = pointer_to<Parameter::CachedValueBase>::type();
    auto newSecondMasterValue = pointer_to<Parameter::CachedValueBase>::type();

#define SWAP_CACHED_VALUE(member)              \
    std::swap(first.member, second.member);    \
    if (first.masterValue_ == &first.member)   \
        newSecondMasterValue = &second.member; \
    if (second.masterValue_ == &second.member) \
        newFirstMasterValue = &first.member;

    SWAP_CACHED_VALUE(boolean_);
    SWAP_CACHED_VALUE(integer_);
    SWAP_CACHED_VALUE(float4_);
    SWAP_CACHED_VALUE(string_);
    SWAP_CACHED_VALUE(pointer_);

#undef SWAP_CACHED_VALUE

    assert(newFirstMasterValue && newSecondMasterValue);

    first.masterValue_ = newFirstMasterValue;
    second.masterValue_ = newSecondMasterValue;
}

Parameter::Type Parameter::getTypeFromString(const String& s)
{
    auto lower = s.asLower();

    if (lower == "boolean")
        return BooleanParameter;
    if (lower == "integer")
        return IntegerParameter;
    if (lower == "float")
        return FloatParameter;
    if (lower == "vec2")
        return Vec2Parameter;
    if (lower == "vec3")
        return Vec3Parameter;
    if (lower == "quaternion")
        return QuaternionParameter;
    if (lower == "color")
        return ColorParameter;
    if (lower == "float4")
        return Float4Parameter;
    if (lower == "string")
        return StringParameter;
    if (lower == "pointer")
        return PointerParameter;

    return NullParameter;
}

const ParameterArray::Lookup Parameter::diffuseMap(Parameter::getHiddenParameterName("diffuseMap"));
const ParameterArray::Lookup Parameter::normalMap(Parameter::getHiddenParameterName("normalMap"));
const ParameterArray::Lookup Parameter::glossMap(Parameter::getHiddenParameterName("glossMap"));
const ParameterArray::Lookup Parameter::lightMap(Parameter::getHiddenParameterName("lightMap"));
const ParameterArray::Lookup Parameter::heightfieldTexture(Parameter::getHiddenParameterName("heightfieldTexture"));
const ParameterArray::Lookup Parameter::baseMap(Parameter::getHiddenParameterName("baseMap"));
const ParameterArray::Lookup Parameter::detailMap(Parameter::getHiddenParameterName("detailMap"));
const ParameterArray::Lookup Parameter::normalAndHeightMap(Parameter::getHiddenParameterName("normalAndHeightMap"));
const ParameterArray::Lookup Parameter::edgeLookupMap(Parameter::getHiddenParameterName("edgeLookupMap"));
const ParameterArray::Lookup Parameter::opacityMap(Parameter::getHiddenParameterName("opacityMap"));
const ParameterArray::Lookup Parameter::specularShiftMap(Parameter::getHiddenParameterName("specularShiftMap"));
const ParameterArray::Lookup Parameter::specularNoiseMap(Parameter::getHiddenParameterName("specularNoiseMap"));
const ParameterArray::Lookup Parameter::shadowMap(Parameter::getHiddenParameterName("shadowMap"));
const ParameterArray::Lookup Parameter::reflectanceMap(Parameter::getHiddenParameterName("reflectanceMap"));
const ParameterArray::Lookup Parameter::inputTexture(Parameter::getHiddenParameterName("inputTexture"));
const ParameterArray::Lookup Parameter::depthTexture(Parameter::getHiddenParameterName("depthTexture"));
const ParameterArray::Lookup Parameter::addTexture(Parameter::getHiddenParameterName("addTexture"));

const ParameterArray::Lookup Parameter::scaleAndOffset("scaleAndOffset");
const ParameterArray::Lookup Parameter::isLightingAllowed("isLightingAllowed");
const ParameterArray::Lookup Parameter::lightColor("lightColor");
const ParameterArray::Lookup Parameter::lightDirection("lightDirection");
const ParameterArray::Lookup Parameter::lightPosition("lightPosition");
const ParameterArray::Lookup Parameter::lightOrientation("lightOrientation");
const ParameterArray::Lookup Parameter::lightRadius("lightRadius");
const ParameterArray::Lookup Parameter::lightAmbient("lightAmbient");
const ParameterArray::Lookup Parameter::lightViewProjectionMatrix("lightViewProjectionMatrix");
const ParameterArray::Lookup Parameter::color("color");
const ParameterArray::Lookup Parameter::diffuseColor("diffuseColor");
const ParameterArray::Lookup Parameter::isSpecularEnabled("isSpecularEnabled");
const ParameterArray::Lookup Parameter::specularIntensity("specularIntensity");
const ParameterArray::Lookup Parameter::specularColor("specularColor");
const ParameterArray::Lookup Parameter::specularExponent("specularExponent");
const ParameterArray::Lookup Parameter::scale("scale");
const ParameterArray::Lookup Parameter::scales("scales");
const ParameterArray::Lookup Parameter::finalScale("finalScale");
const ParameterArray::Lookup Parameter::blurType("blurType");
const ParameterArray::Lookup Parameter::exposure("exposure");
const ParameterArray::Lookup Parameter::addTextureFactor("addTextureFactor");
const ParameterArray::Lookup Parameter::turbidity("turbidity");
const ParameterArray::Lookup Parameter::rayleighCoefficient("rayleighCoefficient");
const ParameterArray::Lookup Parameter::mieCoefficient("mieCoefficient");
const ParameterArray::Lookup Parameter::g("g");
const ParameterArray::Lookup Parameter::clipmapSize("clipmapSize");
const ParameterArray::Lookup Parameter::clipmapOrigin("clipmapOrigin");
const ParameterArray::Lookup Parameter::clipmapCameraPosition("clipmapCameraPosition");
const ParameterArray::Lookup Parameter::bloomFactor("bloomFactor");
const ParameterArray::Lookup Parameter::blurStandardDeviation("blurStandardDeviation");
const ParameterArray::Lookup Parameter::easing("easing");
const ParameterArray::Lookup Parameter::depthWrite("depthWrite");
const ParameterArray::Lookup Parameter::blend("blend");
const ParameterArray::Lookup Parameter::blendSourceFactor(".blendSourceFactor");
const ParameterArray::Lookup Parameter::blendDestinationFactor(".blendDestinationFactor");
const ParameterArray::Lookup Parameter::minimumConeAngle("minimumConeAngle");
const ParameterArray::Lookup Parameter::maximumConeAngle("maximumConeAngle");
const ParameterArray::Lookup Parameter::projectionCubemap("projectionCubemap");
const ParameterArray::Lookup Parameter::projectionTexture("projectionTexture");
const ParameterArray::Lookup Parameter::tilingFactor("tilingFactor");
const ParameterArray::Lookup Parameter::reflectionDistortion("reflectionDistortion");
const ParameterArray::Lookup Parameter::refractionDistortion("refractionDistortion");
const ParameterArray::Lookup Parameter::boneCount("boneCount");
const ParameterArray::Lookup Parameter::boneTransforms("boneTransforms");
const ParameterArray::Lookup Parameter::weightsPerVertex("weightsPerVertex");
const ParameterArray::Lookup Parameter::useVertexColor("useVertexColor");
const ParameterArray::Lookup Parameter::stereo("stereo");
const ParameterArray::Lookup Parameter::distortionCoefficients("distortionCoefficients");
const ParameterArray::Lookup Parameter::lensCenter("lensCenter");
const ParameterArray::Lookup Parameter::chromaticAberration("chromaticAberration");

}
