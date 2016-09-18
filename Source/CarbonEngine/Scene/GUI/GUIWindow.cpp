/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "CarbonEngine/Common.h"
#include "CarbonEngine/Core/CoreEvents.h"
#include "CarbonEngine/Core/EventManager.h"
#include "CarbonEngine/Core/VersionInfo.h"
#include "CarbonEngine/Math/MathCommon.h"
#include "CarbonEngine/Math/Ray.h"
#include "CarbonEngine/Platform/FrameTimers.h"
#include "CarbonEngine/Platform/PlatformInterface.h"
#include "CarbonEngine/Platform/ThemeManager.h"
#include "CarbonEngine/Render/FontManager.h"
#include "CarbonEngine/Scene/Camera.h"
#include "CarbonEngine/Scene/GeometryGather.h"
#include "CarbonEngine/Scene/GUI/GUIEvents.h"
#include "CarbonEngine/Scene/GUI/GUIWindow.h"
#include "CarbonEngine/Scene/Material.h"
#include "CarbonEngine/Scene/MaterialManager.h"
#include "CarbonEngine/Scene/Scene.h"

namespace Carbon
{

const auto GUIWindowVersionInfo = VersionInfo(3, 0);

GUIWindow::GUIWindow()
    : onMouseButtonDownEvent(this),
      onMouseButtonUpEvent(this),
      onMouseMoveEvent(this),
      onMouseEnterEvent(this),
      onMouseExitEvent(this),
      onWindowPressedEvent(this),
      onGainFocusEvent(this),
      onLoseFocusEvent(this),
      onSizeChangedEvent(this)
{
    clear();
    events().addHandler<ResizeEvent>(this);
}

GUIWindow::~GUIWindow()
{
    onDestruct();
    clear();
    events().removeHandler(this);
}

void GUIWindow::clear()
{
    isEnabled_ = true;
    setSize(0.0f, 0.0f);
    centerOnLocalOrigin_ = false;
    borderSize_ = 1.0f;
    alignToScreen_ = false;
    alignScreenLocation_ = ScreenLocationNone;
    alignOffset_ = Vec2::Zero;
    material_.clear();
    hoverMaterial_.clear();
    useCustomFillColor_ = false;
    useCustomBorderColor_ = false;
    useCustomTextColor_ = false;
    fillColor_ = Color(0.5f, 0.5f, 0.5f, 0.5f);
    borderColor_ = Color::White;
    textColor_ = Color::White;
    text_.clear();
    textMargins_ = Rect::Zero;
    isWordWrapEnabled_ = true;
    textAlignment_ = Font::AlignTopLeft;
    fonts().releaseFont(font_);
    font_ = nullptr;
    fontSize_ = 0.0f;
    hasFocus_ = false;
    isMouseInWindow_ = false;
    isDraggable_ = false;
    dragTouchID_ = 0;
    setIsBeingDragged(false);
    isResizable_ = false;
    isBeingResized_ = false;

    lines_.clear();
    areLinesCurrent_ = false;

    ComplexEntity::clear();
}

void GUIWindow::initialize(float width, float height, const Vec2& position, const UnicodeString& text)
{
    setSize(width, height);
    setWorldPosition(position);
    setText(text);

    // Set left and right text margins at 6 pixels, this is only done if there is no default camera, in which case we
    // can be sure that one unit equates to one pixel
    if (getScene() && !getScene()->getDefaultCamera())
        setTextMargins({6.0f, 0.0f, 6.0f, 0.0f});

    // Autosize if no size was specified and there is some text
    if (width == 0.0f && height == 0.0f && text.length())
        autosize();
}

void GUIWindow::intersectRay(const Ray& ray, Vector<IntersectionResult>& intersections, bool onlyWorldGeometry)
{
    if (!isVisibleIgnoreAlpha())
        return;

    // Transform the ray into local space
    auto localRay = getWorldTransform().getInverse() * ray;

    // Intersect the ray with this window as a plane
    auto t = 0.0f;
    if (Plane(Vec3::Zero, Vec3::UnitZ).intersect(localRay, t))
    {
        // See if the intersection point is inside this window's rectangle
        auto intersectionPoint = localRay.getPoint(t);
        if (intersect(localToWorld(intersectionPoint).toVec2()))
        {
            // If the alpha is zero at the point of intersection then don't report it
            auto surfaceColor = getSurfaceColor(intersectionPoint.toVec2());
            if (surfaceColor.a > 0.0f)
            {
                auto material = getMaterialRoot() + getMaterial();

                if (!onlyWorldGeometry || (getScene() && getScene()->isWorldGeometryMaterial(material)))
                {
                    intersections.emplace(t, ray.getPoint(t), getWorldOrientation().getZVector(), this, material,
                                          surfaceColor);
                }
            }
        }
    }

    ComplexEntity::intersectRay(ray, intersections, onlyWorldGeometry);
}

bool GUIWindow::gatherGeometry(GeometryGather& gather)
{
    if (!ComplexEntity::gatherGeometry(gather))
        return false;

    if (shouldProcessGather(gather))
    {
        if (!isCulledBy(gather) && width_ > 0.0f && height_ > 0.0f)
        {
            gather.changePriority(getRenderPriority());
            gather.changeTransformation(getWorldTransform() * -localToWindow(Vec3::Zero), getWorldOrientation());

            if (material_.length())
            {
                auto material = getMaterialRoot();

                if (hoverMaterial_.length() && isMouseInWindow() && isEnabled())
                    material << hoverMaterial_;
                else
                    material << material_;

                auto overrideParameters = getMaterialOverrideParameters(material);

                gather.changeMaterial(material, overrideParameters);
                gather.addRectangle(width_, height_);
            }
            else
            {
                queueWindow(gather, width_, height_, borderSize_, getFillColor(), getBorderColor());

                // If this window is resizable then draw a resize handle in the bottom left corner
                if (isResizable())
                {
                    gather.addImmediateTriangles(1);
                    gather.addImmediateTriangle(Vec3(width_ - getResizeHandleSize(), getBorderSize()),
                                                Vec3(width_ - getBorderSize(), getBorderSize()),
                                                Vec3(width_ - getBorderSize(), getResizeHandleSize()),
                                                getBorderColor());
                }
            }

            // Update the lines if needed
            if (!areLinesCurrent_)
            {
                updateLines();
                positionLines();
                areLinesCurrent_ = true;
            }

            // Render any text on this window
            if (lines_.size())
            {
                gather.changePriority(getRenderPriority() + 1);

                for (auto& line : lines_)
                {
                    if (line.isVisible())
                        queueText(gather, line.getPosition(), line.getText(), getTextColor());
                }
            }
        }
    }

    return true;
}

void GUIWindow::precache()
{
    if (getMaterial().length())
        materials().getMaterial(getMaterial()).precache();

    ComplexEntity::precache();
}

bool GUIWindow::invalidateWorldTransform(const String& attachmentPoint)
{
    if (ComplexEntity::invalidateWorldTransform(attachmentPoint))
    {
        // For interactive windows we need to fire mouse enter and exit events whenever the window is moved, at present
        // this gets deferred to an UpdateEvent handler to avoid always immediately recalculating the world transform
        // after it has just been invalidated, which probably isn't great for performance. The world transform will
        // still be calculated in the UpdateEvent handler that fires, but it means it happens at most once per frame.
        if (isInteractive())
            events().addHandler<UpdateEvent>(this);

        return true;
    }

    return false;
}

Color GUIWindow::getSurfaceColor(const Vec2& localPosition) const
{
    auto u = localPosition.x / getWidth();
    auto v = localPosition.y / getHeight();

    if (centerOnLocalOrigin_)
    {
        u += 0.5f;
        v += 0.5f;
    }

    // Everywhere outside of the sprite returns a zero color
    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
        return Color::Zero;

    if (material_.length())
    {
        auto surfaceColor = Color();
        if (!materials().getMaterial(material_).sampleTexture("diffuseMap", u, v, surfaceColor))
        {
            LOG_ERROR << "Failed sampling material's texture";
            return Color::Zero;
        }

        return adjustColorAlpha(surfaceColor);
    }

    auto normalizedBorderSize = Vec2(getBorderSize()) / Vec2(getWidth(), getHeight());

    if (u <= normalizedBorderSize.x || u >= 1.0f - normalizedBorderSize.x || v <= normalizedBorderSize.y ||
        v >= 1.0f - normalizedBorderSize.y)
        return getBorderColor();

    return getFillColor();
}

void GUIWindow::setEnabled(bool enabled)
{
    isEnabled_ = enabled;

    if (!enabled)
    {
        setIsBeingDragged(false);
        isBeingResized_ = false;
    }
}

void GUIWindow::setSize(float width, float height)
{
    width_ = std::max(width, 0.0f);
    height_ = std::max(height, 0.0f);
    areLinesCurrent_ = false;

    if (alignToScreen_)
        alignToScreen(alignScreenLocation_, alignOffset_);

    onLocalAABBChanged();

    onSizeChangedEvent.fire(Vec2(width, height));
}

Color GUIWindow::getFillColor() const
{
    if (useCustomFillColor_)
        return adjustColorAlpha(fillColor_);

    if (isEnabled() && isInteractive() && isMouseInWindow())
        return adjustColorAlpha(theme()["HoverFillColor"]);

    return adjustColorAlpha(theme()["FillColor"]);
}

void GUIWindow::setFillColor(const Color& color)
{
    useCustomFillColor_ = true;
    fillColor_ = color;
}

Color GUIWindow::getBorderColor() const
{
    return adjustColorAlpha(useCustomBorderColor_ ? borderColor_ : theme()["BorderColor"]);
}

void GUIWindow::setBorderColor(const Color& color)
{
    useCustomBorderColor_ = true;
    borderColor_ = color;
}

Color GUIWindow::getTextColor() const
{
    return adjustColorAlpha(useCustomTextColor_ ? textColor_ : theme()["TextColor"]);
}

void GUIWindow::setTextColor(const Color& color)
{
    useCustomTextColor_ = true;
    textColor_ = color;
}

void GUIWindow::useThemeColors()
{
    useCustomFillColor_ = false;
    useCustomBorderColor_ = false;
    useCustomTextColor_ = false;
}

void GUIWindow::setText(const UnicodeString& text)
{
    text_ = text;
    areLinesCurrent_ = false;
}

void GUIWindow::autosize()
{
    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);

