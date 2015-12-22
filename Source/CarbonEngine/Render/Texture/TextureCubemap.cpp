/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Vec3.h"
#include "CarbonEngine/Graphics/GraphicsInterface.h"
#include "CarbonEngine/Render/Texture/TextureCubemap.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

namespace Carbon
{

void TextureCubemap::getCubeVector(unsigned int size, unsigned int index, unsigned int x, unsigned int y, Vec3& v) const
{
    auto s = (float(x) + 0.5f) / float(size);
    auto t = (float(y) + 0.5f) / float(size);
    auto sc = s * 2.0f - 1.0f;
    auto tc = t * 2.0f - 1.0f;

    if (index == 0)
        v.setXYZ(1.0f, -tc, -sc);
    else if (index == 1)
        v.setXYZ(-1.0f, -tc, sc);
    else if (index == 2)
        v.setXYZ(sc, 1.0f, tc);
    else if (index == 3)
        v.setXYZ(sc, -1.0f, -tc);
    else if (index == 4)
        v.setXYZ(sc, -tc, 1.0f);
    else if (index == 5)
        v.setXYZ(-sc, -tc, -1.0f);

    v.normalize();
}

bool TextureCubemap::upload()
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

            for (auto j = 0U; j < 6; j++)
            {
                // Construct the upload data for this mipmap chain
                auto size = image.getWidth();
                auto dataOffset = 0U;
                for (auto k = 0U; k < mipmapCount; k++)
                {
                    auto dataSize = Image::getImageDataSize(size, size, 1, image.getPixelFormat());

                    if (k >= firstMipmap)
                    {
                        uploadData.emplace(size, size, 1, &image.getCubemapDataForFrame(i, j)[dataOffset], dataSize);

                        videoMemoryUsed_ += dataSize;
                    }

                    dataOffset += dataSize;
                    size /= 2;
                }
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
