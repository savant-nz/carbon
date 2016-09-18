/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Math/Vec2.h"
#include "CarbonEngine/Render/GeometryChunk.h"
#include "CarbonEngine/Render/Renderer.h"

namespace Carbon
{

/**
 * Manages a single bitmap font object, including glyph sizing, creation from a system font, and saving/loading of font
 * data. The image used to render a font is stored in a standard PNG image file. The FreeType library is used to render
 * the glyphs.
 */
class CARBON_API Font : private Noncopyable
{
public:

    /**
     * Copy constructor (not implemented).
     */
    Font(const Font& other);

    /**
     * The directory which fonts are stored under, currently "Fonts/".
     */
    static const UnicodeString FontDirectory;

    /**
     * The file extension for fonts, currently ".font". Fonts also have a separate PNG file that holds their texture
     * data.
     */
    static const UnicodeString FontExtension;

    /**
     * The character to display instead when attempting to display a character that is not supported by this font. This
     * is always the question mark character: U+003F.
     */
    static const UnicodeCharacter FallbackCharacter;

    /**
     * Text alignment values.
     */
    enum TextAlignment
    {
        AlignTopLeft,
        AlignTopCenter,
        AlignTopRight,
        AlignCenterLeft,
        AlignCenter,
        AlignCenterRight,
        AlignBottomLeft,
        AlignBottomCenter,
        AlignBottomRight,
        AlignLast
    };

    /**
     * Holds size and position data on where each character is stored in the main font texture, as well as alignment
     * details to use when rendering the character.
     */
    class Character
    {
    public:

        /**
         * Returns the Unicode code point that this character is for.
         */
        UnicodeCharacter getCodePoint() const { return codePoint_; }

        /**
         * Returns the character position in texels.
         */
        const Vec2& getPosition() const { return position_; }

        /**
         * Returns the character width in texels.
         */
        float getWidth() const { return width_; }

        /**
         * Returns the character height in texels.
         */
        float getHeight() const { return height_; }

        /**
         * Returns the horizontal offset in texels to apply before rendering the character.
         */
        float getPreMove() const { return preMove_; }

        /**
         * Returns the horizontal offset in texels to apply after rendering the character.
         */
        float getPostMove() const { return postMove_; }

        /**
         * Returns the vertical offset in texels required to move this character up to the font's origin line. This
         * offset is lower for characters with tails such as 'y' and 'g' and higher for those without tails such as 'e'
         * and 'o'.
         */
        float getAscend() const { return ascend_; }

        /**
         * Saves this character to a file stream. Throws an Exception if an error occurs.
         */
        void save(FileWriter& file) const;

        /**
         * Loads this character from a file stream. Throws an Exception if an error occurs.
         */
        void load(FileReader& file);

        /**
         * Constructs this character instance with the specified Unicode code point.
         */
        Character(UnicodeCharacter codePoint = 0) : codePoint_(codePoint) {}

    private:

        UnicodeCharacter codePoint_ = 0;
        Vec2 position_;
        float width_ = 0.0f;
        float height_ = 0.0f;
        float preMove_ = 0.0f;
        float postMove_ = 0.0f;
        float ascend_ = 0.0f;

        friend class Font;
    };

    Font() { clear(); }
    ~Font() { clear(); }

    /**
     * Clears the contents of this font.
     */
    void clear();

    /**
     * Returns the name of this font.
     */
    const String& getName() const { return name_; }

    /**
     * This returns how tall the tallest charcter in this font is measured in the pixels of the underlying bitmap font
     * texture. This is useful because when rendering small fonts the quality is improved by maintaining a 1:1 mapping
     * from pixels in the bitmap font texture to pixels in the rendered image.
     */
    float getMaximumCharacterHeightInPixels() const { return maximumCharacterHeight_; }

    /**
     * Returns the width of the widest character in this font when rendered at the given font size.
     */
    float getMaximumCharacterWidth(float size) const;

    /**
     * Returns the Y offset to the origin line for this font when rendered at the given font size. The origin line is
     * the line that characters such as 'e' sit above and characters with tails such as 'g' and 'y' dip underneath.
     */
    float getVerticalOffsetToOrigin(float size) const;

    /**
     * Returns the width of the given character when rendered using this font at the given size.
     */
    float getWidth(UnicodeCharacter character, float size) const;

    /**
     * Returns the width of the given string when rendered using this font at the given size.
     */
    float getWidth(const UnicodeString& text, float size) const;

    /**
     * Returns the height of the given string when rendered using this font at the given size.
     */
    float getHeight(const UnicodeString& text, float size) const;

    /**
     * Returns the size of the pre-move for a specific character when rendered at the given font size.
     */
    float getCharacterPreMove(UnicodeCharacter codePoint, float size) const;

    /**
     * Returns the metrics for the requested character, this will fall back to Font::FallbackCharacter if the requested
     * character isn't supported by this font.
     */
    const Character& getCharacter(UnicodeCharacter codePoint) const;

    /**
     * Returns the index for the requested character into the characters array, or -1 if it isn't supported by this
     * font.
     */
    int getCharacterIndex(UnicodeCharacter codePoint) const;

