/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/SimpleTransform.h"

namespace Carbon
{

/**
 * SkeletalAnimation holds a list of named bones and animation keyframes which is is applied to a skeletal mesh at
 * runtime, and can be used simultaneously on any number of meshes.
 */
class CARBON_API SkeletalAnimation
{
public:

    /**
     * An empty SkeletalAnimation instance.
     */
    static const SkeletalAnimation Empty;

    /**
     * Holds the animation frames for a single bone.
     */
    class BoneAnimation
    {
    public:

        /**
         * Name of the bone this set of animation frames is for.
         */
        String boneName;

        /**
         * Vector of bone transforms that holds the transform of this bone at each frame.
         */
        Vector<SimpleTransform> frames;

        /**
         * Saves this bone animation to a file stream. Throws an Exception if an error occurs.
         */
        void save(FileWriter& file) const;

        /**
         * Loads this bone animation from a file stream. Throws an Exception if an error occurs.
         */
        void load(FileReader& file);
    };

    /**
     * The skeletal animation directory, currently "Animations/".
     */
    static const UnicodeString SkeletalAnimationDirectory;

    /**
     * The skeletal animation file extension, currently ".animation".
     */
    static const UnicodeString SkeletalAnimationExtension;

    SkeletalAnimation() { clear(); }

    /**
     * Clears the contents of this class.
     */
    void clear();

    /**
     * Returns the name of this skeletal animation.
     */
    const String& getName() const { return name_; }

    /**
     * Returns the frame rate to play this skeletal animation at.
     */
    float getFrameRate() const { return frameRate_; }

    /**
     * Sets the frame rate to play this skeletal animation at.
     */
    void setFrameRate(float frameRate) { frameRate_ = frameRate; }

    /**
     * Returns the number of frames in this skeletal animation.
     */
    unsigned int getFrameCount() const { return frameCount_; }

    /**
     * Returns the bone animations.
     */
    const Vector<BoneAnimation>& getBoneAnimations() const { return boneAnimations_; }

    /**
     * Sets the bone animations for this animation. Returns success flag. For this method to succeed the following
     * conditions must be met: There must be at least one bone being animated, each bone must have a unique name, and
     * all bones must have the same number of frames.
     */
    bool setBoneAnimations(const Vector<BoneAnimation>& boneAnimations);

    /**
     * Returns the list of bone transforms for a single bone in this animation, the number of items in the returned
     * vector will be equal to SkeletalAnimation::getFrameCount(). If the requested bone doesn't exist then an empty
     * vector is returned.
     */
    const Vector<SimpleTransform>& getAnimationFramesForBone(const String& bone) const;

    /**
     * Saves this skeletal animation to a file.
     */
    bool save(const UnicodeString& name) const;

    /**
     * Loads this skeletal animation from a file.
     */
    bool load(const String& name);

    /**
     * Returns whether this skeletal animation has been successfully loaded and is ready for use.
     */
    bool isLoaded() const { return isLoaded_; }

    /**
     * Loads and returns the skeletal animation with the given name. Each skeletal animation will only ever be loaded
     * once. To check whether the load succeeded look at the SkeletalAnimation::isLoaded() value of the returned
     * instance. The return value will never be null.
     */
    static const SkeletalAnimation* get(const String& name);

private:

    String name_;
    unsigned int frameCount_ = 0;
    Vector<BoneAnimation> boneAnimations_;

    float frameRate_ = 0.0f;

    bool isLoaded_ = false;
};

}