    auto originalWidth = getWidth();

    // Set a maximum width and re-evaluate the set of text lines
    setWidth(FLT_MAX);
    updateLines();

    // Determine total margins in the X and Y directions
    auto margins = getTextMarginsToUse();
    auto horizontalTextMargin = margins.getLeft() + margins.getRight() + Math::Epsilon;
    auto verticalTextMargin = margins.getLeft() + margins.getRight() + Math::Epsilon;

    // Set the height based on the number of lines
    setHeight(verticalTextMargin + std::max(lines_.size(), 1U) * fontSize);

    // Set the width based on the longest line
    setWidth(horizontalTextMargin);
    for (auto& line : lines_)
    {
        auto lineWidth = horizontalTextMargin + getFontToUse()->getWidth(line.getText(), fontSize);
        if (lineWidth > getWidth())
            setWidth(lineWidth);
    }

    // Move window so the text stays anchored in the same place
    if (textAlignment_ == Font::AlignTopRight || textAlignment_ == Font::AlignCenterRight ||
        textAlignment_ == Font::AlignBottomRight)
        move({originalWidth - getWidth(), 0.0f, 0.0f});
    else if (textAlignment_ == Font::AlignTopCenter || textAlignment_ == Font::AlignCenter ||
             textAlignment_ == Font::AlignBottomCenter)
        move({(originalWidth - getWidth()) * 0.5f, 0.0f, 0.0f});
}

