/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <iterator>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/windows/Window.h>
#include <openrct2/Editor.h>
#include <openrct2/Input.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/interface/Cursors.h>
#include <openrct2/localisation/Localisation.h>
#include <openrct2/management/Research.h>
#include <openrct2/object/DefaultObjects.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/object/ObjectRepository.h>
#include <openrct2/ride/RideGroupManager.h>
#include <openrct2/sprites.h>
#include <openrct2/util/Util.h>
#include <openrct2/world/Scenery.h>

#pragma region Widgets

constexpr int32_t WW = 600;
constexpr int32_t WH = 400;

// clang-format off
enum {
    WIDX_BACKGROUND,
    WIDX_TITLE,
    WIDX_CLOSE,
    WIDX_RESIZE,
    WIDX_TAB_1,
    WIDX_PRE_RESEARCHED_SCROLL,
    WIDX_RESEARCH_ORDER_SCROLL,
    WIDX_PREVIEW,
    WIDX_MOVE_ITEMS_TO_TOP,
    WIDX_MOVE_ITEMS_TO_BOTTOM,
    WIDX_RANDOM_SHUFFLE
};

static rct_widget window_editor_inventions_list_widgets[] = {
    { WWT_FRAME,            0,  0,      599,    0,      399,    STR_NONE,               STR_NONE                },
    { WWT_CAPTION,          0,  1,      598,    1,      14,     STR_INVENTION_LIST,     STR_WINDOW_TITLE_TIP    },
    { WWT_CLOSEBOX,         0,  587,    597,    2,      13,     STR_CLOSE_X,            STR_CLOSE_WINDOW_TIP    },
    { WWT_RESIZE,           1,  0,      599,    43,     399,    STR_NONE,               STR_NONE                },
    { WWT_TAB,              1,  3,      33,     17,     43,     IMAGE_TYPE_REMAP | SPR_TAB,   STR_NONE          },
    { WWT_SCROLL,           1,  4,      371,    56,     216,    SCROLL_VERTICAL,        STR_NONE                },
    { WWT_SCROLL,           1,  4,      371,    231,    387,    SCROLL_VERTICAL,        STR_NONE                },
    { WWT_FLATBTN,          1,  431,    544,    106,    219,    0xFFFFFFFF,             STR_NONE                },
    { WWT_BUTTON,           1,  375,    594,    343,    356,    STR_MOVE_ALL_TOP,       STR_NONE                },
    { WWT_BUTTON,           1,  375,    594,    358,    371,    STR_MOVE_ALL_BOTTOM,    STR_NONE                },
    { WWT_BUTTON,           1,  375,    594,    373,    386,    STR_RANDOM_SHUFFLE,     STR_RANDOM_SHUFFLE_TIP  },
    { WIDGETS_END }
};

static rct_widget window_editor_inventions_list_drag_widgets[] = {
    { WWT_IMGBTN,           0,  0,      149,    0,      13,     STR_NONE,               STR_NONE                },
    { WIDGETS_END }
};

#pragma endregion

#pragma region Events

static void window_editor_inventions_list_close(rct_window *w);
static void window_editor_inventions_list_mouseup(rct_window *w, rct_widgetindex widgetIndex);
static void window_editor_inventions_list_resize(rct_window *w);
static void window_editor_inventions_list_update(rct_window *w);
static void window_editor_inventions_list_scrollgetheight(rct_window *w, int32_t scrollIndex, int32_t *width, int32_t *height);
static void window_editor_inventions_list_scrollmousedown(rct_window *w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords);
static void window_editor_inventions_list_scrollmouseover(rct_window *w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords);
static void window_editor_inventions_list_cursor(rct_window *w, rct_widgetindex widgetIndex, const ScreenCoordsXY& screenCoords, int32_t *cursorId);
static void window_editor_inventions_list_invalidate(rct_window *w);
static void window_editor_inventions_list_paint(rct_window *w, rct_drawpixelinfo *dpi);
static void window_editor_inventions_list_scrollpaint(rct_window *w, rct_drawpixelinfo *dpi, int32_t scrollIndex);

