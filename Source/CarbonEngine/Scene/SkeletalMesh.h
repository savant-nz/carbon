/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Core/Runnable.h"
#include "CarbonEngine/Scene/ComplexEntity.h"

namespace Carbon
{

/**
 * A skeletally animatable mesh.
 */
class CARBON_API SkeletalMesh : public ComplexEntity
{
public:

    /**
     * The skeletal mesh file extension, currently ".skeletalmesh".
     */
    static const UnicodeString SkeletalMeshExtension;

    /**
     * The maximum number of bones allowed in a skeletal mesh. Currently 255 due to bone indices being unsigned 8-bit
     * integers.
     */
    static const auto MaximumBoneCount = 255U;

    /**
     * The maximum number of bones allowed to be referenced by the geometry in a skeletal mesh that all uses the same
     * material. This is currently limited to 80 bones. Note that the total number of bones in a skeletal mesh is
     * limited to 255, so a mesh that uses more than 80 bones will need to use its materials appropriately in order to
     * reach higher bone counts.
     */
    static const auto MaximumMaterialBoneCount = 80U;

    /**
     * Describes a single bone in a skeletal mesh.
     */
    class Bone
    {
    public:

        /**
         * The name of this bone.
         */
        String name;

        /**
         * The index of the parent bone, will be -1 for root bones that don't have a parent.
         */
        int parent = -1;

        /**
         * The length of this bone.
         */
        float length = 0.0f;

        /**
         * The parent space relative transform of this bone in the reference pose.
         */
        SimpleTransform referenceRelative;

        /**
         * The inverse transform for this bone's absolute transform when the whole skeleton is in the reference pose.
         * This converts from object space to bone space and is calculated once during skeletal mesh setup and then
         * stored for use during animation.
         */
        SimpleTransform inverseReferenceAbsolute;

        /**
         * The current relative transform of this bone in parent space
         */
        SimpleTransform currentRelative;

        /**
         * The current absolute transform of this bone in skeleton space.
         */
        SimpleTransform currentAbsolute;

        /**
         * This is a per-frame cached copy of \a currentAbsolute multiplied by \a inverseReferenceAbsolute that is used
         * to optimize skeletal animation by reducing the amount of per-vertex math that is required.
         */
        SimpleTransform currentCombinedTransform;

        /**
         * Whether this bone should be physically simulated when this skeletal mesh is being animated using ragdoll.
         * Default value is true.
         */
        bool isRagdollBone = true;

        /**
         * For use in ragdoll, this is the inverse of the absolute orientation of this bone's corresponding body in the
         * physics simulation when the skeleton is in the reference pose. This is used when reading back the position of
         * this bone's physics body to convert the simulated orientation into an absolute orientation for this bone.
         */
        Quaternion ragdollReferenceOrientationAbsoluteInverse;

        /**
         * This class describes movement constraints on an axis that will be applied to this bone when it is part of a
         * ragdoll simulation. Currently the only supported constraint is the amount of rotation around the axis.
         */
        class RagdollAxisConstraint
        {
        public:

            /**
             * Whether or not constraints on this axis are active, when this is false no constraints will be enforced on
             * this joint during ragdoll simulation.
             */
            bool enabled = false;

            /**
             * The smallest rotation in radians around this axis that is permitted. Should be between -Pi and Pi and not
             * be greater than \a maximumAngle.
             */
            float minimumAngle = 0.0f;

            /**
             * The largest rotation in radians around this axis that is permitted. Should be between -Pi and Pi and not
             * be less than \a minimumAngle.
             */
            float maximumAngle = 0.0f;

            /**
             * Returns this ragdoll axis constraint as a human-readable string.
             */
            operator UnicodeString() const
            {
                return UnicodeString() << "enabled: " << enabled << ", minimumAngle: " << minimumAngle
                                       << ", maximumAngle: " << maximumAngle;
            }

            /**
             * Saves this ragdoll axis constraint to a file stream. Throws an Exception if an error occurs.
             */
            void save(FileWriter& file) const { file.write(enabled, minimumAngle, maximumAngle); }