void GUIWindow::setTextMargins(const Rect& margins)
{
    textMargins_ = margins;
    areLinesCurrent_ = false;
}

void GUIWindow::setWordWrapEnabled(bool enabled)
{
    if (enabled != isWordWrapEnabled_)
    {
        isWordWrapEnabled_ = enabled;
        areLinesCurrent_ = false;
    }
}

void GUIWindow::setTextAlignment(Font::TextAlignment alignment)
{
    textAlignment_ = alignment;
    areLinesCurrent_ = false;
}

void GUIWindow::setFont(const String& font)
{
    fonts().releaseFont(font_);
    font_ = nullptr;

    if (font.length())
        font_ = fonts().setupFont(font);

    areLinesCurrent_ = false;
}

void GUIWindow::setFontSize(float size)
{
    if (size < 0.0f)
        return;

    fontSize_ = size;
    areLinesCurrent_ = false;
}

bool GUIWindow::isMouseInWindow() const
{
    if (isInteractive())
        return isMouseInWindow_;

    return intersect(screenToWorld(platform().getMousePosition()));
}

bool GUIWindow::isTouchInWindow() const
{
    return platform().getTouches().has([&](const Vec2& touch) { return intersect(screenToWorld(touch)); });
}

void GUIWindow::setDraggable(bool draggable)
{
    isDraggable_ = draggable;
    if (!draggable)
        setIsBeingDragged(false);
}

void GUIWindow::setResizable(bool resizable)
{
    isResizable_ = resizable;
    isBeingResized_ = false;
}

