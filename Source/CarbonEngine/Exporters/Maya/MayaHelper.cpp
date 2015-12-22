/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"

#ifdef CARBON_INCLUDE_MAYA_EXPORTER

#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Exporters/Maya/MayaHelper.h"
#include "CarbonEngine/Geometry/TriangleArraySet.h"
#include "CarbonEngine/Render/EffectManager.h"
#include "CarbonEngine/Scene/Entity.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"

namespace Carbon
{

namespace Maya
{

bool Helper::isNodeVisible(const MFnDagNode& fnDagNode)
{
    auto status = MStatus();

    if (fnDagNode.isIntermediateObject())
        return false;

    auto visPlug = fnDagNode.findPlug("visibility", &status);
    if (status == MStatus::kFailure)
        return false;

    auto isVisible = bool();
    visPlug.getValue(isVisible);

    return isVisible;
}

void Helper::getExportObjects(MDagPathArray& dagPaths, MFn::Type type)
{
    if (onlyExportSelected)
    {
        // Get the active selection list
        auto selected = MSelectionList();
        MGlobal::getActiveSelectionList(selected);

        // Iterate through all selected items
        for (auto i = 0U; i < selected.length(); i++)
        {
            auto dagPath = MDagPath();
            auto comp = MObject();
            selected.getDagPath(i, dagPath, comp);

            // If the selected object is of the type we are looking for then add it to the output list
            if (dagPath.hasFn(type))
                dagPaths.append(dagPath);
            else if (dagPath.hasFn(MFn::kTransform))
            {
                // If the selected object is a transform then also check its children

                MFnTransform fn(dagPath);
                for (auto j = 0U; j < fn.childCount(); j++)
                {
                    auto child = fn.child(j);
                    if (child.hasFn(type))
                    {
                        MFnDagNode(child).getPath(dagPath);
                        dagPaths.append(dagPath);
                    }
                }
            }
        }
    }
    else
    {
        for (auto itDag = MItDag(MItDag::kDepthFirst, type); !itDag.isDone(); itDag.next())
        {
            auto dagPath = MDagPath();
            itDag.getPath(dagPath);
            dagPaths.append(dagPath);
        }
    }
}

bool Helper::isObjectSelected(MObject& obj)
{
    // Get the active selection list
    auto selected = MSelectionList();
    MGlobal::getActiveSelectionList(selected);

    // Iterate through all selected items
    for (auto i = 0U; i < selected.length(); i++)
    {
        auto o = MObject();
        selected.getDependNode(i, o);

        if (o == obj)
            return true;
        else if (o.hasFn(MFn::kTransform))
        {
            // If the selected object is a transform then also check its children

            MFnTransform fn(o);
            for (auto j = 0U; j < fn.childCount(); j++)
            {
                auto child = fn.child(j);
                if (child == obj)
                    return true;
            }
        }
    }

    return false;
}

bool Helper::getTransformAtDagPath(const MDagPath& dagPath, SimpleTransform& transform)
{
    auto status = MStatus();

    // Get the transform node for the dag path
    MFnTransform fnTransform(dagPath.transform(), &status);
    if (status != MS::kSuccess)
    {
        LOG_ERROR_WITHOUT_CALLER << "Failed getting transform node: " << MStringToString(status.errorString());
        return false;
    }

    // Get the transformation matrix for the transform node
    auto matrix = fnTransform.transformationMatrix(&status);
    if (status != MS::kSuccess)
    {
        LOG_ERROR_WITHOUT_CALLER << "Failed getting transformation matrix: " << MStringToString(status.errorString());
        return false;
    }

    transform = MMatrixToAffineTransform(matrix);

    return true;
}

bool Helper::getTransformAtDagPath(const MDagPath& dagPath, Entity& entity)
{
    auto transform = SimpleTransform();

    if (!getTransformAtDagPath(dagPath, transform))
        return false;

    entity.setWorldTransform(transform);

    return true;
}

String Helper::processMaterialName(const String& name)
{
    auto result = name;

    // Maya doesn't allow forward slashes in shader names, so two consecutive underscores are replaced with a forward slash here
    // to allow forward slashes in exported material names
    result.replace("__", "/");

    return result;
}

String Helper::getMaterialName(int polygonIndex, const MObjectArray& shaders, const MIntArray& indices)
{
    auto name = String();

    if (polygonIndex < int(indices.length()))
    {
        auto shaderIndex = indices[polygonIndex];

        if (shaderIndex != -1 && shaderIndex < int(shaders.length()))
        {
            auto materials = MPlugArray();
            MFnDependencyNode(shaders[shaderIndex]).findPlug("surfaceShader").connectedTo(materials, true, false);

            if (materials.length())
                name = processMaterialName(MFnDependencyNode(materials[0].node()).name().asChar());
        }
    }

    if (!name.length())
        name = MaterialManager::ExporterNoMaterialFallback;

    return name;
}

void Helper::exportAllMaterials(const TriangleArraySet& triangleSet, const UnicodeString& outputDirectory)
{
    effects().loadEffects(false);

    auto exportedMaterials = triangleSet.getMaterials();

    auto itLambert = MItDependencyNodes(MFn::kLambert);
    while (!itLambert.isDone())
    {
        MFnLambertShader fnLambert(itLambert.item());

        auto name = processMaterialName(fnLambert.name().asChar());
        if (exportedMaterials.has(name))
        {
            auto material = Material(name);

            material.setEffect("BaseSurface");

            switch (itLambert.item().apiType())
            {
                case MFn::kPhong:
                {
                    MFnPhongShader fnPhong(itLambert.item());

                    material.setParameter("specularExponent", fnPhong.cosPower());
                }

                case MFn::kBlinn:
                {
                    MFnReflectShader fnReflect(itLambert.item());

                    auto glossMap = extractMaterialTexture(fnReflect, "specularColor", "White");
                    material.setParameter("glossMap", glossMap);

                    if (glossMap == "white")
                        material.setParameter("specularColor", MColorToColor(fnReflect.specularColor()));
                    else
                        material.setParameter("specularColor", Color::White);
                }

                case MFn::kLambert:
                    material.setParameter("diffuseColor", MColorToColor(fnLambert.color()));
                    material.setParameter("diffuseMap", extractMaterialTexture(fnLambert, "color", "White"));

                    material.setParameter("normalMap", extractMaterialNormalMap(fnLambert));

                    break;

                default:
                    break;
            }

            // Don't overwrite existing material files
            auto materialFilename = outputDirectory + "/" + name + Material::MaterialExtension;
            if (!FileSystem::doesLocalFileExist(materialFilename))
                material.save(FileSystem::LocalFilePrefix + materialFilename);
        }

        itLambert.next();
    }
}

String Helper::extractMaterialTexture(const MFnDependencyNode& fn, const String& colorName, const String& fallback)
{
    auto p = fn.findPlug(colorName.cStr());

    // Get plugs connected to the color attribute
    auto plugs = MPlugArray();
    p.connectedTo(plugs, true, false);

    // Get the name from the first texture node if one exists
    for (auto i = 0U; i != plugs.length(); i++)
    {
        if (plugs[i].node().apiType() == MFn::kFileTexture)
        {
            auto name = getTextureOutputName(plugs[i].node());
            if (name.length())
                return name;
        }
    }

    return fallback;
}

String Helper::extractMaterialNormalMap(const MFnDependencyNode& fn)
{
    // Get a plug to the normalCamera attribute on the material
    auto p = fn.findPlug("normalCamera");

    // Loop through the plug's connections for a bump2d node
    auto connections = MPlugArray();
    p.connectedTo(connections, true, false);
    for (auto i = 0U; i < connections.length(); i++)
    {
        if (connections[i].node().apiType() == MFn::kBump)
            return extractMaterialTexture(connections[i].node(), "bumpValue", "FlatNormalMap.png");
    }

    return "FlatNormalMap.png";
}

String Helper::getTextureOutputName(MObject node)
{
    MFnDependencyNode fn(node);

    // Default to the name of the node
    auto name = String(fn.name().asChar());

    // If the name of the file texture is one autogenerated by Maya (of type "file<number>") then instead use the name of the
    // original texture file that was used
    if (name.startsWith("file") && name.substr(4).isNumeric())
    {
        auto fileTextureName = MString();
        fn.findPlug("fileTextureName").getValue(fileTextureName);

        if (fileTextureName.length())
        {
            // Chop off the path and extension to leave just the raw filename
            name = fileTextureName.asChar();
            auto index = name.findLastOf("/\\");
            if (index != -1)
                name = name.substr(index + 1);
            index = name.findLastOf(String::Period);
            if (index != -1)
                name = name.substr(0, index);
        }
    }

    return name;
}

}

}

#endif
