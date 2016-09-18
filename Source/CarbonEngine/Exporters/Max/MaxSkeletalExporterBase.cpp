/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAX_EXPORTER

#include "CarbonEngine/Exporters/Max/MaxSkeletalExporterBase.h"

namespace Carbon
{

namespace Max
{

bool SkeletalExporterBase::exportData(Runnable& r)
{
    // Gather all the physique and skin nodes
    auto physiqueNodes = Vector<ExportNode>();
    auto skinNodes = Vector<ExportNode>();
    gather(ip->GetRootNode(), physiqueNodes, skinNodes);

    // Check that we got some nodes to export
    if (physiqueNodes.empty() && skinNodes.empty())
    {
        LOG_ERROR_WITHOUT_CALLER << "Did not find any physiques or skins to export";
        return false;
    }

    // Warn if both physiques and skins are found
    if (physiqueNodes.size() && skinNodes.size())
    {
        LOG_WARNING_WITHOUT_CALLER
            << "Both Character Studio Physique and native ISkin nodes were found, ignoring ISkins";
        skinNodes.clear();
    }

    // Log the number of nodes found to export from
    if (physiqueNodes.size())
    {
        LOG_INFO << "Found " << physiqueNodes.size() << " physique node" << (physiqueNodes.size() == 1 ? "" : "s");

        // Go through physique nodes
        for (auto& physiqueNode : physiqueNodes)
        {
            auto result = true;

            r.beginTask("", 100.0f / float(physiqueNodes.size()));

            // Create a PhysiqueExport interface for the physique modifier
            auto phy = static_cast<IPhysiqueExport*>(physiqueNode.modifier->GetInterface(I_PHYINTERFACE));
            if (phy)
            {
                // Create a ModContext export interface for this node of the physique modifier
                auto pcExport = static_cast<IPhyContextExport*>(phy->GetContextInterface(physiqueNode.node));
                if (pcExport)
                {
                    pcExport->ConvertToRigid(true);
                    pcExport->AllowBlending(true);

                    // Call physique export method for this physique
                    result = exportPhysique(physiqueNode.node, phy, pcExport, physiqueNode.doExport);

                    // Release export interface
                    phy->ReleaseContextInterface(pcExport);
                }

                // Release the physique interface
                physiqueNode.modifier->ReleaseInterface(I_PHYINTERFACE, phy);
            }

            r.endTask();

            if (!result)
                return false;
        }
    }
    else
    {
        LOG_INFO << "Found " << skinNodes.size() << " skin node" << (skinNodes.size() == 1 ? "" : "s") << " to export";

        // Go through skin nodes
        for (auto& skinNode : skinNodes)
        {
            auto result = true;

            r.beginTask("", 100.0f / float(skinNodes.size()));

            auto skin = reinterpret_cast<ISkin*>(skinNode.modifier->GetInterface(I_SKIN));
            if (skin)
            {
                auto skinContext = skin->GetContextInterface(skinNode.node);
                if (skinContext)
                {
                    // Disable the skin modifier, storing its current enabled state
                    auto isSkinModifierEnabled = (skinNode.modifier->IsEnabled() != 0);
                    if (isSkinModifierEnabled)
                        skinNode.modifier->DisableMod();

                    // Call skin export method for this skin
                    result = exportSkin(skinNode.node, skin, skinContext, skinNode.doExport);

                    // Restore the modifier's enabled state
                    if (isSkinModifierEnabled)
                        skinNode.modifier->EnableMod();
                }

                // Release the I_SKIN interface
                skinNode.modifier->ReleaseInterface(I_SKIN, skin);
            }

            r.endTask();

            if (!result)
                return false;
        }
    }

    return true;
}

bool SkeletalExporterBase::isBone(INode* node)
{
    // Root node is never a bone
    if (node->IsRootNode())
        return false;

    // Check for standard Max bone class
    auto os = node->EvalWorldState(0);
    if (os.obj->ClassID() == Class_ID(BONE_CLASS_ID, 0) || os.obj->ClassID() == Class_ID(BONE_OBJ_CLASSID) ||
        node->GetBoneNodeOnOff())
        return true;

    // Ignore dummy objects
    if (os.obj->ClassID() == Class_ID(DUMMY_CLASS_ID, 0))
        return false;

    auto c = node->GetTMController();

    // Check for Character Studio bone classes
    return c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID || c->ClassID() == BIPBODY_CONTROL_CLASS_ID;
}

int SkeletalExporterBase::findOrAddBone(INode* node)
{
    if (!node || node->IsRootNode() || !isBone(node))
        return -1;

    // Search the bones list
    auto boneName = String(node->GetName());
    for (auto i = 0U; i < bones_.size(); i++)
    {
        if (bones_[i].name == boneName || node == boneNodes_[i])
            return i;
    }

    // Bone not found in the list, so add it
    auto newBone = SkeletalMesh::Bone();
    newBone.name = boneName;
    newBone.parent = findOrAddBone(node->GetParentNode());

    // Use the bone's first frame transform as the bind pose, this can be overwritten by the exporters if they get out
    // the actual bind pose
    auto localTransform = ::Matrix3();
    if (newBone.parent == -1)
        localTransform = node->GetNodeTM(0);
    else
        localTransform = node->GetNodeTM(0) * Inverse(node->GetParentTM(0));

    newBone.referenceRelative = maxMatrix3ToSimpleTransform(localTransform);

    bones_.append(newBone);
    boneNodes_.append(node);

    LOG_INFO << "Exported bone: '" << boneName << "'";

    if (bones_.size() > SkeletalMesh::MaximumBoneCount)
    {
        LOG_ERROR << "Maximum bone count of " << SkeletalMesh::MaximumBoneCount << " exceeded";
        return -1;
    }

    // Return index of the new bone
    return bones_.size() - 1;
}

Modifier* SkeletalExporterBase::findModifier(INode* node, const Class_ID& classID)
{
    auto object = node->GetObjectRef();

    while (object && object->SuperClassID() == GEN_DERIVOB_CLASS_ID)
    {
        auto derivedObject = static_cast<IDerivedObject*>(object);

        // Iterate over all entries of the modifier stack.
        for (auto i = 0; i < derivedObject->NumModifiers(); i++)
        {
            // If this modifier is a physique then return it
            auto modifier = derivedObject->GetModifier(i);
            if (modifier->ClassID() == classID)
                return modifier;
        }

        object = derivedObject->GetObjRef();
    }

    return nullptr;
}

void SkeletalExporterBase::gather(INode* node, Vector<ExportNode>& physiqueNodes, Vector<ExportNode>& skinNodes)
{
    if (!node)
        return;

    auto doExport = !onlyExportSelected || (onlyExportSelected && node->Selected());

    // Look for a physique modifier on this node
    auto modifier = findModifier(node, Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B));
    if (modifier)
        physiqueNodes.emplace(node, modifier, doExport);

    // Look for a skin modifier on this node
    modifier = findModifier(node, SKIN_CLASSID);
    if (modifier)
        skinNodes.emplace(node, modifier, doExport);

    // Go through child nodes
    for (auto i = 0; i < node->NumberOfChildren(); i++)
        gather(node->GetChildNode(i), physiqueNodes, skinNodes);
}

}

}

#endif