            /**
             * Loads this ragdoll axis constraint from a file stream. Throws an Exception if an error occurs.
             */
            void load(FileReader& file) { file.read(enabled, minimumAngle, maximumAngle); }
        };

        /**
         * Describes how this bone's X axis is constrained when under ragdoll simulation.
         */
        RagdollAxisConstraint ragdollXConstraint;

        /**
         * Describes how this bone's Y axis is constrained when under ragdoll simulation.
         */
        RagdollAxisConstraint ragdollYConstraint;

        /**
         * Describes how this bone's Z axis is constrained when under ragdoll simulation.
         */
        RagdollAxisConstraint ragdollZConstraint;

        /**
         * Local-space bounding box around the reference pose geometry that is influenced by this bone, this is used for
         * culling.
         */
        AABB aabb;

        /**
         * Calculates the length of this bone from the \a position value of \a referenceRelative and stores it in the \a
         * length member variable.
         */
        void calculateLength();

        /**
         * Saves this bone to a file stream.
         */
        void save(FileWriter& file) const;

        /**
         * Loads this bone from a file stream.
         */
        void load(FileReader& file);
    };

    /**
     * Describes a single weight applied to a vertex in a skeletal mesh. There are usually several of these per vertex.
     * Note that this class is not used as a final store for bone and weight data as all that information ends up in
     * vertex streams in geometry chunks.
     */
    class VertexWeight
    {
    public:

        VertexWeight() {}

        /**
         * Constructs this vertex weight with the given bone index and weight values.
         */
        VertexWeight(uint8_t bone, float weight) : bone_(bone), weight_(weight) {}

        /**
         * Returns the index of the bone the vertex is weighted to.
         */
        uint8_t getBone() const { return bone_; }

        /**
         * Returns the weighting of this vertex to the bone.
         */
        float getWeight() const { return 0.0f; }

        /**
         * Normalizes all the weights in a vector of vertex weights so that they add up to one.
         */
        static void normalizeWeights(Vector<VertexWeight>& weights);

        /**
         * Chooses the most important weights from the given vector of vertex weights and discards the rest. The new set
         * of weights are also normalized.
         */
        static void limitWeightCount(Vector<VertexWeight>& weights, unsigned int maximumWeightCount);

    private:

        uint8_t bone_ = 0;
        float weight_ = 0.0f;
    };

    SkeletalMesh();

    /**
     * Copy constructor (not implemented).
     */
    SkeletalMesh(const SkeletalMesh& other);

    ~SkeletalMesh() override;

    /**
     * Sets all the bones in this skeletal mesh to their reference pose transforms. Note that any currently active
     * animation will immediately override this change when it is applied to the skeleton in the next frame. To clear
     * all the animations on this skeletal mesh use the SkeletalMesh::removeAllAnimations() method.
     */
    void setReferencePose();

    /**
     * Adds an animation to use on this skeletal mesh. The animation can be played once or loop continuously. If the
     * given animation is not valid or it is already playing on this skeletal mesh then false is returned. If \a
     * blendFromInitialBoneTransforms is true then the current skeletal transforms will be blended into the first frame
     * of the animation. This makes the animation one frame longer and smooths out any discontinuities in consecutive
     * animations.
     */
    bool addAnimation(const String& name, bool loop = false, bool blendFromInitialBoneTransforms = false);

    /**
     * Removes the given animation from this skeletal mesh.
     */
    void removeAnimation(const String& name);

    /**
     * Removes all animations that are playing on this skeletal mesh.
     */
    void removeAllAnimations();

    /**
     * Removes all animations from this skeletal mesh and then adds the given animation. The parameters for this method
     * are the same as for SkeletalMesh::addAnimation(). Returns success flag.
     */
    bool setAnimation(const String& name, bool loop = false, bool blendFromInitialBoneTransforms = false);

    /**
     * Returns the animations that are currently active on this skeletal mesh.
     */
    Vector<String> getAnimations() const;