bool GUIWindow::intersect(const Vec2& position) const
{
    auto windowPosition = localToWindow(worldToLocal(position));

    return windowPosition.x > 0.0f && windowPosition.x < getWidth() && windowPosition.y > 0.0f &&
        windowPosition.y < getHeight();
}

bool GUIWindow::processEvent(const Event& e)
{
    if (isEnabled() && isVisibleIgnoreAlpha())
    {
        if (auto mbde = e.as<MouseButtonDownEvent>())
        {
            auto mousePosition = screenToWorld(mbde->getPosition());
            if (intersect(mousePosition))
            {
                auto localPosition = worldToLocal(mousePosition).toVec2();
                auto windowPosition = localToWindow(localPosition);

                if (isResizable_)
                {
                    auto isMouseInResizeHandle = windowPosition.y > 0.0f && getWidth() - windowPosition.x > 0.0f &&
                        getWidth() - windowPosition.x + windowPosition.y <= getResizeHandleSize();

                    if (mbde->getButton() == LeftMouseButton && isMouseInResizeHandle)
                    {
                        isBeingResized_ = true;
                        resizeOrigin_ = mousePosition;
                    }
                    else
                        isBeingResized_ = false;
                }

                // If this window is draggable and a drag can be started from the curent window-local mouse position
                // then enter a drag
                if (isDraggable_ && allowDragEnter(localPosition) && !isBeingResized_)
                {
                    if (mbde->getButton() == LeftMouseButton)
                    {
                        setIsBeingDragged(true);
                        dragOrigin_ = mousePosition;
                    }
                    else
                        setIsBeingDragged(false);
                }

                // Translate into GUIMouseButtonDownEvent
                auto event = GUIMouseButtonDownEvent(this, mbde->getButton(), mousePosition, localPosition);
                onBeforeGUIMouseButtonDownEvent(event);
                if (!events().dispatchEvent(event))
                    return false;
                onMouseButtonDownEvent.fire(event);

                auto pressedEvent = GUIWindowPressedEvent(this, localPosition);
                if (!events().dispatchEvent(pressedEvent))
                    return false;
                onWindowPressedEvent.fire(pressedEvent);
            }
            else
                isBeingResized_ = false;
        }
        else if (auto mbue = e.as<MouseButtonUpEvent>())
        {
            isBeingResized_ = false;

            auto mousePosition = screenToWorld(mbue->getPosition());
            if (intersect(mousePosition))
            {
                auto localPosition = worldToLocal(mousePosition).toVec2();

                setIsBeingDragged(false);

                auto event = GUIMouseButtonUpEvent(this, mbue->getButton(), mousePosition, localPosition);
                if (!events().dispatchEvent(event))
                    return false;
                onMouseButtonUpEvent.fire(event);
            }
        }
        else if (auto mme = e.as<MouseMoveEvent>())
        {
            auto mousePosition = screenToWorld(mme->getPosition());
            auto localPosition = worldToLocal(mousePosition).toVec2();

            // Update window size if it is being resized
            if (isBeingResized_)
            {
                auto newWidth = getWidth() + (mousePosition.x - resizeOrigin_.x);
                auto newHeight = getHeight() - (mousePosition.y - resizeOrigin_.y);

                if (newWidth > getResizeHandleSize())
                {
                    setWidth(newWidth);
                    resizeOrigin_.x = mousePosition.x;
                }

                if (newHeight > getResizeHandleSize())
                {
                    move({0.0f, getHeight() - newHeight});
                    setHeight(newHeight);
                    resizeOrigin_.y = mousePosition.y;
                }
            }

            // Update window position if it is being dragged
            if (isBeingDragged_)
            {
                move(mousePosition - dragOrigin_);
                dragOrigin_ = mousePosition;
            }

            // Send out GUI events based on the received mouse move event
            if (isMouseInWindow())
            {
                // Translate into GUIMouseMoveEvent
                auto event = GUIMouseMoveEvent(this, mousePosition, localPosition);
                events().dispatchEvent(event);
                onMouseMoveEvent.fire(event);
            }
        }
        else if (auto tbe = e.as<TouchBeginEvent>())
        {
            auto touchPosition = screenToWorld(tbe->getPosition());

            if (intersect(touchPosition))
            {
                auto localPosition = worldToLocal(touchPosition).toVec2();

                if (isDraggable() && !isBeingDragged() && allowDragEnter(worldToLocal(touchPosition).toVec2()))
                {
                    setIsBeingDragged(true);

                    dragOrigin_ = touchPosition;
                    dragTouchID_ = tbe->getTouchID();
                }

                auto pressedEvent = GUIWindowPressedEvent(this, localPosition);
                if (!events().dispatchEvent(pressedEvent))
                    return false;
                onWindowPressedEvent.fire(pressedEvent);
            }
        }
        else if (auto tee = e.as<TouchEndEvent>())
        {
            if (isDraggable())
            {
                if (tee->getTouchID() == dragTouchID_)
                    setIsBeingDragged(false);
            }
        }
        else if (auto tme = e.as<TouchMoveEvent>())
        {
            if (isDraggable())
            {
                // Update window position if it is being dragged
                if (isBeingDragged() && tme->getTouchID() == dragTouchID_)
                {
                    auto touchPosition = screenToWorld(tme->getPosition());

                    move(touchPosition - dragOrigin_);
                    dragOrigin_ = touchPosition;
                }
            }
        }
    }

    if (e.as<ResizeEvent>())
    {
        if (alignToScreen_)
            alignToScreen(alignScreenLocation_, alignOffset_);
    }
    else if (e.as<UpdateEvent>())
    {
        doMouseEnterExitHandling(platform().getMousePosition());

        events().removeHandler<UpdateEvent>(this);
    }

    return true;
}