static void window_editor_inventions_list_drag_cursor(rct_window *w, rct_widgetindex widgetIndex, const ScreenCoordsXY& screenCoords, int32_t *cursorId);
static void window_editor_inventions_list_drag_moved(rct_window* w, const ScreenCoordsXY& screenCoords);
static void window_editor_inventions_list_drag_paint(rct_window *w, rct_drawpixelinfo *dpi);

static rct_string_id window_editor_inventions_list_prepare_name(const ResearchItem * researchItem, bool withGap);

// 0x0098177C
static rct_window_event_list window_editor_inventions_list_events = {
    window_editor_inventions_list_close,
    window_editor_inventions_list_mouseup,
    window_editor_inventions_list_resize,
    nullptr,
    nullptr,
    nullptr,
    window_editor_inventions_list_update,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_editor_inventions_list_scrollgetheight,
    window_editor_inventions_list_scrollmousedown,
    nullptr,
    window_editor_inventions_list_scrollmouseover,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_editor_inventions_list_cursor,
    nullptr,
    window_editor_inventions_list_invalidate,
    window_editor_inventions_list_paint,
    window_editor_inventions_list_scrollpaint
};

// 0x009817EC
static rct_window_event_list window_editor_inventions_list_drag_events = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_editor_inventions_list_drag_cursor,
    window_editor_inventions_list_drag_moved,
    nullptr,
    window_editor_inventions_list_drag_paint,
    nullptr
};

#pragma endregion

static ResearchItem _editorInventionsListDraggedItem;

static constexpr const rct_string_id EditorInventionsResearchCategories[] = {
    STR_RESEARCH_NEW_TRANSPORT_RIDES,
    STR_RESEARCH_NEW_GENTLE_RIDES,
    STR_RESEARCH_NEW_ROLLER_COASTERS,
    STR_RESEARCH_NEW_THRILL_RIDES,
    STR_RESEARCH_NEW_WATER_RIDES,
    STR_RESEARCH_NEW_SHOPS_AND_STALLS,
    STR_RESEARCH_NEW_SCENERY_AND_THEMING,
};
// clang-format on

static void window_editor_inventions_list_drag_open(ResearchItem* researchItem);
static void move_research_item(ResearchItem* beforeItem, int32_t scrollIndex);

/**
 *
 *  rct2: 0x0068596F
 * Sets rides that are in use to be always researched
 */
static void research_rides_setup()
{
    // Reset all objects to not required
    for (uint8_t objectType = OBJECT_TYPE_RIDE; objectType < OBJECT_TYPE_COUNT; objectType++)
    {
        auto maxObjects = object_entry_group_counts[objectType];
        for (int32_t i = 0; i < maxObjects; i++)
        {
            Editor::ClearSelectedObject(objectType, i, OBJECT_SELECTION_FLAG_ALL);
        }
    }

    // Set research required for rides in use
    for (const auto& ride : GetRideManager())
    {
        Editor::SetSelectedObject(OBJECT_TYPE_RIDE, ride.subtype, OBJECT_SELECTION_FLAG_SELECTED);
    }
}

/**
 *
 *  rct2: 0x006855E7
 */
static void move_research_item(ResearchItem* beforeItem, int32_t scrollIndex)
{
    auto w = window_find_by_class(WC_EDITOR_INVENTION_LIST);
    if (w != nullptr)
    {
        w->research_item = nullptr;
        w->Invalidate();
    }

    research_remove(&_editorInventionsListDraggedItem);

    auto& researchList = scrollIndex == 0 ? gResearchItemsInvented : gResearchItemsUninvented;
    if (beforeItem != nullptr)
    {
        for (size_t i = 0; i < researchList.size(); i++)
        {
            if (researchList[i].Equals(beforeItem))
            {
                researchList.insert((researchList.begin() + i), _editorInventionsListDraggedItem);
                return;
            }
        }
    }

    // Still not found? Append to end of list.
    researchList.push_back(_editorInventionsListDraggedItem);
}

/**
 *
 *  rct2: 0x0068558E
 */