    /**
     * Returns the current frame that the given animation is on. This is a floating point value because interpolation
     * between frames is used to smooth animation playback.
     */
    float getAnimationCurrentFrame(const String& name) const;

    /**
     * Sets the current frame that the given animation is on, this can be between frames as interpolation is used to
     * smooth animation playback. If the given frame value is negative or greater than the number of frames in the
     * animation then it will be wrapped around appropriately. Note that changes to this value will not take effect on
     * the mesh until the next UpdateEvent is received. Returns success flag.
     */
    bool setAnimationCurrentFrame(const String& name, float frame);

    /**
     * Returns whether the given animation is paused.
     */
    bool isAnimationPaused(const String& name) const;

    /**
     * Sets whether the given animation is paused. Returns success flag.
     */
    bool setAnimationPaused(const String& name, bool paused);

    /**
     * Returns the current animation speed scaling factor being used for the animations playing on this skeletal mesh.
     * The default speed scale factor is 1.0 which means the animations are played back at the speed recorded in the
     * animation file.
     */
    float getAnimationSpeedScale() const;

    /**
     * Sets the current animation speed scaling factor being used for the animations playing on this skeletal mesh. The
     * default value is 1.0. Note that using a negative value will cause animations to play back in reverse.
     */
    void setAnimationSpeedScale(float scale);

    /**
     * Makes all skeleton bones valid attachment points for child entities, they can be attached to by using the name of
     * the bone.
     */
    bool getAttachmentPointLocalTransform(const String& name, SimpleTransform& transform) const override;

    /**
     * Sets up this skeletal mesh with the given bones and triangles. This routine validates the contents of the given
     * data and then compiles it into a skeletal mesh suitable for real-time animation. Returns success flag.
     */
    bool setup(const Vector<Bone>& bones, TriangleArraySet& triangleSet, Runnable& r = Runnable::Empty);

    /**
     * Saves this skeletal mesh to a file. Returns success flag.
     */
    bool saveSkeletalMesh(const UnicodeString& name) const;

    /**
     * Adds a skeletal mesh component that is loaded from a file. Returns success flag.
     */
    virtual bool addSkeletalMesh(const String& name);

    /**
     * Removes a skeletal mesh component that was added with SkeletalMesh::addSkeletalMesh(). Returns success flag.
     */
    virtual bool removeSkeletalMesh(const String& name);

    /**
     * Unloads any skeletal mesh currently present on this entity.
     */
    virtual void clearSkeletalMesh();

    /**
     * This is called by the animation routines when an animation on this skeletal mesh reaches its final frame.
     */
    virtual void onAnimationFinished(const SkeletalAnimation* animation) {}

    /**
     * Returns whether or not GPU vertex skinning is currently enabled for this skeletal mesh. Defaults to true.
     */
    bool isGPUSkinningEnabled();

    /**
     * Sets whether or not GPU vertex skinning is currently enabled for this skeletal mesh. When it is disabled vertex
     * skinning will be done by the CPU which is significantly slower in most cases.
     */
    void setGPUSkinningEnabled(bool enabled);

    /**
     * Returns a list of the materials currently in use by this skeletal mesh.
     */
    Vector<String> getMaterials() const;

    /**
     * Sets the materials currently in use by this skeletal mesh. The given list of materials must have as many elements
     * as are returned by SkeletalMesh::getMaterials(), and is used to directly replace all the materials in use by this
     * skeletal mesh. Returns success flag.
     */
    bool setMaterials(const Vector<String>& materials);

    /**
     * Returns the index of the root bone that is used for animation-defined translations and rotations of this entity.
     * A return value of -1 means that animation-defined entity transforms are disabled. The default value is -1.
     */
    int getRootBoneToFollow() const;

    /**
     * Sets the index of the root bone to use for animation-defined translations and rotations of this entity. The given
     * bone index must correspond to a root bone (i.e. one that has a parent index of -1). An index of -1 will disable
     * animation defined entity transforms. Returns success flag.
     */
    bool setRootBoneToFollow(int index);

    /**
     * Returns the number of bones in this skeletal mesh.
     */
    unsigned int getBoneCount() const;

