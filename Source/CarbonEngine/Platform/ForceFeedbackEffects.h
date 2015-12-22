/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Abstract base class for all available force feedback effects, contains common force feedback effect parameters.
 */
class CARBON_API ForceFeedbackEffect
{
public:

    /**
     * The length of time this effect should play for, measured in microseconds, If this is set to `UINT_MAX` then the effect
     * will play forever. The default value is 1,000,000, meaning the effect will play for one second.
     */
    unsigned int duration = 1000000;

    /**
     * The gain to be applied to this effect, in the range 0 - 10,000. The gain is a scaling factor applied to all magnitudes of
     * this effect. The default value is 10,000.
     */
    unsigned int gain = 10000;

    /**
     * The direction to apply the effect in. This applies to the X and Y axes in a cartesian coordinate system.
     */
    std::array<int, 2> direction = {};

    virtual ~ForceFeedbackEffect() {}
};

/**
 * Describes a constant force effect for force feedback enabled game controllers.
 */
class CARBON_API ForceFeedbackConstantForceEffect : public ForceFeedbackEffect
{
public:

    /**
     * The magnitude of the constant force effect, in the range -10,000 to 10,000. The default value is 10,000.
     */
    int magnitude = 10000;
};

/**
 * Describes a ramp effect for force feedback enabled game controllers.
 */
class CARBON_API ForceFeedbackRampForceEffect : public ForceFeedbackEffect
{
public:

    /**
     * The magnitude at the beginning of the ramp effect, in the range -10,000 to 10,000. The default value is zero.
     */
    int startMagnitude = 0;

    /**
     * The magnitude at the end of the ramp effect, in the range -10,000 to 10,000. The default value is 10,000.
     */
    int endMagnitude = 10000;
};

/**
 * Describes a periodic effect for force feedback enabled game controllers.
 */
class CARBON_API ForceFeedbackPeriodicEffect : public ForceFeedbackEffect
{
public:

    /**
     * Enumeration of the types of waveforms available for periodic effects.
     */
    enum WaveformType
    {
        /**
         * A square waveform.
         */
        WaveformSquare,

        /**
         * A sinusoidal waveform.
         */
        WaveformSine,

        /**
         * A triangular waveform.
         */
        WaveformTriangle,

        /**
         * A upward sawtooth waveform, the waveform drops vertically after it reaches the maximum positive force.
         */
        WaveformSawtoothUp,

        /**
         * A downward sawtooth waveform, the waveform rises vertically after it reaches the maximum negative force.
         */
        WaveformSawtoothDown
    };

    /**
     * The type of waveform to use for this periodic effect. The default value is \a WaveformSquare.
     */
    WaveformType waveform = WaveformSquare;

    /**
     * The magnitude of the periodic effect, in the range -10,000 to 10,000. The default value is 10,000.
     */
    unsigned int magnitude = 10000;

    /**
     * Vertical offset of the periodic effect, shifts the waveform up or down. The default value is zero.
     */
    int offset = 0;

    /**
     * Position in the cycle of the periodic effect at which playback begins, in the range 0 to 35,999. The default value is
     * zero.
     */
    unsigned int phase = 0;

    /**
     * Period of the effect, in microseconds. The default value is 1,000,000, meaning the period of the effect is one second.
     */
    unsigned int period = 1000000;
};

}