static ResearchItem* window_editor_inventions_list_get_item_from_scroll_y(int32_t scrollIndex, int32_t y)
{
    auto& researchList = scrollIndex == 0 ? gResearchItemsInvented : gResearchItemsUninvented;
    for (auto& researchItem : researchList)
    {
        y -= SCROLLABLE_ROW_HEIGHT;
        if (y < 0)
        {
            return &researchItem;
        }
    }

    return nullptr;
}

/**
 *
 *  rct2: 0x006855BB
 */
static ResearchItem* window_editor_inventions_list_get_item_from_scroll_y_include_seps(int32_t scrollIndex, int32_t y)
{
    auto& researchList = scrollIndex == 0 ? gResearchItemsInvented : gResearchItemsUninvented;
    for (auto& researchItem : researchList)
    {
        y -= SCROLLABLE_ROW_HEIGHT;
        if (y < 0)
        {
            return &researchItem;
        }
    }
    return nullptr;
}

static ResearchItem* get_research_item_at(const ScreenCoordsXY& screenCoords, int32_t* outScrollId)
{
    rct_window* w = window_find_by_class(WC_EDITOR_INVENTION_LIST);
    if (w != nullptr && w->windowPos.x <= screenCoords.x && w->windowPos.y < screenCoords.y
        && w->windowPos.x + w->width > screenCoords.x && w->windowPos.y + w->height > screenCoords.y)
    {
        rct_widgetindex widgetIndex = window_find_widget_from_point(w, screenCoords);
        rct_widget* widget = &w->widgets[widgetIndex];
        if (widgetIndex == WIDX_PRE_RESEARCHED_SCROLL || widgetIndex == WIDX_RESEARCH_ORDER_SCROLL)
        {
            gPressedWidget.widget_index = widgetIndex;
            int32_t outScrollArea;
            ScreenCoordsXY outScrollCoords;
            widget_scroll_get_part(w, widget, screenCoords, outScrollCoords, &outScrollArea, outScrollId);
            if (outScrollArea == SCROLL_PART_VIEW)
            {
                *outScrollId = *outScrollId == 0 ? 0 : 1;

                int32_t scrollY = outScrollCoords.y + 6;
                return window_editor_inventions_list_get_item_from_scroll_y_include_seps(*outScrollId, scrollY);
            }
        }
    }

    *outScrollId = -1;
    return nullptr;
}

/**
 *
 *  rct2: 0x00684E04
 */
rct_window* window_editor_inventions_list_open()
{
    rct_window* w;

    w = window_bring_to_front_by_class(WC_EDITOR_INVENTION_LIST);
    if (w != nullptr)
        return w;

    research_rides_setup();

    w = window_create_centred(
        WW, WH, &window_editor_inventions_list_events, WC_EDITOR_INVENTION_LIST, WF_NO_SCROLLING | WF_RESIZABLE);
    w->widgets = window_editor_inventions_list_widgets;
    w->enabled_widgets = (1 << WIDX_CLOSE) | (1 << WIDX_RESIZE) | (1 << WIDX_TAB_1) | (1 << WIDX_RANDOM_SHUFFLE)
        | (1 << WIDX_MOVE_ITEMS_TO_BOTTOM) | (1 << WIDX_MOVE_ITEMS_TO_TOP);
    window_init_scroll_widgets(w);
    w->selected_tab = 0;
    w->research_item = nullptr;
    _editorInventionsListDraggedItem.rawValue = RESEARCH_ITEM_NULL;

    w->min_width = WW;
    w->min_height = WH;
    w->max_width = WW * 2;
    w->max_height = WH * 2;

    return w;
}

/**
 *
 *  rct2: 0x006853D2
 */
static void window_editor_inventions_list_close(rct_window* w)
{
    research_remove_flags();

    // When used in-game (as a cheat)
    if (!(gScreenFlags & SCREEN_FLAGS_EDITOR))
    {
        gSilentResearch = true;
        research_reset_current_item();
        gSilentResearch = false;
    }
}

/**
 *
 *  rct2: 0x0068521B
 */
