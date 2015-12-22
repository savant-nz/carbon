/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/ParameterArray.h"
#include "CarbonEngine/Math/AABB.h"
#include "CarbonEngine/Math/SimpleTransform.h"

namespace Carbon
{

/**
 * Used by Scene::gatherGeometry() and Scene::gatherShadowGeometry() to create an EffectQueueArray that contains everything that
 * needs to be drawn for a single scene, or shadow geometry that needs to be drawn for a single light source.
 */
class CARBON_API GeometryGather : private Noncopyable
{
public:

    /**
     * Copy constructor (not implemented).
     */
    GeometryGather(const GeometryGather& other);

    /**
     * Initializes this geometry gather with the given details. The \a queues instance will receive the output of this gather.
     */
    GeometryGather(const Vec3& cameraPosition, const ConvexHull& frustum, bool isShadowGeometryGather,
                   EffectQueueArray& queues);

    ~GeometryGather();

    /**
     * Returns the world position of the camera for this geometry gather.
     */
    const Vec3& getCameraPosition() const { return cameraPosition_; }

    /**
     * Returns the frustum object that visible geometry is being gathered for.
     */
    const ConvexHull& getFrustum() const { return frustum_; }

    /**
     * Indicates whether only shadow casting geometry should be gathered.
     */
    bool isShadowGeometryGather() const { return isShadowGeometryGather_; }

    /**
     * Gathers of shadow casting geometry have the option of specifying arbitrary world-space boundings that the renderer will
     * then treat as having important shadow generating content, this is in addition to the bounds around any geometry chunks
     * added to the gather. This is needed in order to inform the renderer of correct bounds around animated geometry such as
     * skeletal meshes, without this it has no way of knowing the extents of such shadow casting geometry and may clip it off
     * causing shadowing artifacts when rendering.
     */
    void enlargeExtraWorldSpaceShadowCasterExtents(const AABB& aabb) { extraWorldSpaceShadowCasterExtents_.merge(aabb); }

    /**
     * When this is a shadow geometry gather this method returns the extra world space shadow caster extents, see
     * GeometryGather::enlargeExtraWorldSpaceShadowCasterExtents() for details.
     */
    const AABB& getExtraWorldSpaceShadowCasterExtents() const { return extraWorldSpaceShadowCasterExtents_; }

    /**
     * Changes the priority that geometry is being given. Geometry of lower priority is drawn before geometry of higher priority
     * This is used to allow scene-level control of render order where needed. The default priority is zero. The draw order of
     * geometry at the same priority level is undefined.
     */
    void changePriority(int priority) { currentPriority_ = priority; }

    /**
     * Changes the active material, a material must be active before queuing anything for rendering (with the exception of
     * text).
     */
    void changeMaterial(const String& material, const ParameterArray& materialOverrideParameters = ParameterArray::Empty);

    /**
     * Similar to GeometryGather::changeMaterial() however this should only be used when the Material pointer is known and the
     * caller is certain that the specified material has not been used before in this gather. If these conditions are met then
     * this method can be used to improve gather performance as it will be faster than calling GeometryGather::changeMaterial().
     */
    void newMaterial(Material* material, const ParameterArray& materialOverrideParameters = ParameterArray::Empty,
                     const ParameterArray& internalParams = ParameterArray::Empty);

    /**
     * Changes the current transform and scale.
     */
    void changeTransformation(const SimpleTransform& transform, const Vec3& scale = Vec3::One)
    {
        transform_ = transform;
        scale_ = scale;

        if (currentQueue_)
            currentQueue_->isTransformCurrent = false;
    }

    /**
     * Changes the current transform and scale.
     */
    void changeTransformation(const Vec3& position, const Quaternion& orientation = Quaternion::Identity,
                              const Vec3& scale = Vec3::One)
    {
        changeTransformation({position, orientation}, scale);
    }

    /**
     * Adds a geometry chunk that will be rendered with the current material and transformation.
     */
    void addGeometryChunk(const GeometryChunk& geometryChunk, int drawItemIndex = -1);

    /**
     * Adds a rectangle of the given size that will be rendered with the current material and transformation. The rectangle is
     * double-sided and lies in the XY plane.
     */
    void addRectangle(float width, float height);

    /**
     * Adds some text to render with the current transformation. This changes the current material to "Font".
     */
    void addText(const Font* font, float fontSize, const UnicodeString& text, const Color& color = Color::White);

    /**
     * Adds an immediate triangles queue item, following this call the requested number of triangles should be queued using
     * calls to GeometryGather::addImmediateTriangle() in order to specify the triangle vertices. Internally the immediate
     * triangles are rendered using the "ImmediateGeometry" material.
     */
    void addImmediateTriangles(unsigned int triangleCount);

    /**
     * Specifies a single immediate triangle which should have been allocated prior through a call to
     * GeometryGather::addImmediateTriangles(). This is mainly used for rendering GUI elements that don't use custom materials.
     */
    void addImmediateTriangle(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Color& color);

private:

    Vec3 cameraPosition_;
    const ConvexHull& frustum_;

    const bool isShadowGeometryGather_ = false;
    AABB extraWorldSpaceShadowCasterExtents_;

    SimpleTransform transform_;
    Vec3 scale_;

    struct MaterialQueueInfo
    {
        String material;

        EffectQueue* queue = nullptr;

        bool isTransformCurrent = false;

        MaterialQueueInfo() {}
        MaterialQueueInfo(String material_, EffectQueue* queue_) : material(std::move(material_)), queue(queue_) {}
    };

    Vector<MaterialQueueInfo> materialQueueInfos_;
    MaterialQueueInfo* currentQueue_ = nullptr;

    EffectQueueArray& queues_;

    int currentPriority_ = 0;

    void ensureTransformIsCurrent();
};

}
