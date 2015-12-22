/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Scene/Camera.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/SkyDome.h"

namespace Carbon
{

const auto SkyDomeVersionInfo = VersionInfo(1, 0);

const auto LayersAboveHorizontal = 40U;
const auto LayersBelowHorizontal = 40U;
const auto LayerCount = LayersAboveHorizontal + LayersBelowHorizontal;
const auto SliceCount = 50U;

SkyDome::~SkyDome()
{
    onDestruct();
    clear();
}

void SkyDome::clear()
{
    material_.clear();
    geometryChunk_.clear();

    radius_ = 5160.0f;
    height_ = 1000.0f;

    Entity::clear();

    setRenderPriority(INT_MIN);
}

void SkyDome::save(FileWriter& file) const
{
    Entity::save(file);

    file.beginVersionedSection(SkyDomeVersionInfo);
    file.write(material_, radius_, height_);
    file.endVersionedSection();
}

void SkyDome::load(FileReader& file)
{
    try
    {
        Entity::load(file);

        file.beginVersionedSection(SkyDomeVersionInfo);
        file.read(material_, radius_, height_);
        file.endVersionedSection();

        geometryChunk_.clear();
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

bool SkyDome::gatherGeometry(GeometryGather& gather)
{
    if (!Entity::gatherGeometry(gather))
        return false;

    if (shouldProcessGather(gather))
    {
        createGeometry();

        auto material = getMaterialRoot() + material_;
        auto overrideParameters = getMaterialOverrideParameters(material);

        gather.changePriority(getRenderPriority());
        gather.changeMaterial(material_, overrideParameters);
        gather.changeTransformation(gather.getCameraPosition());
        gather.addGeometryChunk(geometryChunk_);
    }

    return true;
}

void SkyDome::precache()
{
    createGeometry();
    materials().getMaterial(material_).precache();

    Entity::precache();
}

bool SkyDome::createGeometry()
{
    // Check geometry not already created
    if (geometryChunk_.getVertexCount() != 0)
        return true;

    auto vertexCount = (LayerCount + 1) * SliceCount;

    geometryChunk_.clear();
    geometryChunk_.addVertexStream({VertexStream::Position, 3});
    geometryChunk_.setVertexCount(vertexCount);

    // Dome vertices
    auto dome = geometryChunk_.lockVertexData<Vec3>();
    auto alphaStep = Math::HalfPi / float(LayersAboveHorizontal);
    auto thetaStep = Math::TwoPi / float(SliceCount);
    auto c = 0U;

    auto alpha = LayersBelowHorizontal * -alphaStep;

    for (auto i = 0U; i < LayerCount + 1; i++)
    {
        auto pitchCos = cosf(alpha) * radius_;
        auto theta = 0.0f;

        for (auto j = 0U; j < SliceCount; j++, c++)
        {
            dome[c].x = sinf(theta) * pitchCos;
            dome[c].y = sinf(alpha) * height_;
            dome[c].z = cosf(theta) * -pitchCos;

            theta += thetaStep;
        }

        alpha += alphaStep;
    }

    geometryChunk_.unlockVertexData();

    // Dome indices
    auto drawItems = Vector<DrawItem>();
    drawItems.emplace(GraphicsInterface::TriangleStrip, LayerCount * (SliceCount + 2) * 2 - 2, 0);

    auto indices = Vector<unsigned int>();
    indices.reserve(drawItems.back().getIndexCount());

    for (auto i = 0U; i < LayerCount; i++)
    {
        if (i)
        {
            indices.append(indices.back());
            indices.append((i + 1) * SliceCount);
        }

        for (auto j = 0U; j <= SliceCount; j++)
        {
            indices.append((i + 1) * SliceCount + j % SliceCount);
            indices.append((i + 0) * SliceCount + j % SliceCount);
        }
    }

    geometryChunk_.setupIndexData(drawItems, indices);
    geometryChunk_.registerWithRenderer();

    return true;
}

void SkyDome::setDomeSize(float radius, float height)
{
    radius_ = radius;
    height_ = height;

    geometryChunk_.clear();
}

}
