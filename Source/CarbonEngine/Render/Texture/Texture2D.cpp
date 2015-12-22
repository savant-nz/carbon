/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

namespace Carbon
{

bool Texture2D::upload()
{
    if (state_ != Texture::UploadPending)
        return false;

    try
    {
        videoMemoryUsed_ = 0;

        auto temporaryImage = Image();
        auto& image = getUploadableImage(temporaryImage);

        // Get the first mipmap level to upload
        auto firstMipmap = calculateFirstMipmapLevel();
        auto mipmapCount = image.getMipmapCount();

        // Upload all frames
        textureObjects_.resize(image.getFrameCount(), nullptr);
        for (auto i = 0U; i < textureObjects_.size(); i++)
        {
            auto uploadData = Vector<GraphicsInterface::TextureData>();

            // Construct the upload data for this mipmap chain
            auto w = image.getWidth();
            auto h = image.getHeight();
            auto dataOffset = 0U;
            for (auto j = 0U; j < mipmapCount; j++)
            {
                auto dataSize = Image::getImageDataSize(w, h, 1, image.getPixelFormat());

                if (j >= firstMipmap)
                {
                    uploadData.emplace(w, h, 1, &image.getDataForFrame(i)[dataOffset], dataSize);
                    videoMemoryUsed_ += dataSize;
                }

                dataOffset += dataSize;
                Image::getNextMipmapSize(w, h);
            }

            // Create a new texture object if needed
            if (!textureObjects_[i])
                textureObjects_[i] = graphics().createTexture();

            // Upload this frame's texture data
            if (!graphics().uploadTexture(textureObjects_[i], getTextureType(), image.getPixelFormat(), uploadData))
                throw Exception("Failed uploading texture data");
        }

        setProperties(getProperties());

        state_ = Ready;

        return true;
    }
    catch (const Exception& e)
    {
        state_ = Error;

        LOG_ERROR << "'" << getName() << "' - " << e;

        return false;
    }
}

Color Texture2D::sampleNearestTexel(float u, float v, unsigned int frame) const
{
    const_cast<Texture2D*>(this)->ensureImageIsLoaded();

    return getImage().getPixelColor(u, v, 0.0f, frame);
}

}