static void window_editor_inventions_list_mouseup(rct_window* w, rct_widgetindex widgetIndex)
{
    switch (widgetIndex)
    {
        case WIDX_CLOSE:
            window_close(w);
            break;
        case WIDX_RANDOM_SHUFFLE:
            research_items_shuffle();
            w->Invalidate();
            break;
        case WIDX_MOVE_ITEMS_TO_TOP:
            research_items_make_all_researched();
            window_init_scroll_widgets(w);
            w->Invalidate();
            break;
        case WIDX_MOVE_ITEMS_TO_BOTTOM:
            research_items_make_all_unresearched();
            window_init_scroll_widgets(w);
            w->Invalidate();
            break;
    }
}

static void window_editor_inventions_list_resize(rct_window* w)
{
    if (w->width < w->min_width)
    {
        w->Invalidate();
        w->width = w->min_width;
    }
    if (w->height < w->min_height)
    {
        w->Invalidate();
        w->height = w->min_height;
    }
}

/**
 *
 *  rct2: 0x00685392
 */
static void window_editor_inventions_list_update(rct_window* w)
{
    w->frame_no++;
    window_event_invalidate_call(w);
    widget_invalidate(w, WIDX_TAB_1);

    if (_editorInventionsListDraggedItem.IsNull())
        return;

    if (window_find_by_class(WC_EDITOR_INVENTION_LIST_DRAG) != nullptr)
        return;

    _editorInventionsListDraggedItem.rawValue = RESEARCH_ITEM_NULL;
    w->Invalidate();
}

/**
 *
 *  rct2: 0x00685239
 */
static void window_editor_inventions_list_scrollgetheight(rct_window* w, int32_t scrollIndex, int32_t* width, int32_t* height)
{
    *height = 0;
    if (scrollIndex == 0)
    {
        *height += (int32_t)gResearchItemsInvented.size() * SCROLLABLE_ROW_HEIGHT;
    }
    else
    {
        *height += (int32_t)gResearchItemsUninvented.size() * SCROLLABLE_ROW_HEIGHT;
    }
}

/**
 *
 *  rct2: 0x006852D4
 */
static void window_editor_inventions_list_scrollmousedown(
    rct_window* w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords)
{
    ResearchItem* researchItem;

    researchItem = window_editor_inventions_list_get_item_from_scroll_y(scrollIndex, screenCoords.y);
    if (researchItem == nullptr)
        return;

    // Disallow picking up always-researched items
    if (researchItem->IsAlwaysResearched())
        return;

    w->Invalidate();
    window_editor_inventions_list_drag_open(researchItem);
}

/**
 *
 *  rct2: 0x00685275
 */
static void window_editor_inventions_list_scrollmouseover(
    rct_window* w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords)
{
    ResearchItem* researchItem;

    researchItem = window_editor_inventions_list_get_item_from_scroll_y(scrollIndex, screenCoords.y);
    if (researchItem != w->research_item)
    {
        w->research_item = researchItem;
        w->Invalidate();

        // Prevent always-researched items from being highlighted when hovered over
        if (researchItem != nullptr && researchItem->IsAlwaysResearched())
        {
            w->research_item = nullptr;
        }
    }
}

/**
 *
 *  rct2: 0x00685291
 */
static void window_editor_inventions_list_cursor(
    rct_window* w, rct_widgetindex widgetIndex, const ScreenCoordsXY& screenCoords, int32_t* cursorId)
{
    ResearchItem* researchItem;
    int32_t scrollIndex;

    switch (widgetIndex)
    {
        case WIDX_PRE_RESEARCHED_SCROLL:
            scrollIndex = 0;
            break;
        case WIDX_RESEARCH_ORDER_SCROLL:
            scrollIndex = 1;
            break;
        default:
            return;
    }

    // Use the open hand as cursor for items that can be picked up
    researchItem = window_editor_inventions_list_get_item_from_scroll_y(scrollIndex, screenCoords.y);
    if (researchItem != nullptr && !researchItem->IsAlwaysResearched())
    {
        *cursorId = CURSOR_HAND_OPEN;
    }
}

