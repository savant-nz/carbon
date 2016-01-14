/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAYA_EXPORTER

#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/Runnable.h"
#include "CarbonEngine/Exporters/ExporterStrings.h"
#include "CarbonEngine/Exporters/Maya/MayaHelper.h"
#include "CarbonEngine/Exporters/ProgressDialog.h"
#include "CarbonEngine/Scene/SkeletalAnimation.h"

namespace Carbon
{

namespace Maya
{

class SkeletalAnimationExportRunner : public Runnable
{
public:

    SkeletalAnimationExportRunner(const UnicodeString& filename) : filename_(filename) {}

    Vector<SkeletalAnimation::BoneAnimation> boneAnimations;

    bool run() override
    {
        auto animation = SkeletalAnimation();

        // Set the output frame rate
        animation.setFrameRate(float(1.0 / MTime(1.0, MTime::uiUnit()).as(MTime::kSeconds)));
        LOG_INFO << "Animation frame rate: " << animation.getFrameRate();

        // Export animation
        if (!exportBoneAnimations())
        {
            if (!isCancelled())
                LOG_ERROR << "Failed exporting bones";
            return false;
        }

        // Put exported data into the skeletal mesh
        if (!animation.setBoneAnimations(boneAnimations))
        {
            LOG_INFO << "Failed setting up Animation class";
            return false;
        }

        // Save skeletal animation file
        if (!animation.save(FileSystem::LocalFilePrefix + filename_))
        {
            LOG_INFO << "Failed saving file";
            return false;
        }

        return true;
    }

    bool exportBoneAnimations()
    {
        auto oldTime = MAnimControl::currentTime();

        try
        {
            // Get animation start and end times
            auto startTime = MAnimControl::minTime();
            auto endTime = MAnimControl::maxTime();
            LOG_INFO << "Exporting from frame " << startTime.as(MTime::uiUnit()) << " to frame " << endTime.as(MTime::uiUnit());

            // Calculate start and end frames
            auto startFrame = int(startTime.value());
            auto endFrame = int(endTime.value());

            // Calculate frame count
            auto frameCount = endFrame - startFrame;
            LOG_INFO << "Output frame count: " << frameCount;

            // Get the joints to export
            auto joints = MDagPathArray();
            Helper::getExportObjects(joints, MFn::kJoint);

            // Iterate through all joints
            for (auto i = 0U; i < joints.length(); i++)
            {
                MFnIkJoint fnJoint(joints[i]);

                beginTask(fnJoint.partialPathName().asChar(), 100.0f / float(joints.length()));

                // Animation for this bone
                boneAnimations.append(SkeletalAnimation::BoneAnimation());
                boneAnimations.back().boneName = fnJoint.partialPathName().asChar();
                boneAnimations.back().frames.resize(frameCount);
                for (auto j = startFrame; j < endFrame; j++)
                {
                    MAnimControl::setCurrentTime(MTime(j, MTime::uiUnit()));

                    auto& bt = boneAnimations.back().frames[j - startFrame];

                    // Get joint rotation and translation
                    auto orientation = MQuaternion();
                    auto rotation = MQuaternion();
                    fnJoint.getOrientation(orientation);
                    fnJoint.getRotation(rotation);
                    bt.setOrientation(MQuaternionToQuaternion(rotation * orientation).getInverse());
                    bt.setPosition(MVectorToVec3(fnJoint.translation(MSpace::kTransform)));

                    if (setTaskProgress(j - startFrame + 1, frameCount))
                        throw Exception();
                }

                endTask();

                LOG_INFO << "Exported bone: " << boneAnimations.back().boneName;
            }

            MAnimControl::setCurrentTime(oldTime);

            return true;
        }
        catch (const Exception& e)
        {
            if (e.get().length())
                LOG_ERROR_WITHOUT_CALLER << e;

            MAnimControl::setCurrentTime(oldTime);

            return false;
        }
    }

private:

    UnicodeString filename_;
};

class SkeletalAnimationExporter : public MPxFileTranslator
{
public:

    bool canBeOpened() const override { return true; }
    bool haveReadMethod() const override { return false; }
    bool haveWriteMethod() const override { return true; }

    MString defaultExtension() const override { return toMString(SkeletalAnimation::SkeletalAnimationExtension.substr(1)); }

    MString filter() const override { return toMString("*" + SkeletalAnimation::SkeletalAnimationExtension); }

    MPxFileTranslator::MFileKind identifyFile(const MFileObject& fileName, const char* buffer, short size) const override
    {
        if (MStringToString(fileName.name()).asLower().endsWith(SkeletalAnimation::SkeletalAnimationExtension))
            return kIsMyFileType;

        return kNotMyFileType;
    }

    MStatus writer(const MFileObject& file, const MString& optionsString, MPxFileTranslator::FileAccessMode mode) override
    {
        onlyExportSelected = (mode == kExportActiveAccessMode);

        Globals::initializeEngine(getMayaClientName());

        auto runner = SkeletalAnimationExportRunner(MStringToString(file.fullName()));
        ProgressDialog(SkeletalAnimationExporterTitle).show(runner, M3dView::applicationShell());

        Globals::uninitializeEngine();

        return MS::kSuccess;
    }
};

void* createSkeletalAnimationExporter()
{
    return new SkeletalAnimationExporter;
}

}

}

#endif
