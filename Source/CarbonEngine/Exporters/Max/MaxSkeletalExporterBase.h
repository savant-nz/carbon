/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Exporters/Max/MaxPlugin.h"
#include "CarbonEngine/Scene/SkeletalMesh.h"

namespace Carbon
{

namespace Max
{

/**
 * Helper class for iterating through the skeletal physiques and skins in a scene and exporting data from them.
 */
class SkeletalExporterBase
{
protected:

    virtual ~SkeletalExporterBase() {}

    /**
     * This is called for each physique found by SkeletalExporterBase::exportData(). Should be implemented by subclasses to get
     * out the data they are interested in. This will be called for every physique found, even if only a subset of physiques are
     * selected and the 'Export Selected' option os chosen, in this case the \a doExport parameter describes whether or not the
     * data in this physique should actually be exported.
     */
    virtual bool exportPhysique(INode* node, IPhysiqueExport* phy, IPhyContextExport* mcExport, bool doExport) = 0;

    /**
     * Same as SkeletalExporterBase::exportPhysique() but for ISkin nodes. The skin modifier for the node is disabled before
     * this method is called and then re-enabled afterwards.
     */
    virtual bool exportSkin(INode* node, ISkin* skin, ISkinContextData* skinContext, bool doExport) = 0;

    /**
     * Looks through the scene for physiques and skins. If only physiques are found then SkeletalExporterBase::exportPhysique()
     * is called for each one of them and likewise for skins. If both physiques and skins are found then a warning is logged and
     * only the physiques will be exported. This is to avoid potential confusion from trying to export both physiques and skins
     * at the same time.
     */
    bool exportData(Runnable& r);

    /**
     * If the given node is a bone then it will be added to the bones list and a bone index returned. Returns -1 on failure.
     */
    int findOrAddBone(INode* node);

    Vector<SkeletalMesh::Bone> bones_;
    Vector<INode*> boneNodes_;

private:

    // Searches a node for a modifier of the given class, returns null if one is not found
    static Modifier* findModifier(INode* node, const Class_ID& classID);

    struct ExportNode
    {
        INode* node = nullptr;
        Modifier* modifier = nullptr;
        bool doExport = false;

        ExportNode(INode* node, Modifier* modifier, bool doExport) : node(node), modifier(modifier), doExport(doExport) {}
    };

    // Gets all the physique and skin nodes in the scene
    static void gather(INode* node, Vector<ExportNode>& physiqueNodes, Vector<ExportNode>& skinNodes);

    // Returns whether the given node is a bone. This works for both standard Max bones and Character Studio bones.
    bool isBone(INode* node);
};

}

}