void GUIWindow::alignToScreen(ScreenLocation location, const Vec2& offset)
{
    alignToScreen_ = (location != ScreenLocationNone);
    alignScreenLocation_ = location;
    alignOffset_ = offset;

    if (!alignToScreen_ || !getScene())
        return;

    auto position = Vec2();

    auto entityExtents = AABB(getLocalAABB(), {Vec3::Zero, getWorldOrientation()});

    auto width = entityExtents.getWidth();
    auto height = entityExtents.getHeight();

    auto orthographicRect = getScene()->getDefaultCameraOrthographicRect();

    switch (location)
    {
        case ScreenBottomLeft:
            position.setXY(0.0f, 0.0f);
            break;
        case ScreenMiddleLeft:
            position.setXY(0.0f, (orthographicRect.getHeight() - height) * 0.5f);
            break;
        case ScreenTopLeft:
            position.setXY(0.0f, orthographicRect.getHeight() - height);
            break;
        case ScreenBottomMiddle:
            position.setXY((orthographicRect.getWidth() - width) * 0.5f, 0.0f);
            break;
        case ScreenMiddle:
            position.setXY((orthographicRect.getWidth() - width) * 0.5f,
                           (orthographicRect.getHeight() - height) * 0.5f);
            break;
        case ScreenTopMiddle:
            position.setXY((orthographicRect.getWidth() - width) * 0.5f, orthographicRect.getHeight() - height);
            break;
        case ScreenBottomRight:
            position.setXY(orthographicRect.getWidth() - width, 0.0f);
            break;
        case ScreenMiddleRight:
            position.setXY(orthographicRect.getWidth() - width, (orthographicRect.getHeight() - height) * 0.5f);
            break;
        case ScreenTopRight:
            position.setXY(orthographicRect.getWidth() - width, orthographicRect.getHeight() - height);
            break;
        default:
            break;
    }

    setWorldPosition(position + -entityExtents.getMinimum().toVec2() + offset +
                     Vec2(orthographicRect.getLeft(), orthographicRect.getBottom()));
}

void GUIWindow::setCenteredOnLocalOrigin(bool center)
{
    centerOnLocalOrigin_ = center;
    onLocalAABBChanged();
}

float GUIWindow::getRotationAroundCenter() const
{
    auto x = getWorldOrientation().getXVector();

    return atan2f(-x.y, x.x);
}

void GUIWindow::setRotationAroundCenter(float radians)
{
    auto delta = radians - getRotationAroundCenter();

    if (centerOnLocalOrigin_)
        rotateAroundZ(delta);
    else
        rotateAroundCenter(delta);
}

void GUIWindow::rotateAroundCenter(float radians)
{
    if (centerOnLocalOrigin_)
        rotateAroundZ(radians);
    else
        rotateAroundPoint(localToWorld(Vec3(getWidth() * 0.5f, getHeight() * 0.5f)),
                          Quaternion::createRotationZ(radians));
}

void GUIWindow::setIsBeingDragged(bool dragged)
{
    if (isBeingDragged_ == dragged)
        return;

    if (dragged)
        onEnterDrag();
    else
        onExitDrag();

    isBeingDragged_ = dragged;
    dragOrigin_ = Vec2::Zero;
    dragTouchID_ = 0;
}

