/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Exporters/ExportInfo.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Scene/SkeletalAnimation.h"

namespace Carbon
{

const SkeletalAnimation SkeletalAnimation::Empty;

const UnicodeString SkeletalAnimation::SkeletalAnimationDirectory = "Animations/";
const UnicodeString SkeletalAnimation::SkeletalAnimationExtension = ".animation";

const auto SkeletalAnimationVersionInfo = VersionInfo(1, 3);
const auto SkeletalAnimationHeaderID = FileSystem::makeFourCC("canm");

void SkeletalAnimation::BoneAnimation::save(FileWriter& file) const
{
    file.write(boneName, frames);
}

void SkeletalAnimation::BoneAnimation::load(FileReader& file)
{
    file.read(boneName, frames);
}

void SkeletalAnimation::clear()
{
    name_.clear();
    frameRate_ = 15.0f;
    frameCount_ = 0;
    boneAnimations_.clear();
    isLoaded_ = false;
}

bool SkeletalAnimation::setBoneAnimations(const Vector<BoneAnimation>& boneAnimations)
{
    if (boneAnimations.empty())
    {
        LOG_ERROR << "No bones";
        return false;
    }

    auto frameCount = boneAnimations[0].frames.size();
    if (!frameCount)
    {
        LOG_ERROR << "No animation frames";
        return false;
    }

    for (auto i = 0U; i < boneAnimations.size(); i++)
    {
        // Check there is a bone name
        if (boneAnimations[i].boneName.length() == 0)
        {
            LOG_ERROR << "Bone has no name";
            return false;
        }

        // Check there are no duplicate bone names
        for (auto j = i + 1; j < boneAnimations.size(); j++)
        {
            if (boneAnimations[i].boneName == boneAnimations[j].boneName)
            {
                LOG_ERROR << "Duplicated bone name: " << boneAnimations[i].boneName;
                return false;
            }
        }

        // Check this bone has the correct number of frames
        if (boneAnimations[i].frames.size() != frameCount)
        {
            LOG_ERROR << "Bone has incorrect number of frames";
            return false;
        }
    }

    frameCount_ = frameCount;
    boneAnimations_ = boneAnimations;

    isLoaded_ = true;

    return true;
}

const Vector<SimpleTransform>& SkeletalAnimation::getAnimationFramesForBone(const String& bone) const
{
    for (auto& animation : boneAnimations_)
    {
        if (animation.boneName == bone)
            return animation.frames;
    }

    static auto empty = Vector<SimpleTransform>();

    return empty;
}

bool SkeletalAnimation::save(const UnicodeString& name)
{
    try
    {
        auto filename = name;
        if (!filename.startsWith(FileSystem::LocalFilePrefix))
            filename = SkeletalAnimationDirectory + filename + SkeletalAnimationExtension;

        auto file = FileWriter();
        fileSystem().open(filename, file);

        file.write(SkeletalAnimationHeaderID);
        file.beginVersionedSection(SkeletalAnimationVersionInfo);
        file.write(uint(frameRate_), frameCount_, boneAnimations_, ExportInfo::get(), frameRate_);
        file.endVersionedSection();

        file.close();

        LOG_INFO << "Saved animation - '" << name << "'";

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name << "' - " << e;

        return false;
    }
}

bool SkeletalAnimation::load(const String& name)
{
    try
    {
        name_ = name;

        // Open the file
        auto file = FileReader();
        fileSystem().open(SkeletalAnimationDirectory + name + SkeletalAnimationExtension, file);

        // Read header and check ID
        if (file.readFourCC() != SkeletalAnimationHeaderID)
            throw Exception("Not a skeletal animation file");

        auto loadedVersion = file.beginVersionedSection(SkeletalAnimationVersionInfo);

        auto frameRate = 0U;
        file.read(frameRate, frameCount_, boneAnimations_);
        frameRate_ = float(frameRate);

        // v1.1, export info
        auto exportInfo = ExportInfo();
        if (loadedVersion.getMinor() >= 1)
            file.read(exportInfo);

        // v1.3, frame rate stored as float
        if (loadedVersion.getMinor() >= 3)
            file.read(frameRate_);

        file.endVersionedSection();

        LOG_INFO << "Loaded skeletal animation - '" << name << "' - frame rate: " << frameRate_ << ", frames: " << frameCount_
                 << ", bones: " << boneAnimations_.size() << ", export info: " << exportInfo;

        isLoaded_ = true;

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name << "' - " << e;

        clear();
        name_ = name;

        return false;
    }
}

static auto skeletalAnimations = std::vector<std::unique_ptr<SkeletalAnimation>>();

const SkeletalAnimation* SkeletalAnimation::get(const String& name)
{
    if (!name.length())
        return &Empty;

    for (auto& animation : skeletalAnimations)
    {
        if (animation->getName() == name)
            return animation.get();
    }

    skeletalAnimations.emplace_back(new SkeletalAnimation);
    skeletalAnimations.back()->load(name);

    return skeletalAnimations.back().get();
}

static void clearSkeletalAnimations()
{
    skeletalAnimations.clear();
}
CARBON_REGISTER_SHUTDOWN_FUNCTION(clearSkeletalAnimations, 0)

}