/**
 *
 *  rct2: 0x00685392
 */
static void window_editor_inventions_list_invalidate(rct_window* w)
{
    w->pressed_widgets |= 1 << WIDX_PREVIEW;
    w->pressed_widgets |= 1 << WIDX_TAB_1;

    w->widgets[WIDX_CLOSE].type = gScreenFlags & SCREEN_FLAGS_SCENARIO_EDITOR ? WWT_EMPTY : WWT_CLOSEBOX;

    w->widgets[WIDX_BACKGROUND].right = w->width - 1;
    w->widgets[WIDX_BACKGROUND].bottom = w->height - 1;
    w->widgets[WIDX_TITLE].right = w->width - 2;
    w->widgets[WIDX_CLOSE].left = w->width - 13;
    w->widgets[WIDX_CLOSE].right = w->width - 3;
    w->widgets[WIDX_RESIZE].right = w->width - 1;
    w->widgets[WIDX_RESIZE].bottom = w->height - 1;

    int16_t scroll_list_height = (w->height - 88) / 2;

    w->widgets[WIDX_PRE_RESEARCHED_SCROLL].bottom = 60 + scroll_list_height;
    w->widgets[WIDX_PRE_RESEARCHED_SCROLL].right = w->width - 229;

    w->widgets[WIDX_RESEARCH_ORDER_SCROLL].top = w->widgets[WIDX_PRE_RESEARCHED_SCROLL].bottom + 15;
    w->widgets[WIDX_RESEARCH_ORDER_SCROLL].bottom = w->widgets[WIDX_RESEARCH_ORDER_SCROLL].top + scroll_list_height;
    w->widgets[WIDX_RESEARCH_ORDER_SCROLL].right = w->width - 229;

    w->widgets[WIDX_PREVIEW].left = w->width - 169;
    w->widgets[WIDX_PREVIEW].right = w->width - 56;

    w->widgets[WIDX_MOVE_ITEMS_TO_TOP].top = w->height - 57;
    w->widgets[WIDX_MOVE_ITEMS_TO_TOP].bottom = w->height - 44;
    w->widgets[WIDX_MOVE_ITEMS_TO_TOP].left = w->width - 225;
    w->widgets[WIDX_MOVE_ITEMS_TO_TOP].right = w->width - 6;

    w->widgets[WIDX_MOVE_ITEMS_TO_BOTTOM].top = w->height - 42;
    w->widgets[WIDX_MOVE_ITEMS_TO_BOTTOM].bottom = w->height - 29;
    w->widgets[WIDX_MOVE_ITEMS_TO_BOTTOM].left = w->width - 225;
    w->widgets[WIDX_MOVE_ITEMS_TO_BOTTOM].right = w->width - 6;

    w->widgets[WIDX_RANDOM_SHUFFLE].top = w->height - 27;
    w->widgets[WIDX_RANDOM_SHUFFLE].bottom = w->height - 14;
    w->widgets[WIDX_RANDOM_SHUFFLE].left = w->width - 225;
    w->widgets[WIDX_RANDOM_SHUFFLE].right = w->width - 6;
}

/**
 *
 *  rct2: 0x00684EE0
 */