void GUIWindow::updateLines()
{
    lines_.clear();

    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);
    auto textMargins = getTextMarginsToUse();

    // Work out the lines of text, wrap round, recognize newline characters etc...
    auto currentLine = UnicodeString();
    auto currentLineWidth = 0.0f;

    auto maxWidth = getWidth() - textMargins.getLeft() - textMargins.getRight();

    for (auto i = 0U; i < text_.length(); i++)
    {
        // Get the next word
        auto wordIndex = text_.findFirstOf(" \n", i);
        if (wordIndex == -1)
            wordIndex = text_.length() - 1;

        // Get the length of the word
        auto word = text_.substr(i, wordIndex - i + 1);
        auto newline = (word.back() == '\n');
        if (newline)
            word.resize(word.length() - 1);

        auto wordWidth = font->getWidth(word, fontSize);

        if (!isWordWrapEnabled_ || currentLineWidth + wordWidth <= maxWidth || currentLine.length() == 0)
        {
            currentLine << word;
            currentLineWidth += wordWidth;

            if (currentLineWidth > maxWidth)
            {
                // If this happens then the line is now too long to fit in the window so it has to be clipped off to
                // ensure that it doesn't spill over the edge

                auto clippedLine = UnicodeString();
                auto newWidth = 0.0f;
                for (auto j = 0U; j < currentLine.length(); j++)
                {
                    auto charWidth = font->getWidth(currentLine.at(j), fontSize);
                    if (newWidth + charWidth > maxWidth)
                        break;

                    newWidth += charWidth;
                    clippedLine.append(currentLine.at(j));
                }

                currentLine = clippedLine;
                currentLineWidth = newWidth;
            }
        }
        else
        {
            if (currentLine.back() == ' ')
                currentLine.resize(currentLine.length() - 1);

            lines_.append(currentLine);

            currentLineWidth = wordWidth;
            currentLine = word;
        }

        if (newline)
        {
            lines_.append(currentLine);
            currentLineWidth = 0.0f;
            currentLine.clear();
        }
        i = wordIndex;

        if (i == text_.length() - 1)
            lines_.append(currentLine);
    }
}

void GUIWindow::positionLines()
{
    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);
    auto textMargins = getTextMarginsToUse();

    // Align the lines inside this window obeying the margin values
    auto height = getHeight() - textMargins.getBottom() - textMargins.getTop();
    auto width = getWidth() - textMargins.getLeft() - textMargins.getRight();
    if (height < fontSize || width < 0.0f)
        return;

    // Number of lines that can fit inside
    auto maxLineCount = uint(height / fontSize);
    if (maxLineCount == 0)
        return;

    auto totalLinesToRender = (lines_.size() < maxLineCount) ? lines_.size() : maxLineCount;

    auto totalHeight = float(totalLinesToRender) * fontSize;

    // Vertical alignment
    auto verticalOffset = 0.0f;
    if (textAlignment_ == Font::AlignTopLeft || textAlignment_ == Font::AlignTopCenter ||
        textAlignment_ == Font::AlignTopRight)
        verticalOffset = height - totalHeight;
    else if (textAlignment_ == Font::AlignCenterLeft || textAlignment_ == Font::AlignCenter ||
             textAlignment_ == Font::AlignCenterRight)
        verticalOffset = (height - totalHeight) * 0.5f;

    verticalOffset += textMargins.getBottom();

    // Position the lines
    for (auto i = 0U; i < totalLinesToRender; i++)
    {
        auto& line = lines_[i];
        line.setVisible(true);

        auto lineWidth = font->getWidth(line.getText(), fontSize);

        // Horizontal alignment
        auto linePosition = Vec2();
        if (textAlignment_ != Font::AlignTopLeft && textAlignment_ != Font::AlignCenterLeft &&
            textAlignment_ != Font::AlignBottomLeft)
        {
            linePosition.x = width - lineWidth;

            if (textAlignment_ == Font::AlignTopCenter || textAlignment_ == Font::AlignCenter ||
                textAlignment_ == Font::AlignBottomCenter)
                linePosition.x *= 0.5f;
        }

        linePosition.x += textMargins.getLeft();
        linePosition.y = (totalLinesToRender - i - 1) * fontSize + verticalOffset;

        // Adjust for when centerOnLocalOrigin_ is true
        linePosition -= localToWindow(Vec3::Zero);

        line.setPosition(linePosition);
    }
}