    /**
     * Returns the bone at the specified index.
     */
    const Bone& getBone(unsigned int index) const;

    /**
     * Returns a list of the names of the bones in this skeletal mesh.
     */
    Vector<String> getBoneNames() const;

    /**
     * Returns the index of the given bone in this skeletal mesh, or -1 if no bone with the given name exists.
     */
    int getBoneIndex(const String& boneName) const;

    /**
     * Returns the reference pose transform for the given bone relative to its parent. Returns success flag.
     */
    bool getBoneReferencePoseRelativeTransform(unsigned int index, SimpleTransform& transform) const;

    /**
     * Returns the current relative transform of the given bone relative to its parent. Returns success flag.
     */
    bool getBoneRelativeTransform(unsigned int index, SimpleTransform& transform) const;

    /**
     * Sets the current relative transform of the given bone relative to its parent. Note that animations constantly
     * update this transform and so any changes made to it may be overwritten if there is an animation playing on the
     * affected bone. Returns success flag.
     */
    bool setBoneRelativeTransform(unsigned int index, const SimpleTransform& transform);

    /**
     * Returns whether the given bone will be physically simulated when this skeletal mesh is running as a ragdoll.
     * Defaults to true for all bones.
     */
    bool isRagdollBone(const String& boneName) const;

    /**
     * Sets whether the given bone should be physically simulated when this skeletal mesh is running as a ragdoll. This
     * method only works when this mesh has not been made physical. If a bone is removed from the ragdoll simulation it
     * causes all child bones below it in the skeleton hierarchy to be excluded from ragdoll simulation. Returns success
     * flag.
     */
    bool setRagdollBone(const String& boneName, bool ragdollBone);

    /**
     * Makes this skeletal mesh a physical ragdoll in the scene. \a fixed must be false. Once the ragdoll has been
     * created it will be used to control the transforms of all the joints in this skeletal mesh. The current ragdoll
     * can be disabled using the SkeletalMesh::makeNotPhysical() method. Returns success flag.
     */
    bool makePhysical(float mass, bool fixed = false) override;

    /**
     * Returns whether this skeletal mesh has been made into a physical ragdoll in the scene.
     */
    bool isPhysical() const override;

    /**
     * Removes any physical ragdoll belonging to this skeletal mesh.
     */
    void makeNotPhysical() override;

    /**
     * Prints this skeletal mesh's skeleton to the main log file.
     */
    void debugSkeleton() const;

    /**
     * Sets the maximum number of weights that will be used when doing per vertex skinning of this skeletal mesh.
     * Reducing the maximum weight count can help to improve skinning performance. Note that this method is destructive,
     * any skinning data that gets removed as a result of reducing the number of weights per vertex can't be retrieved
     * except by reloading the original mesh file. The only valid values that can be passed to this method are 1, 2, 3
     * and 4. The default value is a maximum of 4 weights per vertex. Returns success flag.
     */
    bool setMaximumAllowedWeightsPerVertex(unsigned int maximumAllowedWeightsPerVertex);

    /**
     * Sets whether rendering of the skeleton is enabled, this is useful when debugging. When the skeleton is being
     * rendered the mesh itself is not drawn.
     */
    void setDrawSkeletonEnabled(bool enabled);

    /**
     * Returns whether rendering of the skeleton is enabled, see SkeletalMesh::setDrawSkeletonEnabled() for details.
     */
    bool isDrawSkeletonEnabled() const;

    void clear() override;
    bool isPerFrameUpdateRequired() const override;
    void update() override;
    void intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry) override;
    bool isShadowCaster() const override;
    void setShadowCaster(bool isShadowCaster) override;
    void setShadowCaster(const String& meshName, bool isShadowCaster) override;
    bool gatherGeometry(GeometryGather& gather) override;
    void precache() override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;
    void getAttachmentPointNames(Vector<String>& names, const String& requiredPrefix = String::Empty) const override;
    operator UnicodeString() const override;

private:

    class Members;
    Members* m = nullptr;

    void calculateLocalAABB() const override;
};

}
