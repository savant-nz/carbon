/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "CarbonEngine/Scene/GUI/GUIWindow.h"

namespace Carbon
{

/**
 * Combobox GUI item. Has a collection of items that can be selected from a dropdown list. When an item is selected a
 * GUIComboboxItemSelectEvent event is sent.
 */
class CARBON_API GUICombobox : public GUIWindow
{
public:

    GUICombobox();
    ~GUICombobox() override;

    /**
     * Item select event dispatcher for this combobox, this can be used as an instance-specific alternative to handling for
     * GUIComboboxItemSelectEvent globally.
     */
    EventDispatcher<GUICombobox, const GUIComboboxItemSelectEvent&> onItemSelectEvent;

    void initialize(float width, float height, const Vec2& position = Vec2::Zero,
                    const UnicodeString& text = UnicodeString::Empty) override
    {
        GUIWindow::initialize(width, height, position);
    }

    /**
     * Initializer method intended for use by Scene::addEntity<>() and ComplexEntity::addChild<>(), it sets the width, height,
     * position and items of this GUICombobox.
     */
    virtual void initialize(float width, float height, const Vec2& position, const Vector<UnicodeString>& items)
    {
        GUIWindow::initialize(width, height, position);
        setItems(items, 0);
    }

    /**
     * Returns the number of items in this combobox
     */
    unsigned int getItemCount() const { return items_.size(); }

    /**
     * Adds an item to this combobox and returns the index of the new item.
     */
    unsigned int addItem(const UnicodeString& item)
    {
        items_.append(item);
        return items_.size() - 1;
    }

    /**
     * Adds the passed items to this combobox.
     */
    void addItems(const Vector<UnicodeString>& items) { items_.append(items); }

    /**
     * Removes the specified item from this combobox. Returns success flag.
     */
    bool removeItem(unsigned int index);

    /**
     * Removes all items from this combobox.
     */
    void clearItems() { setItems({}, -1); }

    /**
     * Clears all the items in this combobox and replaces them with the given item list.
     */
    void setItems(const Vector<UnicodeString>& items, int selectedItem);

    /**
     * Returns the index of the currently selected item. -1 means there is no selection.
     */
    int getSelectedItem() const { return selectedItem_; }

    /**
     * Sets the selected item in this combobox by index. An index of -1 means there is no selection. Returns success flag. If
     * this method changes the current selection then GUIComboboxSelectEvent will be sent, unless \a fireEvent is false.
     */
    bool setSelectedItem(int index, bool fireEvent = true);

    /**
     * This sets the text of the combobox as long as the given string is already an item in the combobox. If this method changes
     * the current selection then GUIComboboxSelectEvent will be sent, unless \a fireEvent is false.
     */
    virtual void setText(const UnicodeString& text, bool fireEvent);

    bool isInteractive() const override { return true; }
    Color getTextColor() const override;
    Color getFillColor() const override;
    bool processEvent(const Event& e) override;
    void autosize() override;
    bool intersect(const Entity* entity) const override { return GUIWindow::intersect(entity); }
    bool intersect(const Vec2& position) const override;
    bool intersect(const Vec3& position) const override { return GUIWindow::intersect(position); }
    void setText(const UnicodeString& text) override { setText(text, true); }

    void clear() override;
    bool gatherGeometry(GeometryGather& gather) override;
    void save(FileWriter& file) const override;
    void load(FileReader& file) override;

private:

    Vector<UnicodeString> items_;
    int selectedItem_ = -1;
    bool isExpanded_ = false;

    float getDropdownWidth() const;
    float getDropdownHeight() const;
    Vec2 getDropdownPosition() const;
    int getMouseoverItem() const;
    float getArrowBoxWidth() const;

    void updateLines() override;
    Rect getTextMarginsToUse() const override;
};

}
