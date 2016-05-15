/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_OCULUSRIFT

#include "CarbonEngine/Platform/Windows/PlatformWindows.h"

namespace Carbon
{

bool PlatformWindows::isOculusRiftSupported() const
{
    return true;
}

bool PlatformWindows::isOculusRiftPresent() const
{
    return oculusRiftHMDDesc_.Type != ovrHmd_None;
}

bool PlatformWindows::oculusRiftInitialize()
{
    if (!OVR_SUCCESS(ovr_Initialize(nullptr)))
        return false;

    // Try and create an HMD instance
    auto graphicsID = ovrGraphicsLuid();
    auto result = ovr_Create(&oculusRiftSession_, &graphicsID);
    if (OVR_FAILURE(result))
    {
        LOG_ERROR << "Failed creating Oculus Rift session";
        return false;
    }

    LOG_INFO << "Oculus Rift session created";

    return true;
}

void PlatformWindows::oculusRiftShutdown()
{
    if (oculusRiftSession_)
    {
        ovr_Destroy(oculusRiftSession_);
        oculusRiftSession_ = nullptr;
    }

    ovr_Shutdown();
}

void PlatformWindows::oculusRiftUpdate()
{
    if (!oculusRiftSession_)
        return;

    oculusRiftHMDDesc_ = ovr_GetHmdDesc(oculusRiftSession_);

    if (!isOculusRiftPresent())
        return;

    // Store the offscreen eye texture size if not already retrieved
    if (oculusRiftEyeTextureDimensions_ == Rect::Zero)
    {
        auto leftEyeSize =
            ovr_GetFovTextureSize(oculusRiftSession_, ovrEye_Left, oculusRiftHMDDesc_.DefaultEyeFov[ovrEye_Left], 1.0f);
        auto rightEyeSize = ovr_GetFovTextureSize(oculusRiftSession_, ovrEye_Right,
                                                  oculusRiftHMDDesc_.DefaultEyeFov[ovrEye_Right], 1.0f);

        if (leftEyeSize.w != rightEyeSize.w || leftEyeSize.h != rightEyeSize.h)
            LOG_WARNING << "Different texture sizes for left and right eyes were requested but this is not supported";

        oculusRiftEyeTextureDimensions_.setRight(float(leftEyeSize.w));
        oculusRiftEyeTextureDimensions_.setTop(float(leftEyeSize.h));
    }

    auto frameTiming = ovr_GetPredictedDisplayTime(oculusRiftSession_, oculusRiftFrameIndex_);
    auto sensorSampleTime = ovr_GetTimeInSeconds();

    auto trackingState = ovr_GetTrackingState(oculusRiftSession_, frameTiming, true);
    auto& headPose = trackingState.HeadPose.ThePose;

    auto eyeRenderDesc = std::array<ovrEyeRenderDesc, 2>{
        {ovr_GetRenderDesc(oculusRiftSession_, ovrEye_Left, oculusRiftHMDDesc_.DefaultEyeFov[0]),
         ovr_GetRenderDesc(oculusRiftSession_, ovrEye_Right, oculusRiftHMDDesc_.DefaultEyeFov[1])}};

    auto viewOffset = std::array<ovrVector3f, 2>{
        {eyeRenderDesc[ovrEye_Left].HmdToEyeOffset, eyeRenderDesc[ovrEye_Right].HmdToEyeOffset}};
    auto eyePose = std::array<ovrPosef, 2>();

    ovr_CalcEyePoses(headPose, viewOffset.data(), eyePose.data());

    for (auto eye = 0U; eye < 2; eye++)
    {
        oculusRiftEyeTransforms_[eye].setPosition(
            {-eyePose[eye].Position.x, eyePose[eye].Position.y, -eyePose[eye].Position.z});

        // The swapping and negating of quaternion components makes the quaternion 'point' in the opposite direction
        oculusRiftEyeTransforms_[eye].setOrientation({-eyePose[eye].Orientation.z, -eyePose[eye].Orientation.w,
                                                      eyePose[eye].Orientation.x, -eyePose[eye].Orientation.y});
    }

    auto textureSwapChains = static_cast<OpenGL11&>(graphics()).getOculusRiftTextureSwapChains();
    if (textureSwapChains[ovrEye_Left] && textureSwapChains[ovrEye_Right])
    {
        // Set up positional data
        auto viewScaleDesc = ovrViewScaleDesc();
        viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
        viewScaleDesc.HmdToEyeOffset[ovrEye_Left] = viewOffset[ovrEye_Left];
        viewScaleDesc.HmdToEyeOffset[ovrEye_Right] = viewOffset[ovrEye_Right];

        auto layer = ovrLayerEyeFov();
        layer.Header.Type = ovrLayerType_EyeFov;
        layer.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;

        for (auto eye = 0U; eye < 2; eye++)
        {
            ovr_CommitTextureSwapChain(oculusRiftSession_, textureSwapChains[eye]);
            layer.ColorTexture[eye] = textureSwapChains[eye];
            layer.Viewport[eye].Size.w = int(getOculusRiftTextureDimensions().getWidth());
            layer.Viewport[eye].Size.h = int(getOculusRiftTextureDimensions().getHeight());
            layer.Fov[eye] = oculusRiftHMDDesc_.DefaultEyeFov[eye];
            layer.RenderPose[eye] = eyePose[eye];
            layer.SensorSampleTime = sensorSampleTime;
        }

        auto layers = &layer.Header;
        auto result = ovr_SubmitFrame(oculusRiftSession_, oculusRiftFrameIndex_, &viewScaleDesc, &layers, 1);

        if (!OVR_SUCCESS(result))
            LOG_ERROR << "Failed submitting Oculus Rift frame";
    }

    oculusRiftFrameIndex_++;
}

Matrix4 PlatformWindows::getOculusRiftProjectionMatrixLeftEye(float nearPlaneDistance, float farPlaneDistance) const
{
    auto matrix =
        ovrMatrix4f_Projection(oculusRiftHMDDesc_.DefaultEyeFov[ovrEye_Left], nearPlaneDistance, farPlaneDistance, 0);

    return {matrix.M[0][0], matrix.M[1][0], matrix.M[2][0], matrix.M[3][0], matrix.M[0][1], matrix.M[1][1],
            matrix.M[2][1], matrix.M[3][1], matrix.M[0][2], matrix.M[1][2], matrix.M[2][2], matrix.M[3][2],
            matrix.M[0][3], matrix.M[1][3], matrix.M[2][3], matrix.M[3][3]};
}

Matrix4 PlatformWindows::getOculusRiftProjectionMatrixRightEye(float nearPlaneDistance, float farPlaneDistance) const
{
    auto matrix =
        ovrMatrix4f_Projection(oculusRiftHMDDesc_.DefaultEyeFov[ovrEye_Right], nearPlaneDistance, farPlaneDistance, 0);

    return {matrix.M[0][0], matrix.M[1][0], matrix.M[2][0], matrix.M[3][0], matrix.M[0][1], matrix.M[1][1],
            matrix.M[2][1], matrix.M[3][1], matrix.M[0][2], matrix.M[1][2], matrix.M[2][2], matrix.M[3][2],
            matrix.M[0][3], matrix.M[1][3], matrix.M[2][3], matrix.M[3][3]};
}

}

#endif
