/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAX_EXPORTER

#include "CarbonEngine/Core/BuildInfo.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Exporters/ProgressDialog.h"
#include "CarbonEngine/Exporters/ExporterStrings.h"
#include "CarbonEngine/exporters/Max/MaxPlugin.h"
#include "CarbonEngine/exporters/Max/MaxSkeletalExporterBase.h"
#include "CarbonEngine/Scene/SkeletalAnimation.h"
#include "CarbonEngine/Scene/SkeletalMesh.h"

namespace Carbon
{

namespace Max
{

class SkeletalAnimationExportRunner : public Runnable, public SkeletalExporterBase
{
public:

    SkeletalAnimationExportRunner(const UnicodeString& filename) : filename_(filename) {}

    Vector<SkeletalAnimation::BoneAnimation> boneAnimations;
    int startFrame = 0;
    int endFrame = 0;

    bool exportPhysique(INode* node, IPhysiqueExport* phy, IPhyContextExport* mcExport, bool doExport)
    {
        if (!doExport)
            return true;

        auto vertexCount = mcExport->GetNumberVertices();

        for (auto i = 0; i < vertexCount; i++)
        {
            auto vertexInterface = mcExport->GetVertexInterface(i);

            if (vertexInterface->GetVertexType() & BLENDED_TYPE)
            {
                // This vertex is a weighted blend of a number of bones
                auto blendedVertex = static_cast<IPhyBlendedRigidVertex*>(vertexInterface);

                for (auto j = 0; j < blendedVertex->GetNumberNodes(); j++)
                {
                    if (findOrAddBone(blendedVertex->GetNode(j)) == -1)
                    {
                        LOG_ERROR_WITHOUT_CALLER << "Failed getting bone index for node: " << node->GetName();
                        mcExport->ReleaseVertexInterface(vertexInterface);
                        return false;
                    }
                }
            }
            else
            {
                // This vertex is fixed to a single bone
                auto rigidVertex = static_cast<IPhyRigidVertex*>(vertexInterface);

                if (findOrAddBone(rigidVertex->GetNode()) == -1)
                {
                    LOG_ERROR_WITHOUT_CALLER << "Failed getting bone index for node: " << node->GetName();
                    mcExport->ReleaseVertexInterface(vertexInterface);
                    return false;
                }
            }

            mcExport->ReleaseVertexInterface(vertexInterface);

            if (setTaskProgress(i + 1, vertexCount))
                return false;
        }

        return true;
    }

    bool exportSkin(INode* node, ISkin* skin, ISkinContextData* skinContext, bool doExport) override
    {
        if (!doExport)
            return true;

        auto vertexCount = skinContext->GetNumPoints();

        for (auto i = 0; i < vertexCount; i++)
        {
            auto boneCount = skinContext->GetNumAssignedBones(i);
            if (boneCount < 0)
                continue;

            for (auto j = 0; j < boneCount; j++)
            {
                auto boneIndex = skinContext->GetAssignedBone(i, j);
                if (boneIndex < 0)
                    continue;

                auto bone = skin->GetBone(boneIndex);

                if (findOrAddBone(bone) == -1)
                {
                    LOG_ERROR_WITHOUT_CALLER << "Failed getting bone index for node: " << node->GetName();
                    return false;
                }
            }

            if (setTaskProgress(i + 1, vertexCount))
                return false;
        }

        return true;
    }