const Font* GUIWindow::getFontToUse() const
{
    return font_ ? font_ : fonts().getSystemVariableWidthFont();
}

float GUIWindow::getFontSizeToUse(const Font* font) const
{
    return fontSize_ != 0.0f ? fontSize_ : font->getMaximumCharacterHeightInPixels();
}

void GUIWindow::save(FileWriter& file) const
{
    ComplexEntity::save(file);

    file.beginVersionedSection(GUIWindowVersionInfo);

    file.write(isEnabled_, width_, height_, centerOnLocalOrigin_, borderSize_, alignToScreen_);
    file.writeEnum(alignScreenLocation_);
    file.write(alignOffset_, material_, hoverMaterial_, useCustomFillColor_, useCustomBorderColor_, useCustomTextColor_,
               fillColor_, borderColor_, textColor_, text_, textMargins_, isWordWrapEnabled_);
    file.writeEnum(textAlignment_);
    file.write(font_ ? font_->getName() : String::Empty, fontSize_, isDraggable_, isResizable_);

    file.endVersionedSection();
}

void GUIWindow::load(FileReader& file)
{
    try
    {
        ComplexEntity::load(file);

        file.beginVersionedSection(GUIWindowVersionInfo);

        auto fontName = String();

        file.read(isEnabled_, width_, height_, centerOnLocalOrigin_, borderSize_, alignToScreen_);
        file.readEnum(alignScreenLocation_);
        file.read(alignOffset_, material_, hoverMaterial_, useCustomFillColor_, useCustomBorderColor_,
                  useCustomTextColor_, fillColor_, borderColor_, textColor_, text_, textMargins_, isWordWrapEnabled_);
        file.readEnum(textAlignment_, Font::AlignLast);
        file.read(fontName, fontSize_, isDraggable_, isResizable_);

        file.endVersionedSection();

        setFont(fontName);

        areLinesCurrent_ = false;
    }
    catch (const Exception&)
    {
        clear();
        throw;
    }
}

void GUIWindow::queueWindow(GeometryGather& gather, float width, float height, float borderSize, const Color& fillColor,
                            const Color& borderColor)
{
    gather.changeMaterial("ImmediateGeometry");

    // Queue the two triangles that draw the fill area
    if (fillColor.a > 0.0f)
    {
        gather.addImmediateTriangles(2);

        gather.addImmediateTriangle(Vec3(borderSize, borderSize), Vec3(width - borderSize, borderSize),
                                    Vec3(borderSize, height - borderSize), fillColor);
        gather.addImmediateTriangle(Vec3(borderSize, height - borderSize), Vec3(width - borderSize, borderSize),
                                    Vec3(width - borderSize, height - borderSize), fillColor);
    }

    // Queue the triangles for the border
    if (borderColor.a > 0.0f)
    {
        gather.addImmediateTriangles(8);

        // Bottom
        gather.addImmediateTriangle(Vec3::Zero, Vec3(width, 0.0f), Vec3(0.0f, borderSize), borderColor);
        gather.addImmediateTriangle(Vec3(0.0f, borderSize), Vec3(width, 0.0f), Vec3(width, borderSize), borderColor);

        // Left
        gather.addImmediateTriangle(Vec3(0.0f, borderSize), Vec3(borderSize, borderSize),
                                    Vec3(0.0f, height - borderSize), borderColor);
        gather.addImmediateTriangle(Vec3(0.0f, height - borderSize), Vec3(borderSize, borderSize),
                                    Vec3(borderSize, height - borderSize), borderColor);

        // Right
        gather.addImmediateTriangle(Vec3(width - borderSize, borderSize), Vec3(width, borderSize),
                                    Vec3(width - borderSize, height - borderSize), borderColor);
        gather.addImmediateTriangle(Vec3(width - borderSize, height - borderSize), Vec3(width, borderSize),
                                    Vec3(width, height - borderSize), borderColor);

        // Top
        gather.addImmediateTriangle(Vec3(0.0f, height - borderSize), Vec3(width, height - borderSize),
                                    Vec3(0.0f, height), borderColor);
        gather.addImmediateTriangle(Vec3(0.0f, height), Vec3(width, height - borderSize), Vec3(width, height),
                                    borderColor);
    }
}