static void window_editor_inventions_list_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    rct_widget* widget;
    ResearchItem* researchItem;
    rct_string_id stringId;
    int32_t x, y, width;

    window_draw_widgets(w, dpi);

    // Tab image
    x = w->windowPos.x + w->widgets[WIDX_TAB_1].left;
    y = w->windowPos.y + w->widgets[WIDX_TAB_1].top;
    gfx_draw_sprite(dpi, SPR_TAB_FINANCES_RESEARCH_0 + (w->frame_no / 2) % 8, x, y, 0);

    // Pre-researched items label
    x = w->windowPos.x + w->widgets[WIDX_PRE_RESEARCHED_SCROLL].left;
    y = w->windowPos.y + w->widgets[WIDX_PRE_RESEARCHED_SCROLL].top - 11;
    gfx_draw_string_left(dpi, STR_INVENTION_PREINVENTED_ITEMS, nullptr, COLOUR_BLACK, x, y - 1);

    // Research order label
    x = w->windowPos.x + w->widgets[WIDX_RESEARCH_ORDER_SCROLL].left;
    y = w->windowPos.y + w->widgets[WIDX_RESEARCH_ORDER_SCROLL].top - 11;
    gfx_draw_string_left(dpi, STR_INVENTION_TO_BE_INVENTED_ITEMS, nullptr, COLOUR_BLACK, x, y - 1);

    // Preview background
    widget = &w->widgets[WIDX_PREVIEW];
    gfx_fill_rect(
        dpi, w->windowPos.x + widget->left + 1, w->windowPos.y + widget->top + 1, w->windowPos.x + widget->right - 1,
        w->windowPos.y + widget->bottom - 1, ColourMapA[w->colours[1]].darkest);

    researchItem = &_editorInventionsListDraggedItem;
    if (researchItem->IsNull())
        researchItem = w->research_item;
    // If the research item is null or a list separator.
    if (researchItem == nullptr || researchItem->IsNull())
        return;

    // Preview image
    int32_t objectEntryType = OBJECT_TYPE_SCENERY_GROUP;
    if (researchItem->type == RESEARCH_ENTRY_TYPE_RIDE)
        objectEntryType = OBJECT_TYPE_RIDE;

    auto chunk = object_entry_get_chunk(objectEntryType, researchItem->entryIndex);
    if (chunk == nullptr)
        return;

    auto entry = object_entry_get_entry(objectEntryType, researchItem->entryIndex);

    // Draw preview
    widget = &w->widgets[WIDX_PREVIEW];

    void* object = object_manager_get_loaded_object(entry);
    if (object != nullptr)
    {
        rct_drawpixelinfo clipDPI;
        x = w->windowPos.x + widget->left + 1;
        y = w->windowPos.y + widget->top + 1;
        width = widget->right - widget->left - 1;
        int32_t height = widget->bottom - widget->top - 1;
        if (clip_drawpixelinfo(&clipDPI, dpi, x, y, width, height))
        {
            object_draw_preview(object, &clipDPI, width, height);
        }
    }

    // Item name
    x = w->windowPos.x + ((widget->left + widget->right) / 2) + 1;
    y = w->windowPos.y + widget->bottom + 3;
    width = w->width - w->widgets[WIDX_RESEARCH_ORDER_SCROLL].right - 6;

    rct_string_id drawString = window_editor_inventions_list_prepare_name(researchItem, false);
    gfx_draw_string_centred_clipped(dpi, drawString, gCommonFormatArgs, COLOUR_BLACK, x, y, width);
    y += 15;

    // Item category
    x = w->windowPos.x + w->widgets[WIDX_RESEARCH_ORDER_SCROLL].right + 4;
    stringId = EditorInventionsResearchCategories[researchItem->category];
    gfx_draw_string_left(dpi, STR_INVENTION_RESEARCH_GROUP, &stringId, COLOUR_BLACK, x, y);
}

/**
 *
 *  rct2: 0x006850BD
 */