    bool sampleBones()
    {
        for (auto i = 0U; i < boneNodes_.size(); i++)
        {
            beginTask(boneNodes_[i]->GetName(), 100.0f / float(boneNodes_.size()));

            INode* node = boneNodes_[i];

            // Export at all frames
            boneAnimations.emplace();
            auto& ba = boneAnimations.back();

            ba.boneName = node->GetName();
            ba.frames.resize(endFrame - startFrame);

            for (auto j = startFrame; j < endFrame; j++)
            {
                // Convert frame number into ticks
                auto ticks = j * GetTicksPerFrame();

                // Get bone transform at this frame
                auto nodeTransform = ::Matrix3();
                if (bones_[i].parent == -1)
                    nodeTransform = node->GetNodeTM(ticks);
                else
                    nodeTransform = node->GetNodeTM(ticks) * Inverse(node->GetParentTM(ticks));

                ba.frames[j - startFrame] = maxMatrix3ToSimpleTransform(nodeTransform);

                if (setTaskProgress(j + 1, ba.frames.size()))
                    return false;
            }

            endTask();
        }

        return true;
    }

    bool run() override
    {
        bones_.clear();
        boneNodes_.clear();
        boneAnimations.clear();

        auto animation = SkeletalAnimation();

        // Export the bones used by the physiques
        beginTask("Gathering physique bones", 10);
        if (!exportData(*this))
            return false;
        endTask();

        // Get the start and end frames
        startFrame = ip->GetAnimRange().Start() / GetTicksPerFrame();
        endFrame = ip->GetAnimRange().End() / GetTicksPerFrame();
        LOG_INFO << "Exporting from frame " << startFrame << " to frame " << endFrame;

        // Get frame rate
        animation.setFrameRate(float(GetFrameRate()));
        LOG_INFO << "Frame rate: " << animation.getFrameRate();

        // Sample the bone transforms at each frame
        beginTask("Sampling bone transforms", 90);
        if (!sampleBones())
            return false;
        endTask();

        // Set and validate bone animations
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

private:

    UnicodeString filename_;
};

class SkeletalAnimationExporter : public SceneExport
{
public:

    unsigned int Version() override { return BuildInfo::getVersion().asInteger(); }
    const char* ShortDesc() override { return SkeletalAnimationExporterFileType.cStr(); }
    const char* LongDesc() override { return ShortDesc(); }
    const char* AuthorName() override { return Globals::getDeveloperName().cStr(); }
    const char* CopyrightMessage() override { return ""; }
    const char* OtherMessage1() override { return ""; }
    const char* OtherMessage2() override { return ""; }
    void ShowAbout(HWND hWnd) override {}
    int ExtCount() override { return 1; }

    const char* Ext(int n) override
    {
        static auto const extension = A(Carbon::SkeletalAnimation::SkeletalAnimationExtension.substr(1));

        return extension.cStr();
    }

    BOOL SupportsOptions(int ext, DWORD options) override { return options == SCENE_EXPORT_SELECTED; }

    int DoExport(const char* name, ExpInterface* ei, Interface* pIp, BOOL suppressPrompts, DWORD options)
    {
        ip = pIp;
        onlyExportSelected = (options & SCENE_EXPORT_SELECTED);

        Globals::initializeEngine(getMaxClientName());

        auto runner = SkeletalAnimationExportRunner(fixMaxFilename(name, Ext(0)));
        ProgressDialog(SkeletalAnimationExporterTitle).show(runner, ip->GetMAXHWnd());

        Globals::uninitializeEngine();

        return 1;
    }
};

class SkeletalAnimationExporterClassDesc : public ClassDesc2
{
public:

    int IsPublic() override { return TRUE; }
    void* Create(BOOL loading) override { return new SkeletalAnimationExporter; }
    const char* ClassName() override { return "SkeletalAnimationExporterClassDesc"; }
    SClass_ID SuperClassID() override { return SCENE_EXPORT_CLASS_ID; }
    Class_ID ClassID() override { return Class_ID(0x68931526, 0x36773998); }
    const char* Category() override { return ""; }
    const char* InternalName() override { return "CarbonSkeletalAnimationExporter"; }
    HINSTANCE HInstance() override { return Globals::getHInstance(); }
};

ClassDesc* getSkeletalAnimationExporterClassDesc()
{
    static auto desc = SkeletalAnimationExporterClassDesc();
    return &desc;
}

}

}

#endif
