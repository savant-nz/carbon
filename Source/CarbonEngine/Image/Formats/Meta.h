/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

namespace Carbon
{

/**
 * This class implements the .meta image format which is a text based image format that loads data from other images and passes
 * it through the set of transforms defined in a .meta file and makes the result available as its final image. The .meta format
 * allows #-style comments and is made up of a series of commands, one command per line, which are executed in the order they
 * appear. The following commands are available:
 *
 *  - `LoadImage <image name>`. This command must appear exactly once in every .meta file and must be the first command to
 *    appear. It takes one parameter which is the name of the image to load, relative to the .meta file. Any valid image can be
 *    passed here, including other .meta files.
 *
 *  - `SetPixelFormat <pixel format>`. Changes the image to the specified pixel format. The pixel format given must be a
 *    recognized pixel format string, e.g. RGBA8.
 *
 *  - `RotateCCW`. Rotates the image counter-clockwise by 90 degrees.
 *
 *  - `FlipHorizontal`. Flips the image in the Y axis.
 *
 *  - `FlipVertical`. Flips the image in the X axis.
 *
 *  - `GenerateMipmaps`. Generates mipmaps for this image if it doesn't already have them.
 */
class Meta
{
public:

    static bool load(FileReader& file, Image& image, unsigned int imageIndex, Image::PixelFormat targetPixelFormat)
    {
        // Because .meta files execute their own image loads it is necessary to track the list of in-progress .meta files in
        // order to avoid possible infinite recursion
        static auto currentlyLoadingMetaFiles = Vector<UnicodeString>();
        if (currentlyLoadingMetaFiles.has(file.getName().asLower()))
        {
            LOG_ERROR << "Infinite recursion detected in meta file";
            return false;
        }

        currentlyLoadingMetaFiles.append(file.getName().asLower());

        try
        {
            if (imageIndex)
                LOG_WARNING << "Image indexing not supported for this format";

            // Tokenize the file contents
            auto lineTokens = Vector<Vector<String>>();
            if (!file.getLineTokens(lineTokens))
                throw Exception("Failed parsing file");

            for (const auto& tokens : lineTokens)
            {
                auto command = tokens[0].asLower();

                if (command == "loadimage")
                {
                    if (tokens.size() < 2)
                        throw Exception("Invalid load image command");

                    auto sourceImageName = String(tokens, " ", 1);
                    auto sourceFilename = FileSystem::joinPaths(FileSystem::getDirectory(file.getName()), sourceImageName);

                    if (!ImageFormatRegistry::loadImageFile(sourceFilename, image, targetPixelFormat))
                        throw Exception() << "Failed loading meta source image: " << sourceImageName;
                }
                else if (command == "setpixelformat")
                {
                    if (tokens.size() < 2)
                        throw Exception("Invalid set pixel format command");

                    auto pixelFormat = Image::getPixelFormatFromString(tokens[1]);
                    if (pixelFormat == Image::UnknownPixelFormat)
                        throw Exception() << "Unrecognized pixel format: " << tokens[1];

                    if (!image.setPixelFormat(pixelFormat))
                        throw Exception() << "Failed changing to pixel format: " << tokens[1];
                }
                else if (command == "rotateccw")
                {
                    if (!image.rotateCCW())
                        throw Exception("Failed rotating image");
                }
                else if (command == "fliphorizontal")
                {
                    if (!image.flipHorizontal())
                        throw Exception("Failed flipping image horizontally");
                }
                else if (command == "flipvertical")
                {
                    if (!image.flipVertical())
                        throw Exception("Failed flipping image vertically");
                }
                else if (command == "generatemipmaps")
                {
                    if (!image.generateMipmaps())
                        throw Exception("Failed generating mipmaps");
                }
                else
                    throw Exception() << "Unknown command: " << command;

                // Check an image is loaded
                if (!image.isValidImage())
                    throw Exception("The first command in a .meta file must be LoadImage");
            }

            currentlyLoadingMetaFiles.popBack();

            return true;
        }
        catch (const Exception& e)
        {
            LOG_ERROR << e;

            currentlyLoadingMetaFiles.popBack();

            return false;
        }
    }
};

CARBON_REGISTER_IMAGE_FILE_FORMAT(meta, Meta::load, nullptr)

}