static void window_editor_inventions_list_scrollpaint(rct_window* w, rct_drawpixelinfo* dpi, int32_t scrollIndex)
{
    // Draw background
    uint8_t paletteIndex = ColourMapA[w->colours[1]].mid_light;
    gfx_clear(dpi, paletteIndex);

    int16_t boxWidth = (w->widgets[WIDX_RESEARCH_ORDER_SCROLL].right - w->widgets[WIDX_RESEARCH_ORDER_SCROLL].left);
    int16_t columnSplitOffset = boxWidth / 2;
    int32_t itemY = -SCROLLABLE_ROW_HEIGHT;

    const auto& researchList = scrollIndex == 0 ? gResearchItemsInvented : gResearchItemsUninvented;
    for (const auto& researchItem : researchList)
    {
        itemY += SCROLLABLE_ROW_HEIGHT;
        if (itemY + SCROLLABLE_ROW_HEIGHT < dpi->y || itemY >= dpi->y + dpi->height)
            continue;

        if (w->research_item == &researchItem)
        {
            int32_t top, bottom;
            if (_editorInventionsListDraggedItem.IsNull())
            {
                // Highlight
                top = itemY;
                bottom = itemY + SCROLLABLE_ROW_HEIGHT - 1;
            }
            else
            {
                // Drop horizontal rule
                top = itemY - 1;
                bottom = itemY;
            }

            gfx_filter_rect(dpi, 0, top, boxWidth, bottom, PALETTE_DARKEN_1);
        }

        if (researchItem.Equals(&_editorInventionsListDraggedItem))
            continue;

        utf8 groupNameBuffer[256], vehicleNameBuffer[256];
        utf8* groupNamePtr = groupNameBuffer;
        utf8* vehicleNamePtr = vehicleNameBuffer;

        uint8_t colour;
        if (researchItem.IsAlwaysResearched())
        {
            if (w->research_item == &researchItem && _editorInventionsListDraggedItem.IsNull())
                gCurrentFontSpriteBase = FONT_SPRITE_BASE_MEDIUM_EXTRA_DARK;
            else
                gCurrentFontSpriteBase = FONT_SPRITE_BASE_MEDIUM_DARK;
            colour = w->colours[1] | COLOUR_FLAG_INSET;
        }
        else
        {
            // TODO: this is actually just a black colour.
            colour = COLOUR_BRIGHT_GREEN | COLOUR_FLAG_TRANSLUCENT;
            gCurrentFontSpriteBase = FONT_SPRITE_BASE_MEDIUM;

            groupNamePtr = utf8_write_codepoint(groupNamePtr, colour);
            vehicleNamePtr = utf8_write_codepoint(vehicleNamePtr, colour);
        }

        rct_string_id itemNameId = researchItem.GetName();

        if (researchItem.type == RESEARCH_ENTRY_TYPE_RIDE
            && !RideGroupManager::RideTypeIsIndependent(researchItem.baseRideType))
        {
            const auto rideEntry = get_ride_entry(researchItem.entryIndex);
            const rct_string_id rideGroupName = get_ride_naming(researchItem.baseRideType, rideEntry).name;
            format_string(
                groupNamePtr, std::size(groupNameBuffer), STR_INVENTIONS_LIST_RIDE_AND_VEHICLE_NAME, (void*)&rideGroupName);
            format_string(vehicleNamePtr, std::size(vehicleNameBuffer), itemNameId, nullptr);
        }
        else
        {
            format_string(groupNamePtr, std::size(groupNameBuffer), itemNameId, nullptr);
            vehicleNamePtr = nullptr;
        }

        // Draw group name
        gfx_clip_string(groupNameBuffer, columnSplitOffset);
        gfx_draw_string(dpi, groupNameBuffer, colour, 1, itemY);

        // Draw vehicle name
        if (vehicleNamePtr)
        {
            gfx_clip_string(vehicleNameBuffer, columnSplitOffset - 11);
            gfx_draw_string(dpi, vehicleNameBuffer, colour, columnSplitOffset + 1, itemY);
        }
    }
}

#pragma region Drag item

/**
 *
 *  rct2: 0x006852F4
 */