void GUIWindow::queueText(GeometryGather& gather, const Vec2& position, const UnicodeString& text, const Color& color)
{
    auto font = getFontToUse();
    auto fontSize = getFontSizeToUse(font);

    gather.changeTransformation(getWorldTransform() *
                                    (position + Vec2(0.0f, -floorf(font->getVerticalOffsetToOrigin(fontSize) * 0.5f))),
                                getWorldOrientation());
    gather.addText(font, fontSize, text, color);
}

bool GUIWindow::isCulledBy(const GeometryGather& gather) const
{
    return !gather.getFrustum().intersect(getWorldAABB());
}

void GUIWindow::setHasFocus(bool hasFocus)
{
    if (hasFocus == hasFocus_)
        return;

    hasFocus_ = hasFocus;

    if (hasFocus)
    {
        auto event = GUIGainFocusEvent(this);
        events().dispatchEvent(event);
        onGainFocusEvent.fire(event);
    }
    else
    {
        auto event = GUILoseFocusEvent(this);
        events().dispatchEvent(event);
        onLoseFocusEvent.fire(event);
    }
}

void GUIWindow::doMouseEnterExitHandling(const Vec2& mousePosition)
{
    auto mouseWorldPosition = screenToWorld(mousePosition);

    if (intersect(mouseWorldPosition) == isMouseInWindow_)
        return;

    if (!isMouseInWindow_)
    {
        isMouseInWindow_ = true;

        // Send GUIMouseEnterEvent
        auto event = GUIMouseEnterEvent(this, mousePosition, worldToLocal(mouseWorldPosition).toVec2());
        events().dispatchEvent(event);
        onMouseEnterEvent.fire(event);
    }
    else
    {
        isMouseInWindow_ = false;

        // Send GUIMouseExitEvent
        auto event = GUIMouseExitEvent(this, mousePosition, worldToLocal(mouseWorldPosition).toVec2());
        events().dispatchEvent(event);
        onMouseExitEvent.fire(event);
    }
}

int GUIWindow::getRenderPriority() const
{
    // GUIWindows with focus return a render priority of a million so that they appear above everything else in the
    // scene
    if (hasFocus())
        return 1000000;

    // If a GUIWindow has not had an explicit render priority set then it defaults to using a render priority that is
    // one greater than that of its parent entity, assuming the parent entity is also a GUIWindow. This means that
    // render priorities will be correct by default in window hierarchies.

    auto actualRenderPriority = ComplexEntity::getRenderPriority();

    if (actualRenderPriority == 0 && getParent() && getParent()->isEntityType<GUIWindow>())
        return getParent()->getRenderPriority() + 1;

    return actualRenderPriority;
}

void GUIWindow::lookAtPoint(const Vec3& p)
{
    auto direction = p - getWorldPosition();

    // Check there is some distance between us and the target point
    if (direction.length() < 0.01f)
        return;

    // Construct rotation
    setWorldOrientation(Quaternion::createRotationZ(atan2f(direction.x, direction.y)));
}

GUIWindow::operator UnicodeString() const
{
    auto info = Vector<UnicodeString>();

    info.append("");
    info.append(UnicodeString() + "width: " + getWidth());
    info.append(UnicodeString() + "height: " + getHeight());

    if (getMaterial().length() && !getMaterial().startsWith(String::Period))
        info.append("material: " + getMaterial());

    if (getText().length())
    {
        info.append("font: " + getFontToUse()->getName());
        info.append(UnicodeString() + "font size: " + getFontSizeToUse(getFontToUse()));

        auto text = getText();
        text.replace("\n", "\\n");
        info.append("text: '" + text + "'");
    }

    return ComplexEntity::operator UnicodeString() << info;
}

Vec2 GUIWindow::screenToWorld(const Vec2& p) const
{
    return getScene() ? getScene()->screenToWorld(p).toVec2() : p;
}

Vec2 GUIWindow::localToWindow(const Vec3& p) const
{
    if (centerOnLocalOrigin_)
        return p.toVec2() + Vec2(getWidth(), getHeight()) * 0.5f;

    return p.toVec2();
}

void GUIWindow::calculateLocalAABB() const
{
    ComplexEntity::calculateLocalAABB();

    if (centerOnLocalOrigin_)
    {
        localAABB_.addPoint({getWidth() * -0.5f, getHeight() * -0.5f, -10.0f});
        localAABB_.addPoint({getWidth() * 0.5f, getHeight() * 0.5f, 10.0f});
    }
    else
    {
        localAABB_.addPoint({0.0f, 0.0f, -10.0f});
        localAABB_.addPoint({getWidth(), getHeight(), 10.0f});
    }
}

}
