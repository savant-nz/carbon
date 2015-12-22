/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Render/Texture/Texture3D.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

namespace Carbon
{

bool Texture3D::upload()
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
            auto d = image.getDepth();
            auto dataOffset = 0U;
            for (auto j = 0U; j < mipmapCount; j++)
            {
                auto dataSize = Image::getImageDataSize(w, h, d, image.getPixelFormat());

                if (j >= firstMipmap)
                {
                    uploadData.emplace(w, h, d, &image.getDataForFrame(i)[dataOffset], dataSize);
                    videoMemoryUsed_ += dataSize;
                }

                dataOffset += dataSize;
                Image::getNextMipmapSize(w, h, d);
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

}