static void window_editor_inventions_list_drag_open(ResearchItem* researchItem)
{
    char buffer[256], *ptr;
    int32_t stringWidth;
    rct_window* w;

    window_close_by_class(WC_EDITOR_INVENTION_LIST_DRAG);
    _editorInventionsListDraggedItem = *researchItem;
    rct_string_id stringId = researchItem->GetName();

    ptr = buffer;
    if (researchItem->type == RESEARCH_ENTRY_TYPE_RIDE && !RideGroupManager::RideTypeIsIndependent(researchItem->baseRideType))
    {
        const auto rideEntry = get_ride_entry(researchItem->entryIndex);
        const rct_string_id rideGroupName = get_ride_naming(researchItem->baseRideType, rideEntry).name;
        rct_string_id args[] = {
            rideGroupName,
            stringId,
        };
        format_string(ptr, 256, STR_INVENTIONS_LIST_RIDE_AND_VEHICLE_NAME, &args);
    }
    else
    {
        format_string(ptr, 256, stringId, nullptr);
    }

    stringWidth = gfx_get_string_width(buffer);
    window_editor_inventions_list_drag_widgets[0].right = stringWidth;

    w = window_create(
        ScreenCoordsXY(gTooltipCursorX - (stringWidth / 2), gTooltipCursorY - 7), stringWidth, 14,
        &window_editor_inventions_list_drag_events, WC_EDITOR_INVENTION_LIST_DRAG,
        WF_STICK_TO_FRONT | WF_TRANSPARENT | WF_NO_SNAPPING);
    w->widgets = window_editor_inventions_list_drag_widgets;
    w->colours[1] = COLOUR_WHITE;
    input_window_position_begin(w, 0, ScreenCoordsXY(gTooltipCursorX, gTooltipCursorY));
}

/**
 *
 *  rct2: 0x0068549C
 */
static void window_editor_inventions_list_drag_cursor(
    rct_window* w, rct_widgetindex widgetIndex, const ScreenCoordsXY& screenCoords, int32_t* cursorId)
{
    rct_window* inventionListWindow = window_find_by_class(WC_EDITOR_INVENTION_LIST);
    if (inventionListWindow != nullptr)
    {
        int32_t scrollId;
        ResearchItem* researchItem = get_research_item_at(screenCoords, &scrollId);
        if (researchItem != inventionListWindow->research_item)
        {
            inventionListWindow->Invalidate();
        }
    }

    *cursorId = CURSOR_HAND_CLOSED;
}

/**
 *
 *  rct2: 0x00685412
 */
static void window_editor_inventions_list_drag_moved(rct_window* w, const ScreenCoordsXY& screenCoords)
{
    ResearchItem* researchItem;

    int32_t scrollId;
    // Skip always researched items, so that the dragged item gets placed underneath them
    auto newScreenCoords = screenCoords;
    do
    {
        researchItem = get_research_item_at(newScreenCoords, &scrollId);
        newScreenCoords.y += LIST_ROW_HEIGHT;
    } while (researchItem != nullptr && researchItem->IsAlwaysResearched());

    if (scrollId != -1)
    {
        move_research_item(researchItem, scrollId);
    }

    window_close(w);
    _editorInventionsListDraggedItem.rawValue = RESEARCH_ITEM_NULL;
    window_invalidate_by_class(WC_EDITOR_INVENTION_LIST);
}

/**
 *
 *  rct2: 0x006853D9
 */
static void window_editor_inventions_list_drag_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    rct_string_id drawString;
    int32_t x, y;

    x = w->windowPos.x;
    y = w->windowPos.y + 2;
    drawString = window_editor_inventions_list_prepare_name(&_editorInventionsListDraggedItem, true);
    gfx_draw_string_left(dpi, drawString, gCommonFormatArgs, COLOUR_BLACK | COLOUR_FLAG_OUTLINE, x, y);
}

static rct_string_id window_editor_inventions_list_prepare_name(const ResearchItem* researchItem, bool withGap)
{
    rct_string_id drawString;
    rct_string_id stringId = researchItem->GetName();

    if (researchItem->type == RESEARCH_ENTRY_TYPE_RIDE && !RideGroupManager::RideTypeIsIndependent(researchItem->baseRideType))
    {
        drawString = withGap ? STR_INVENTIONS_LIST_RIDE_AND_VEHICLE_NAME_DRAG : STR_WINDOW_COLOUR_2_STRINGID_STRINGID;
        rct_string_id rideGroupName = get_ride_naming(researchItem->baseRideType, get_ride_entry(researchItem->entryIndex))
                                          .name;
        set_format_arg(0, rct_string_id, rideGroupName);
        set_format_arg(2, rct_string_id, stringId);
    }
    else
    {
        drawString = STR_WINDOW_COLOUR_2_STRINGID;
        set_format_arg(0, rct_string_id, stringId);
    }

    return drawString;
}

#pragma endregion