    /**
     * Returns the internal list of characters for this font.
     */
    const Vector<Character>& getCharacters() const { return characters_; }

    /**
     * Returns the font's texture.
     */
    const Texture2D* getTexture() const { return texture_; }

    /**
     * Returns whether each character in this font will be forcibly aligned to a pixel boundary during rendering. Some
     * fonts need to map accurately to individual pixels in order to render correctly, this particularly applies to
     * fonts that are rendered at a lower resolution and have thin stems on the characters. However, this alignment can
     * reduce the fluidity of smooth motion text and so may need to be disabled when smooth text motion is needed and
     * the font does not require the pixel alignment alignment in order to render acceptably.
     */
    bool getAlignCharactersToPixelBoundaries() const { return alignCharactersToPixelBoundaries_; }

    /**
     * Sets whether each character in this font will be forcibly aligned to a pixel boundary during rendering. See
     * Font::getAlignCharactersToPixelBoundaries() for details.
     */
    void setAlignCharactersToPixelBoundaries(bool align) { alignCharactersToPixelBoundaries_ = align; }

    /**
     * Prepares this font for rendering by initializing its font texture and creating the character geometry used to
     * render the font. The font texture image will be loaded by the texture load thread if possible.
     */
    bool setup();

    /**
     * Returns the geometry chunk for this font.
     */
    const GeometryChunk& getGeometryChunk() const { return geometryChunk_; }

    /**
     * Saves this font to a font file and font texture.
     */
    bool save() const;

    /**
     * Loads a font from a font file and font texture.
     */
    bool load(const String& name);

    /**
     * Returns whether this font is ready for rendering, i.e. it has a texture and geometry data that are ready for use.
     * Fonts that failed on loading will return false from this method.
     */
    bool isReadyForUse() const { return texture_ && geometryChunk_.getVertexCount(); }

    /**
     * On Windows and Mac OS X this method loads a system font into this font object, sampled at the specified size.
     * Returns success flag.
     *
     * The \a codePoints parameter specifies which Unicode code points will be included in the created font. If this is
     * left blank then a default set of code points will be used. By default this set has 271 characters which are all
     * the printable characters in the Windows-1250 and Windows-1252 code pages, meaning the following languages are
     * supported: Afrikaans, Albanian, Basque, Catalan, Croatian, Czech, Danish, Dutch, English, Faroese, Finnish,
     * French, Galician, German, Hungarian, Icelandic, Indonesian, Italian, Malay, Norwegian, Polish, Portuguese,
     * Romanian, Slovak, Slovenian, Spanish, Swahili and Swedish. This default set can be retrieved and altered using
     * the Font::getDefaultCodePoints() and Font::setDefaultCodePoints() methods.
     *
     * If a font needs to support languages other than those listed above then \a codePoints should be set appropriately
     * to include the Unicode code points that are needed. The only non-optional code point is the question mark
     * (U+003F), because it is used as a fallback when attempting to render an unsupported character. See
     * Font::FallbackCharacter for details.
     */
    bool loadFromSystemFont(const UnicodeString& name, unsigned int size, const UnicodeString& codePoints = {},
                            unsigned int textureSize = 512);

    /**
     * Returns the set of code points that will be included in a new font if the \a codePoints parameter to
     * Font::loadFromSystemFont() is empty. By default this includes all 271 printable characters in the Windows-1250
     * and Windows-1252 code pages.
     */
    static const UnicodeString& getDefaultCodePoints() { return defaultCodePoints_; }

    /**
     * Sets the code points that will be included in a new font if the \a codePoints parameter to
     * Font::loadFromSystemFont() is empty. By default this set includes all 271 printable characters in the
     * Windows-1250 and Windows-1252 code pages.
     */
    static void addDefaultCodePoint(UnicodeCharacter codePoint) { defaultCodePoints_.append(codePoint); }

private:

    String name_;
    UnicodeString originalSystemFont_;
    float maximumCharacterHeight_ = 0.0f;
    float maximumCharacterWidth_ = 0.0f;
    float verticalOffsetToOrigin_ = 0.0f;
    bool alignCharactersToPixelBoundaries_ = true;

    Vector<Character> characters_;    // Sorted by code point

    // The index of code points up to U+0192 in characters_ can be looked up directly via this table, which is faster
    // than searching for them manually in getCharacterIndex(). Code points above U+0192 trigger a search through
    // characters_.
    std::array<int, 0x0193> fastCharacterIndexLookup_ = {};
    void setupFastCharacterIndexLookupTable();

    // Font geometry and texture used in rendering
    GeometryChunk geometryChunk_;
    Texture2D* texture_ = nullptr;

    // The texture dimensions are stored in the .font file to avoid having to load the font texture on the main thread
    // when setting up the font geometry in Font::setup()
    Vec2 textureDimensions_;

    static UnicodeString defaultCodePoints_;
    static UnicodeString formatCodePoint(UnicodeCharacter c);

    friend class FontManager;

    mutable unsigned int referenceCount_ = 0;
};

}
