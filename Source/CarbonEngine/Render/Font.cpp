/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/FileSystem/FileSystem.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Globals.h"
#include "CarbonEngine/Image/Image.h"
#include "CarbonEngine/Image/ImageFormatRegistry.h"
#include "CarbonEngine/Render/Font.h"
#include "CarbonEngine/Render/Renderer.h"
#include "CarbonEngine/Render/Texture/Texture2D.h"
#include "CarbonEngine/Render/Texture/TextureManager.h"

#ifdef CARBON_INCLUDE_FREETYPE
    #include "CarbonEngine/Render/FreeTypeIncludeWrapper.h"

    #ifdef _MSC_VER
        #pragma comment(lib, "FreeType" CARBON_STATIC_LIBRARY_DEPENDENCY_SUFFIX)
    #endif
#endif

namespace Carbon
{

const UnicodeString Font::FontDirectory = "Fonts/";
const UnicodeString Font::FontExtension = ".font";
const UnicodeCharacter Font::FallbackCharacter = '?';
const unsigned int FontHeaderID = FileSystem::makeFourCC("cfnt");

UnicodeString Font::defaultCodePoints_;

// Current font file version, should be incremented whenever the format changes
const auto FontVersionInfo = VersionInfo(5, 1);

void Font::Character::save(FileWriter& file) const
{
    file.write(codePoint_, position_, width_, height_, preMove_, postMove_, ascend_);
}

void Font::Character::load(FileReader& file)
{
    file.read(codePoint_, position_, width_, height_, preMove_, postMove_, ascend_);
}

void Font::clear()
{
    name_.clear();
    originalSystemFont_.clear();
    maximumCharacterHeight_ = 0.0f;
    maximumCharacterWidth_ = 0.0f;
    verticalOffsetToOrigin_ = 0.0f;

    characters_.clear();

    geometryChunk_.clear();
    textures().releaseTexture(texture_);
    texture_ = nullptr;
    textureDimensions_ = Vec2::Zero;

    alignCharactersToPixelBoundaries_ = true;

    referenceCount_ = 0;
}

float Font::getMaximumCharacterWidth(float size) const
{
    return (maximumCharacterWidth_ / maximumCharacterHeight_) * size;
}

float Font::getVerticalOffsetToOrigin(float size) const
{
    return (verticalOffsetToOrigin_ / maximumCharacterHeight_) * size;
}

float Font::getWidth(UnicodeCharacter character, float size) const
{
    auto s = UnicodeString();
    s.append(character);
    return getWidth(s, size);
}

float Font::getWidth(const UnicodeString& text, float size) const
{
    if (maximumCharacterHeight_ == 0.0f)
        return 0.0f;

    auto width = 0.0f;
    for (auto i = 0U; i < text.length(); i++)
    {
        auto& c = getCharacter(text.at(i));
        width += c.getPreMove() + c.getWidth() + c.getPostMove();
    }

    auto scaleFactor = size / maximumCharacterHeight_;

    return width * scaleFactor;
}

float Font::getHeight(const UnicodeString& text, float size) const
{
    if (maximumCharacterHeight_ == 0.0f)
        return 0.0f;

    auto height = 0.0f;
    for (auto i = 0U; i < text.length(); i++)
        height = std::max(height, getCharacter(text.at(i)).getHeight());

    auto scaleFactor = size / maximumCharacterHeight_;

    return height * scaleFactor;
}

float Font::getCharacterPreMove(UnicodeCharacter codePoint, float size) const
{
    if (maximumCharacterHeight_ == 0.0f)
        return 0.0f;

    auto scaleFactor = size / maximumCharacterHeight_;

    return getCharacter(codePoint).getPreMove() * scaleFactor;
}

const Font::Character& Font::getCharacter(UnicodeCharacter codePoint) const
{
    auto index = getCharacterIndex(codePoint);
    if (index == -1)
        index = getCharacterIndex(FallbackCharacter);

    return characters_[index];
}

int Font::getCharacterIndex(UnicodeCharacter codePoint) const
{
    // Do a fast lookup of the code point's character index if possible
    if (codePoint < fastCharacterIndexLookup_.size())
        return fastCharacterIndexLookup_[codePoint];

    return characters_.findBy([&](const Character& c) { return c.getCodePoint() == codePoint; });
}

bool Font::setup()
{
    if (!texture_)
    {
        // Create a texture for this font and initialize it, it will now be in the ImageLoadPending state
        texture_ = textures().create2DTexture();
        texture_->load(String() + "/" + A(FontDirectory) + name_, "Font");

        // Texture dimensions are needed to create the font geometry, if we don't know what the texture size is then
        // there is no alternative but to wait here on the main thread for the font texture image to load. This wait is
        // only needed when loading older font files, the latest format includes the texture dimensions directly in the
        // font file so the font texture load is free to occur on the texture load thread.
        if (textureDimensions_ == Vec2::Zero)
        {
            texture_->ensureImageIsLoaded();
            textureDimensions_ = Vec2(float(texture_->getWidth()), float(texture_->getHeight()));
        }
    }

    // Prepare the geometry chunk
    geometryChunk_.clear();
    geometryChunk_.addVertexStream({VertexStream::Position, 3});
    geometryChunk_.addVertexStream({VertexStream::DiffuseTextureCoordinate, 2});
    geometryChunk_.setVertexCount(characters_.size() * 4);

    // Get writable iterators over the font geometry, one for vertex position and one for texture coordinates
    geometryChunk_.lockVertexData();
    auto itPosition = geometryChunk_.getVertexStreamIterator<Vec3>(VertexStream::Position);
    auto itTexCoord = geometryChunk_.getVertexStreamIterator<Vec2>(VertexStream::DiffuseTextureCoordinate);

    auto oow = 1.0f / textureDimensions_.x;
    auto ooh = 1.0f / textureDimensions_.y;
    auto scale = 1.0f / maximumCharacterHeight_;

    // Create character geometry
    for (auto& character : characters_)
    {
        *itPosition++ = Vec3(0.0f, character.getAscend() * scale);
        *itTexCoord++ =
            Vec2(character.getPosition().x * oow, 1.0f - (character.getPosition().y + character.getHeight()) * ooh);

        *itPosition++ = Vec3(character.getWidth() * scale, character.getAscend() * scale);
        *itTexCoord++ = Vec2((character.getPosition().x + character.getWidth()) * oow,
                             1.0f - (character.getPosition().y + character.getHeight()) * ooh);

        *itPosition++ = Vec3(0.0f, (character.getHeight() + character.getAscend()) * scale);
        *itTexCoord++ = Vec2(character.getPosition().x * oow, 1.0f - character.getPosition().y * ooh);

        *itPosition++ = Vec3(character.getWidth() * scale, (character.getHeight() + character.getAscend()) * scale);
        *itTexCoord++ =
            Vec2((character.getPosition().x + character.getWidth()) * oow, 1.0f - character.getPosition().y * ooh);
    }

    // Finished creating the vertex data
    geometryChunk_.unlockVertexData();

    // Create triangle indices used when rendering, there are two triangles per character
    auto indices = Vector<unsigned int>();
    indices.reserve(characters_.size() * 6);
    for (auto i = 0U; i < characters_.size(); i++)
    {
        indices.append(i * 4 + 0);
        indices.append(i * 4 + 1);
        indices.append(i * 4 + 2);
        indices.append(i * 4 + 1);
        indices.append(i * 4 + 3);
        indices.append(i * 4 + 2);
    }
    geometryChunk_.setupIndexData({}, indices);

    return geometryChunk_.registerWithRenderer();
}

bool Font::save() const
{
    try
    {
        if (name_.length() == 0)
            throw Exception("No font loaded");

        auto file = FileWriter();
        fileSystem().open(FontDirectory + name_ + FontExtension, file);

        // Write header
        file.write(FontHeaderID);

        file.beginVersionedSection(FontVersionInfo);
        file.write(characters_, maximumCharacterHeight_, maximumCharacterWidth_, verticalOffsetToOrigin_);
        file.write(originalSystemFont_, textureDimensions_);
        file.endVersionedSection();

        file.close();

        // Save character texture as a PNG
        fileSystem().open(FontDirectory + name_ + ".png", file);

        if (texture_)
        {
            texture_->ensureImageIsLoaded();
            if (texture_->getState() == Texture::UploadPending || texture_->getState() == Texture::Ready)
            {
                auto fnWriter = ImageFormatRegistry::getWriterForExtension("png");
                if (!fnWriter)
                    throw Exception("The PNG image writer is missing");
                if (!fnWriter(file, texture_->getImage()))
                    throw Exception("Failed writing font texture");
            }
        }

        LOG_INFO << "Saved font - '" << name_ << "'";

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name_ << "' - " << e;

        return false;
    }
}

bool Font::load(const String& name)
{
    clear();

    try
    {
        name_ = name;

        auto file = FileReader();
        fileSystem().open(FontDirectory + name + FontExtension, file);

        // Read header and check ID
        if (file.readFourCC() != FontHeaderID)
            throw Exception("Not a font file");

        auto readVersion = file.beginVersionedSection(FontVersionInfo);
        if (readVersion.getMajor() != FontVersionInfo.getMajor())
            throw Exception("Font file version is too old");

        file.read(characters_, maximumCharacterHeight_, maximumCharacterWidth_, verticalOffsetToOrigin_,
                  originalSystemFont_);

        // v5.1, store texture dimensions in the .font file to avoid needing the font image loaded when creating font
        // geometry
        if (readVersion.getMinor() >= 1)
            file.read(textureDimensions_);

        file.endVersionedSection();

        setupFastCharacterIndexLookupTable();

        LOG_INFO << "Loaded font - '" << name_ << "', created from " << originalSystemFont_
                 << ", character count: " << characters_.size();

        return true;
    }
    catch (const Exception& e)
    {
        LOG_ERROR << "'" << name_ << "' - " << e;

        clear();

        return false;
    }
}

bool Font::loadFromSystemFont(const UnicodeString& name, unsigned int size, const UnicodeString& codePoints,
                              unsigned int textureSize)
{
#if defined(CARBON_INCLUDE_FREETYPE) && defined(CARBON_INCLUDE_LOCAL_FILESYSTEM_ACCESS)

    const auto padding = 2U;

    auto ftLibrary = FT_Library();
    auto ftFace = FT_Face();
    auto error = 0;

    try
    {
        clear();

        name_ = A(name);

        if (!Math::isPowerOfTwo(textureSize))
            throw Exception("The font texture size must be a power of two");

        // Make a sorted list of all the Unicode code points that will be included in this font
        auto includedCodePoints = std::set<UnicodeCharacter>();
        for (auto i = 0U; i < codePoints.length(); i++)
            includedCodePoints.insert(codePoints.at(i));

        // If no code points were explicitly specified then use the default set
        if (includedCodePoints.empty())
        {
            for (auto i = 0U; i < defaultCodePoints_.length(); i++)
                includedCodePoints.insert(defaultCodePoints_.at(i));
        }

        // The fallback character is mandatory
        includedCodePoints.insert(FallbackCharacter);

        // Get rid of non-printing ASCII characters that may have made their way in, e.g. CR/LF
        for (auto it = includedCodePoints.begin(), end = includedCodePoints.end(); it != end;)
        {
            if (*it <= 0x7F && !String::isPrintableASCII(*it))
                includedCodePoints.erase(it++);
            else
                ++it;
        }

        // Start FreeType
        if (FT_Init_FreeType(&ftLibrary))
            throw Exception("Failed initialiizing FreeType library");

#ifdef WINDOWS
        auto systemFontPath = std::array<wchar_t, MAX_PATH>();
        SHGetFolderPathW(nullptr, CSIDL_FONTS, nullptr, SHGFP_TYPE_CURRENT, &systemFontPath[0]);
        auto systemFontPaths = std::array<UnicodeString, 1>{{fromUTF16(&systemFontPath[0])}};
#elif defined(MACOS)
        auto systemFontPaths =
            std::array<UnicodeString, 3>{{"/Library/Fonts", "/Library/Fonts/Microsoft", "/System/Library/Fonts"}};
#else
        auto systemFontPaths = std::array<UnicodeString, 0>();
        throw Exception("Loading system fonts is not supported on this platform");
#endif

        auto systemFontExtensions = std::array<UnicodeString, 3>{{".ttc", ".ttf", ".dfont"}};

        // Try and load the requested system font
        for (auto& fontPath : systemFontPaths)
        {
            for (auto& fontExtension : systemFontExtensions)
            {
                auto fontFile = FileSystem::joinPaths(fontPath, name) + fontExtension;
                if (fileSystem().doesLocalFileExist(fontFile))
                {
                    error = FT_New_Face(ftLibrary, fontFile.toUTF8().as<char>(), 0, &ftFace);
                    if (error)
                        throw Exception() << "Failed loading font, error: " << error;

                    originalSystemFont_ = fontFile + " at " + size + "pt";
                    break;
                }
            }

            if (ftFace)
                break;
        }

        if (!ftFace)
            throw Exception() << "Could not find the system font '" << name << "'";

        // Set font size
        error = FT_Set_Char_Size(ftFace, 0, size * 64, 72, 72);
        if (error)
            throw Exception() << "Failed setting character size, error: " << error;

        auto textureData = Vector<unsigned int>();
        auto xPos = padding;
        auto yPos = padding;
        auto tallest = 0U;
        auto lowestAscend = 0.0f;

        // Render and position all the character glyphs
        for (auto codePoint : includedCodePoints)
        {
            auto charIndex = FT_Get_Char_Index(ftFace, codePoint);
            if (!charIndex)
            {
                LOG_WARNING << formatCodePoint(codePoint) << " is not supported by this font, skipping";
                continue;
            }

            // Load this glyph
            error = FT_Load_Glyph(ftFace, charIndex, FT_LOAD_TARGET_NORMAL);
            if (error)
                throw Exception() << "Failed loading glyph for " << formatCodePoint(codePoint) << ", error: " << error;

            // Render this glyph
            error = FT_Render_Glyph(ftFace->glyph, FT_RENDER_MODE_NORMAL);
            if (error != 0 && error != 0x13)
                throw Exception() << "Failed rendering glyph for " << formatCodePoint(codePoint)
                                  << ", error: " << error;

            auto isGlyphRenderable = (error == 0);

            // Create new character entry
            characters_.emplace(codePoint);
            auto& character = characters_.back();
            auto& bitmap = ftFace->glyph->bitmap;

            // Store width and height if this glyph is renderable
            if (isGlyphRenderable)
            {
                character.width_ = float(bitmap.width);
                character.height_ = float(bitmap.rows);
            }

            // Store glyph metrics
            auto g = FT_Glyph();
            auto bbox = FT_BBox();
            FT_Get_Glyph(ftFace->glyph, &g);
            FT_Glyph_Get_CBox(g, FT_GLYPH_BBOX_PIXELS, &bbox);
            character.preMove_ = float(bbox.xMin);
            character.postMove_ =
                float(ftFace->glyph->metrics.horiAdvance) / 64.0f - character.getWidth() - character.getPreMove();
            character.ascend_ = float(bbox.yMin);
            if (character.getAscend() < lowestAscend)
                lowestAscend = character.getAscend();

            if (isGlyphRenderable)
            {
                // Wrap onto next line if needed
                if (xPos + character.getWidth() >= textureSize)
                {
                    xPos = 0;
                    yPos += tallest + padding * 2;
                    tallest = 0;
                }

                // Keep track of the tallest character in this row so that yPos can be updated when wrapping to the next
                // row
                if (bitmap.rows > tallest)
                    tallest = bitmap.rows;

                // Record the position of this character's texture data
                character.position_.x = float(xPos);
                character.position_.y = float(yPos);

                // Ensure there's enough rows in textureData
                while (yPos + bitmap.rows > textureData.size() / textureSize)
                    textureData.resize(textureData.size() + textureSize);

                if (textureData.size() > textureSize * textureSize)
                    throw Exception("Font texture is full, try a larger texture size or reducing the font size");

                // Store font bitmap data
                for (auto y = 0U; y < uint(bitmap.rows); y++)
                {
                    auto data = &textureData[(yPos + y) * textureSize + xPos];

                    for (auto x = 0U; x < uint(bitmap.width); x++)
                        *data++ = Color(1.0f, 1.0f, 1.0f, bitmap.buffer[y * bitmap.width + x] / 255.0f).toRGBA8();
                }

                // Move past this character's texture data
                xPos += bitmap.width + padding * 2;
            }

            // Track maximum character sizes
            if (character.getHeight() > maximumCharacterHeight_)
                maximumCharacterHeight_ = character.getHeight();
            if (character.getWidth() > maximumCharacterWidth_)
                maximumCharacterWidth_ = character.getWidth();
        }

        // Clean up FreeType
        FT_Done_Face(ftFace);
        FT_Done_FreeType(ftLibrary);
        ftFace = nullptr;
        ftLibrary = nullptr;

        // Check that at least some texture data was actually rendered
        if (textureData.empty())
            throw Exception("No character data was generated");

        // Shift all characters up by the lowest ascend in all the font's characters, this ensures all characters render
        // above their local origin
        verticalOffsetToOrigin_ = -lowestAscend;
        for (auto& character : characters_)
            character.ascend_ += verticalOffsetToOrigin_;

        // Setup image description
        auto height = textureData.size() / textureSize;
        if (!Math::isPowerOfTwo(height))
            height = Math::getNextPowerOfTwo(height);

        auto image = Image();
        if (!image.initialize(textureSize, height, 1, Image::RGBA8, false, 1))
            throw Exception("Failed initializing image for the font data");

        memcpy(image.getDataForFrame(0), textureData.getData(), textureData.getDataSize());

        image.flipVertical();

        // Load font texture into a texture object
        textureDimensions_ = Vec2(float(image.getWidth()), float(image.getHeight()));
        texture_ = textures().create2DTexture();
        if (!texture_->load(A(FontDirectory) + name_, std::move(image), "Font"))
            throw Exception("Failed creating font texture");

        // Log all the supported Unicode code points to the logfile
        auto codePointNames = characters_.map<UnicodeString>([](const Character& character) {
            auto s = formatCodePoint(character.getCodePoint()).padToLength(8) + " = ";
            s.append(character.getCodePoint());
            return s;
        });

        Logfile::get().writeCollapsibleSection("Font '" + name_ + "' - supported Unicode code points", codePointNames);

        setupFastCharacterIndexLookupTable();

        LOG_INFO << "Loaded system font '" << originalSystemFont_ << "'"
                 << " at " << size << "pt, character count: " << characters_.size()
                 << ", native size: " << maximumCharacterHeight_ << "px";

        return true;
    }
    catch (const Exception& e)
    {
        if (ftFace)
            FT_Done_Face(ftFace);
        if (ftLibrary)
            FT_Done_FreeType(ftLibrary);

        LOG_ERROR << name << " - " << e;

        clear();

        return false;
    }

#else

    LOG_ERROR << "Support for FreeType was not included in the build, system fonts can't be loaded";
    return false;

#endif
}

void Font::setupFastCharacterIndexLookupTable()
{
    for (auto i = 0U; i < fastCharacterIndexLookup_.size(); i++)
    {
        fastCharacterIndexLookup_[i] = -1;

        for (auto j = 0U; j < characters_.size(); j++)
        {
            if (characters_[j].getCodePoint() == i)
            {
                fastCharacterIndexLookup_[i] = j;
                break;
            }
        }
    }
}

UnicodeString Font::formatCodePoint(UnicodeCharacter c)
{
#ifdef CARBON_LITTLE_ENDIAN
    Endian::convert(c);
#endif

    return "U+" + UnicodeString::toHex(c).trimmedLeft("0").prePadToLength(4, '0');
}

// Initialize the default code points at startup to include all the printable characters from Windows-1250 and
// Windows-1252. Client applications can alter the set of code points included in a font by using
// Font::setDefaultCodePoints() and/or the characters parameter of Font::loadFromSystemFont().
static void initializeDefaultCodePoints()
{
    auto defaultCodePoints = std::array<UnicodeCharacter, 271>{
        {32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
         60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87,
         88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
         112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,

         // Additional characters that occur in either Windows-1250 or Windows-1252
         160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181,
         182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203,
         204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225,
         226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
         248, 249, 250, 251, 252, 253, 254, 255, 258, 259, 260, 261, 262, 263, 268, 269, 270, 271, 272, 273, 280, 281,
         282, 283, 313, 314, 317, 318, 321, 322, 323, 324, 327, 328, 336, 337, 338, 339, 340, 341, 344, 345, 346, 347,
         350, 351, 352, 353, 354, 355, 356, 357, 366, 367, 368, 369, 376, 377, 378, 379, 380, 381, 382, 402, 710, 711,
         728, 729, 731, 732, 733, 8211, 8212, 8216, 8217, 8218, 8220, 8221, 8222, 8224, 8225, 8226, 8230, 8240, 8249,
         8250, 8364, 8482}};

    for (auto codePoint : defaultCodePoints)
        Font::addDefaultCodePoint(codePoint);
}
CARBON_REGISTER_STARTUP_FUNCTION(initializeDefaultCodePoints, 0)

}
