/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace Carbon
{

/**
 * Resolution class that describes a single resolution (e.g. 1920x1080, 1024x768) that is used by the PlatformInterface class to
 * manage supported resolutions.
 */
class CARBON_API Resolution
{
public:

    /**
     * Resolution object that is used to indicate an invalid or unspecified resolution, both width and height are zero and so
     * its Resolution::isValid() method will return false.
     */
    static const Resolution Zero;

    Resolution() {}

    /**
     * Constructs this resolution with the given values.
     */
    Resolution(unsigned int width, unsigned int height, bool isCustom = false, bool isRetina = false)
        : width_(width), height_(height), isCustomResolution_(isCustom), isRetinaResolution_(isRetina)
    {
    }

    /**
     * Equality operator.
     */
    bool operator==(const Resolution& other) const
    {
        return width_ == other.getWidth() && height_ == other.getHeight() &&
            isCustomResolution_ == other.isCustomResolution() && isRetinaResolution_ == other.isRetinaResolution();
    }

    /**
     * Inequality operator.
     */
    bool operator!=(const Resolution& other) const { return !operator==(other); }

    /**
     * Compares two resolutions, first by width, then by height. This is used to sort a list of resolutions.
     */
    bool operator<(const Resolution& other) const
    {
        return width_ == other.width_ ? height_ < other.height_ : width_ < other.width_;
    }

    /**
     * Returns this resolution's width.
     */
    unsigned int getWidth() const { return width_; }

    /**
     * Returns this resolution's height.
     */
    unsigned int getHeight() const { return height_; }

    /**
     * Returns this resolution's aspect ratio.
     */
    float getAspectRatio() const { return float(width_) / float(height_); }

    /**
     * Returns whether this resolution is valid, meaning that it has non-zero values for both width and height.
     */
    bool isValid() const { return width_ && height_; }

    /**
     * Returns whether or not this resolution was added via PlatformInterface::addCustomResolution(), if this is the case then
     * it is likely that fullscreen rendering using this resolution will not be possible.
     */
    bool isCustomResolution() const { return isCustomResolution_; }

    /**
     * Returns whether or not this resolution is a retina resolution such as that introduced by the iPhone 4 and iPad 3.
     */
    bool isRetinaResolution() const { return isRetinaResolution_; }

    /**
     * Returns this resolution as a standard formatted resolution string such as "800x600".
     */
    operator UnicodeString() const { return UnicodeString() << width_ << "x" << height_; }

private:

    unsigned int width_ = 0;
    unsigned int height_ = 0;

    bool isCustomResolution_ = false;
    bool isRetinaResolution_ = false;
};

}
