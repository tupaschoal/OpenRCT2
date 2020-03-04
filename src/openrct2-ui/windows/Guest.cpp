/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <openrct2-ui/interface/Viewport.h>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/windows/Window.h>
#include <openrct2/Context.h>
#include <openrct2/Game.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/actions/GuestSetFlagsAction.hpp>
#include <openrct2/actions/PeepPickupAction.hpp>
#include <openrct2/config/Config.h>
#include <openrct2/localisation/Localisation.h>
#include <openrct2/management/Marketing.h>
#include <openrct2/network/network.h>
#include <openrct2/peep/Peep.h>
#include <openrct2/peep/Staff.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/ride/ShopItem.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/sprites.h>
#include <openrct2/util/Util.h>
#include <openrct2/windows/Intent.h>
#include <openrct2/world/Footpath.h>
#include <openrct2/world/Park.h>

// clang-format off
enum WINDOW_GUEST_PAGE {
    WINDOW_GUEST_OVERVIEW,
    WINDOW_GUEST_STATS,
    WINDOW_GUEST_RIDES,
    WINDOW_GUEST_FINANCE,
    WINDOW_GUEST_THOUGHTS,
    WINDOW_GUEST_INVENTORY,
    WINDOW_GUEST_DEBUG
};

enum WINDOW_GUEST_WIDGET_IDX {
    WIDX_BACKGROUND,
    WIDX_TITLE,
    WIDX_CLOSE,
    WIDX_PAGE_BACKGROUND,
    WIDX_TAB_1,
    WIDX_TAB_2,
    WIDX_TAB_3,
    WIDX_TAB_4,
    WIDX_TAB_5,
    WIDX_TAB_6,
    WIDX_TAB_7,

    WIDX_MARQUEE = 11,
    WIDX_VIEWPORT,
    WIDX_ACTION_LBL,
    WIDX_PICKUP,
    WIDX_RENAME,
    WIDX_LOCATE,
    WIDX_TRACK,

    WIDX_RIDE_SCROLL = 11
};

validate_global_widx(WC_PEEP, WIDX_PICKUP);

static constexpr int32_t TabWidth = 30;

#define MAIN_GUEST_WIDGETS \
    { WWT_FRAME,    0, 0,   191,            0,   156, 0xFFFFFFFF,                   STR_NONE },                         /* Panel / Background */    \
    { WWT_CAPTION,  0, 1,   190,            1,   14,  STR_STRINGID,                 STR_WINDOW_TITLE_TIP },             /* Title */                 \
    { WWT_CLOSEBOX, 0, 179, 189,            2,   13,  STR_CLOSE_X,                  STR_CLOSE_WINDOW_TIP },             /* Close x button */        \
    { WWT_RESIZE,   1, 0,   191,            43,  156, 0xFFFFFFFF,                   STR_NONE },                         /* Resize */                \
    { WWT_TAB,      1, 3,   TabWidth + 3,   17,  43,  IMAGE_TYPE_REMAP | SPR_TAB,   STR_SHOW_GUEST_VIEW_TIP },          /* Tab 1 */                 \
    { WWT_TAB,      1, 34,  TabWidth + 34,  17,  43,  IMAGE_TYPE_REMAP | SPR_TAB,   STR_SHOW_GUEST_NEEDS_TIP },         /* Tab 2 */                 \
    { WWT_TAB,      1, 65,  TabWidth + 65,  17,  43,  IMAGE_TYPE_REMAP | SPR_TAB,   STR_SHOW_GUEST_VISITED_RIDES_TIP }, /* Tab 3 */                 \
    { WWT_TAB,      1, 96,  TabWidth + 96,  17,  43,  IMAGE_TYPE_REMAP | SPR_TAB,   STR_SHOW_GUEST_FINANCE_TIP },       /* Tab 4 */                 \
    { WWT_TAB,      1, 127, TabWidth + 127, 17,  43,  IMAGE_TYPE_REMAP | SPR_TAB,   STR_SHOW_GUEST_THOUGHTS_TIP },      /* Tab 5 */                 \
    { WWT_TAB,      1, 158, TabWidth + 158, 17,  43,  IMAGE_TYPE_REMAP | SPR_TAB,   STR_SHOW_GUEST_ITEMS_TIP },         /* Tab 6 */                 \
    { WWT_TAB,      1, 189, TabWidth + 189, 17,  43,  IMAGE_TYPE_REMAP | SPR_TAB,   STR_DEBUG_TIP }                     /* Tab 7 */

static rct_widget window_guest_overview_widgets[] = {
    MAIN_GUEST_WIDGETS,
    { WWT_LABEL_CENTRED,    1, 3,   166, 45,  56,  0xFFFFFFFF,      STR_NONE                        },  // Label Thought marquee
    { WWT_VIEWPORT,         1, 3,   166, 57,  143, 0xFFFFFFFF,      STR_NONE                        },  // Viewport
    { WWT_LABEL_CENTRED,    1, 3,   166, 144, 154, 0xFFFFFFFF,      STR_NONE                        },  // Label Action
    { WWT_FLATBTN,          1, 167, 190, 45,  68,  SPR_PICKUP_BTN,  STR_PICKUP_TIP                  },  // Pickup Button
    { WWT_FLATBTN,          1, 167, 190, 69,  92,  SPR_RENAME,      STR_NAME_GUEST_TIP              },  // Rename Button
    { WWT_FLATBTN,          1, 167, 190, 93,  116, SPR_LOCATE,      STR_LOCATE_SUBJECT_TIP          },  // Locate Button
    { WWT_FLATBTN,          1, 167, 190, 117, 140, SPR_TRACK_PEEP,  STR_TOGGLE_GUEST_TRACKING_TIP   },  // Track Button
    { WIDGETS_END },
};

static rct_widget window_guest_stats_widgets[] = {
    MAIN_GUEST_WIDGETS,
    { WIDGETS_END },
};

static rct_widget window_guest_rides_widgets[] = {
    MAIN_GUEST_WIDGETS,
    { WWT_SCROLL,           1, 3,   188, 57, 143, SCROLL_VERTICAL,  STR_NONE },
    { WIDGETS_END },
};

static rct_widget window_guest_finance_widgets[] = {
    MAIN_GUEST_WIDGETS,
    { WIDGETS_END },
};

static rct_widget window_guest_thoughts_widgets[] = {
    MAIN_GUEST_WIDGETS,
    { WIDGETS_END },
};

static rct_widget window_guest_inventory_widgets[] = {
    MAIN_GUEST_WIDGETS,
    { WIDGETS_END },
};

static rct_widget window_guest_debug_widgets[] = {
    MAIN_GUEST_WIDGETS,
    { WIDGETS_END },
};

// 0x981D0C
static rct_widget *window_guest_page_widgets[] = {
    window_guest_overview_widgets,
    window_guest_stats_widgets,
    window_guest_rides_widgets,
    window_guest_finance_widgets,
    window_guest_thoughts_widgets,
    window_guest_inventory_widgets,
    window_guest_debug_widgets
};

static void window_guest_set_page(rct_window* w, int32_t page);
static void window_guest_disable_widgets(rct_window* w);
static void window_guest_viewport_init(rct_window* w);
static void window_guest_common_resize(rct_window* w);
static void window_guest_common_invalidate(rct_window* w);

static void window_guest_overview_close(rct_window *w);
static void window_guest_overview_resize(rct_window *w);
static void window_guest_overview_mouse_up(rct_window *w, rct_widgetindex widgetIndex);
static void window_guest_overview_paint(rct_window *w, rct_drawpixelinfo *dpi);
static void window_guest_overview_invalidate(rct_window *w);
static void window_guest_overview_viewport_rotate(rct_window *w);
static void window_guest_overview_update(rct_window* w);
static void window_guest_overview_text_input(rct_window *w, rct_widgetindex widgetIndex, char *text);
static void window_guest_overview_tool_update(rct_window* w, rct_widgetindex widgetIndex, const ScreenCoordsXY& screenCoords);
static void window_guest_overview_tool_down(rct_window* w, rct_widgetindex widgetIndex, const ScreenCoordsXY& screenCoords);
static void window_guest_overview_tool_abort(rct_window *w, rct_widgetindex widgetIndex);

static void window_guest_mouse_up(rct_window *w, rct_widgetindex widgetIndex);

static void window_guest_stats_update(rct_window *w);
static void window_guest_stats_paint(rct_window *w, rct_drawpixelinfo *dpi);

static void window_guest_rides_update(rct_window *w);
static void window_guest_rides_scroll_get_size(rct_window *w, int32_t scrollIndex, int32_t *width, int32_t *height);
static void window_guest_rides_scroll_mouse_down(rct_window *w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords);
static void window_guest_rides_scroll_mouse_over(rct_window *w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords);
static void window_guest_rides_invalidate(rct_window *w);
static void window_guest_rides_paint(rct_window *w, rct_drawpixelinfo *dpi);
static void window_guest_rides_scroll_paint(rct_window *w, rct_drawpixelinfo *dpi, int32_t scrollIndex);

static void window_guest_finance_update(rct_window *w);
static void window_guest_finance_paint(rct_window *w, rct_drawpixelinfo *dpi);

static void window_guest_thoughts_update(rct_window *w);
static void window_guest_thoughts_paint(rct_window *w, rct_drawpixelinfo *dpi);

static void window_guest_inventory_update(rct_window *w);
static void window_guest_inventory_paint(rct_window *w, rct_drawpixelinfo *dpi);

static void window_guest_debug_update(rct_window *w);
static void window_guest_debug_paint(rct_window *w, rct_drawpixelinfo* dpi);

static rct_window_event_list window_guest_overview_events = {
    window_guest_overview_close,
    window_guest_overview_mouse_up,
    window_guest_overview_resize,
    nullptr,
    nullptr,
    nullptr,
    window_guest_overview_update,
    nullptr,
    nullptr,
    window_guest_overview_tool_update,
    window_guest_overview_tool_down,
    nullptr,
    nullptr,
    window_guest_overview_tool_abort,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_guest_overview_text_input,
    window_guest_overview_viewport_rotate,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_guest_overview_invalidate,
    window_guest_overview_paint,
    nullptr
};

static rct_window_event_list window_guest_stats_events = {
    nullptr,
    window_guest_mouse_up,
    window_guest_common_resize,
    nullptr,
    nullptr,
    nullptr,
    window_guest_stats_update,
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
    window_guest_common_invalidate,
    window_guest_stats_paint,
    nullptr
};

static rct_window_event_list window_guest_rides_events = {
    nullptr,
    window_guest_mouse_up,
    window_guest_common_resize,
    nullptr,
    nullptr,
    nullptr,
    window_guest_rides_update,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_guest_rides_scroll_get_size,
    window_guest_rides_scroll_mouse_down,
    nullptr,
    window_guest_rides_scroll_mouse_over,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_guest_rides_invalidate,
    window_guest_rides_paint,
    window_guest_rides_scroll_paint
};

static rct_window_event_list window_guest_finance_events = {
    nullptr,
    window_guest_mouse_up,
    window_guest_common_resize,
    nullptr,
    nullptr,
    nullptr,
    window_guest_finance_update,
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
    window_guest_common_invalidate,
    window_guest_finance_paint,
    nullptr
};

static rct_window_event_list window_guest_thoughts_events = {
    nullptr,
    window_guest_mouse_up,
    window_guest_common_resize,
    nullptr,
    nullptr,
    nullptr,
    window_guest_thoughts_update,
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
    window_guest_common_invalidate,
    window_guest_thoughts_paint,
    nullptr
};

static rct_window_event_list window_guest_inventory_events = {
    nullptr,
    window_guest_mouse_up,
    window_guest_common_resize,
    nullptr,
    nullptr,
    nullptr,
    window_guest_inventory_update,
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
    window_guest_common_invalidate,
    window_guest_inventory_paint,
    nullptr
};

static rct_window_event_list window_guest_debug_events = {
    nullptr,
    window_guest_mouse_up,
    window_guest_common_resize,
    nullptr,
    nullptr,
    nullptr,
    window_guest_debug_update,
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
    window_guest_common_invalidate,
    window_guest_debug_paint,
    nullptr
};

// 0x981D24
static rct_window_event_list *window_guest_page_events[] = {
    &window_guest_overview_events,
    &window_guest_stats_events,
    &window_guest_rides_events,
    &window_guest_finance_events,
    &window_guest_thoughts_events,
    &window_guest_inventory_events,
    &window_guest_debug_events
};

void window_guest_set_colours();

// 0x981D3C
static constexpr const uint32_t window_guest_page_enabled_widgets[] = {
    (1 << WIDX_CLOSE) |
    (1 << WIDX_TAB_1) |
    (1 << WIDX_TAB_2) |
    (1 << WIDX_TAB_3) |
    (1 << WIDX_TAB_4) |
    (1 << WIDX_TAB_5) |
    (1 << WIDX_TAB_6) |
    (1 << WIDX_TAB_7) |
    (1 << WIDX_RENAME)|
    (1 << WIDX_PICKUP)|
    (1 << WIDX_LOCATE)|
    (1 << WIDX_TRACK),

    (1 << WIDX_CLOSE) |
    (1 << WIDX_TAB_1) |
    (1 << WIDX_TAB_2) |
    (1 << WIDX_TAB_3) |
    (1 << WIDX_TAB_4) |
    (1 << WIDX_TAB_5) |
    (1 << WIDX_TAB_6) |
    (1 << WIDX_TAB_7),

    (1 << WIDX_CLOSE) |
    (1 << WIDX_TAB_1) |
    (1 << WIDX_TAB_2) |
    (1 << WIDX_TAB_3) |
    (1 << WIDX_TAB_4) |
    (1 << WIDX_TAB_5) |
    (1 << WIDX_TAB_6) |
    (1 << WIDX_TAB_7) |
    (1 << WIDX_RIDE_SCROLL),

    (1 << WIDX_CLOSE) |
    (1 << WIDX_TAB_1) |
    (1 << WIDX_TAB_2) |
    (1 << WIDX_TAB_3) |
    (1 << WIDX_TAB_4) |
    (1 << WIDX_TAB_5) |
    (1 << WIDX_TAB_6) |
    (1 << WIDX_TAB_7),

    (1 << WIDX_CLOSE) |
    (1 << WIDX_TAB_1) |
    (1 << WIDX_TAB_2) |
    (1 << WIDX_TAB_3) |
    (1 << WIDX_TAB_4) |
    (1 << WIDX_TAB_5) |
    (1 << WIDX_TAB_6) |
    (1 << WIDX_TAB_7),

    (1 << WIDX_CLOSE) |
    (1 << WIDX_TAB_1) |
    (1 << WIDX_TAB_2) |
    (1 << WIDX_TAB_3) |
    (1 << WIDX_TAB_4) |
    (1 << WIDX_TAB_5) |
    (1 << WIDX_TAB_6) |
    (1 << WIDX_TAB_7),

    (1 << WIDX_CLOSE) |
    (1 << WIDX_TAB_1) |
    (1 << WIDX_TAB_2) |
    (1 << WIDX_TAB_3) |
    (1 << WIDX_TAB_4) |
    (1 << WIDX_TAB_5) |
    (1 << WIDX_TAB_6) |
    (1 << WIDX_TAB_7)
};

static constexpr const rct_size16 window_guest_page_sizes[][2] = {
    { 192, 159, 500, 450 },     // WINDOW_GUEST_OVERVIEW
    { 192, 180, 192, 180 },     // WINDOW_GUEST_STATS
    { 192, 180, 500, 400 },     // WINDOW_GUEST_RIDES
    { 210, 148, 210, 148 },     // WINDOW_GUEST_FINANCE
    { 192, 159, 500, 450 },     // WINDOW_GUEST_THOUGHTS
    { 192, 159, 500, 450 },     // WINDOW_GUEST_INVENTORY
    { 192, 159, 192, 171 }      // WINDOW_GUEST_DEBUG
};
// clang-format on

/**
 *
 *  rct2: 0x006989E9
 *
 */
rct_window* window_guest_open(Peep* peep)
{
    if (peep->type == PEEP_TYPE_STAFF)
    {
        return window_staff_open(peep);
    }

    rct_window* window;

    window = window_bring_to_front_by_number(WC_PEEP, peep->sprite_index);
    if (window == nullptr)
    {
        int32_t windowWidth = 192;
        if (gConfigGeneral.debugging_tools)
            windowWidth += TabWidth;

        window = window_create_auto_pos(windowWidth, 157, &window_guest_overview_events, WC_PEEP, WF_RESIZABLE);
        window->widgets = window_guest_overview_widgets;
        window->enabled_widgets = window_guest_page_enabled_widgets[0];
        window->number = peep->sprite_index;
        window->page = 0;
        window->viewport_focus_coordinates.y = 0;
        window->frame_no = 0;
        window->list_information_type = 0;
        window->picked_peep_frame = 0;
        window->highlighted_item = 0;
        window_guest_disable_widgets(window);
        window->min_width = windowWidth;
        window->min_height = 157;
        window->max_width = 500;
        window->max_height = 450;
        window->no_list_items = 0;
        window->selected_list_item = -1;

        window->viewport_focus_coordinates.y = -1;
    }

    window->page = 0;
    window->Invalidate();

    window->widgets = window_guest_page_widgets[WINDOW_GUEST_OVERVIEW];
    window->enabled_widgets = window_guest_page_enabled_widgets[WINDOW_GUEST_OVERVIEW];
    window->hold_down_widgets = 0;
    window->event_handlers = window_guest_page_events[WINDOW_GUEST_OVERVIEW];
    window->pressed_widgets = 0;

    window_guest_disable_widgets(window);
    window_init_scroll_widgets(window);
    window_guest_viewport_init(window);

    return window;
}

static void window_guest_common_resize(rct_window* w)
{
    // Get page specific min and max size
    int32_t minWidth = window_guest_page_sizes[w->page][0].width;
    int32_t minHeight = window_guest_page_sizes[w->page][0].height;
    int32_t maxWidth = window_guest_page_sizes[w->page][1].width;
    int32_t maxHeight = window_guest_page_sizes[w->page][1].height;

    // Ensure min size is large enough for all tabs to fit
    for (int32_t i = WIDX_TAB_1; i <= WIDX_TAB_7; i++)
    {
        if (!(w->disabled_widgets & (1ULL << i)))
        {
            minWidth = std::max(minWidth, w->widgets[i].right + 3);
        }
    }
    maxWidth = std::max(minWidth, maxWidth);

    window_set_resize(w, minWidth, minHeight, maxWidth, maxHeight);
}

static void window_guest_common_invalidate(rct_window* w)
{
    if (window_guest_page_widgets[w->page] != w->widgets)
    {
        w->widgets = window_guest_page_widgets[w->page];
        window_init_scroll_widgets(w);
    }

    w->pressed_widgets |= 1ULL << (w->page + WIDX_TAB_1);

    auto peep = GET_PEEP(w->number);
    peep->FormatNameTo(gCommonFormatArgs);

    w->widgets[WIDX_BACKGROUND].right = w->width - 1;
    w->widgets[WIDX_BACKGROUND].bottom = w->height - 1;
    w->widgets[WIDX_PAGE_BACKGROUND].right = w->width - 1;
    w->widgets[WIDX_PAGE_BACKGROUND].bottom = w->height - 1;
    w->widgets[WIDX_TITLE].right = w->width - 2;
    w->widgets[WIDX_CLOSE].left = w->width - 13;
    w->widgets[WIDX_CLOSE].right = w->width - 3;

    window_align_tabs(w, WIDX_TAB_1, WIDX_TAB_7);
}

/**
 * Disables the finance tab when no money.
 * Disables peep pickup when in certain no pickup states.
 *  rct2: 0x006987A6
 */
void window_guest_disable_widgets(rct_window* w)
{
    Peep* peep = &get_sprite(w->number)->peep;
    uint64_t disabled_widgets = 0;

    if (peep_can_be_picked_up(peep))
    {
        if (w->disabled_widgets & (1 << WIDX_PICKUP))
            w->Invalidate();
    }
    else
    {
        disabled_widgets = (1 << WIDX_PICKUP);
        if (!(w->disabled_widgets & (1 << WIDX_PICKUP)))
            w->Invalidate();
    }
    if (gParkFlags & PARK_FLAGS_NO_MONEY)
    {
        disabled_widgets |= (1 << WIDX_TAB_4); // Disable finance tab if no money
    }
    if (!gConfigGeneral.debugging_tools)
    {
        disabled_widgets |= (1 << WIDX_TAB_7); // Disable debug tab when debug tools not turned on
    }
    w->disabled_widgets = disabled_widgets;
}

/**
 *
 *  rct2: 0x00696A75
 */
void window_guest_overview_close(rct_window* w)
{
    if (input_test_flag(INPUT_FLAG_TOOL_ACTIVE))
    {
        if (w->classification == gCurrentToolWidget.window_classification && w->number == gCurrentToolWidget.window_number)
            tool_cancel();
    }
}

/**
 *
 *  rct2: 0x00696FBE
 */
void window_guest_overview_resize(rct_window* w)
{
    window_guest_disable_widgets(w);
    window_event_invalidate_call(w);

    widget_invalidate(w, WIDX_MARQUEE);

    window_guest_common_resize(w);

    auto viewport = w->viewport;
    if (viewport != nullptr)
    {
        auto reqViewportWidth = w->width - 30;
        auto reqViewportHeight = w->height - 72;
        if (viewport->width != reqViewportWidth || viewport->height != reqViewportHeight)
        {
            uint8_t zoom_amount = 1 << viewport->zoom;
            viewport->width = reqViewportWidth;
            viewport->height = reqViewportHeight;
            viewport->view_width = viewport->width / zoom_amount;
            viewport->view_height = viewport->height / zoom_amount;
        }
    }
    window_guest_viewport_init(w);
}

/**
 *
 *  rct2: 0x00696A06
 */
void window_guest_overview_mouse_up(rct_window* w, rct_widgetindex widgetIndex)
{
    Peep* peep = GET_PEEP(w->number);

    switch (widgetIndex)
    {
        case WIDX_CLOSE:
            window_close(w);
            break;
        case WIDX_TAB_1:
        case WIDX_TAB_2:
        case WIDX_TAB_3:
        case WIDX_TAB_4:
        case WIDX_TAB_5:
        case WIDX_TAB_6:
        case WIDX_TAB_7:
            window_guest_set_page(w, widgetIndex - WIDX_TAB_1);
            break;
        case WIDX_PICKUP:
        {
            if (!peep_can_be_picked_up(peep))
            {
                return;
            }
            w->picked_peep_old_x = peep->x;
            PeepPickupAction pickupAction{ PeepPickupType::Pickup, w->number, {}, network_get_current_player_id() };
            pickupAction.SetCallback([peepnum = w->number](const GameAction* ga, const GameActionResult* result) {
                if (result->Error != GA_ERROR::OK)
                    return;
                rct_window* wind = window_find_by_number(WC_PEEP, peepnum);
                if (wind)
                {
                    tool_set(wind, WC_PEEP__WIDX_PICKUP, TOOL_PICKER);
                }
            });
            GameActions::Execute(&pickupAction);
        }
        break;
        case WIDX_RENAME:
        {
            auto peepName = peep->GetName();
            window_text_input_raw_open(w, widgetIndex, STR_GUEST_RENAME_TITLE, STR_GUEST_RENAME_PROMPT, peepName.c_str(), 32);
            break;
        }
        case WIDX_LOCATE:
            w->ScrollToViewport();
            break;
        case WIDX_TRACK:
        {
            uint32_t flags = peep->peep_flags ^ PEEP_FLAGS_TRACKING;

            auto guestSetFlagsAction = GuestSetFlagsAction(w->number, flags);
            GameActions::Execute(&guestSetFlagsAction);
        }
        break;
    }
}

/**
 *
 *  rct2: 0x696AA0
 */
void window_guest_set_page(rct_window* w, int32_t page)
{
    if (input_test_flag(INPUT_FLAG_TOOL_ACTIVE))
    {
        if (w->number == gCurrentToolWidget.window_number && w->classification == gCurrentToolWidget.window_classification)
            tool_cancel();
    }
    int32_t listen = 0;
    if (page == WINDOW_GUEST_OVERVIEW && w->page == WINDOW_GUEST_OVERVIEW && w->viewport)
    {
        if (!(w->viewport->flags & VIEWPORT_FLAG_SOUND_ON))
            listen = 1;
    }

    w->page = page;
    w->frame_no = 0;
    w->no_list_items = 0;
    w->selected_list_item = -1;

    rct_viewport* viewport = w->viewport;
    w->viewport = nullptr;
    if (viewport)
    {
        viewport->width = 0;
    }

    w->enabled_widgets = window_guest_page_enabled_widgets[page];
    w->hold_down_widgets = 0;
    w->event_handlers = window_guest_page_events[page];
    w->pressed_widgets = 0;
    w->widgets = window_guest_page_widgets[page];
    window_guest_disable_widgets(w);
    w->Invalidate();
    window_event_resize_call(w);
    window_event_invalidate_call(w);
    window_init_scroll_widgets(w);
    w->Invalidate();

    if (listen && w->viewport)
        w->viewport->flags |= VIEWPORT_FLAG_SOUND_ON;
}

void window_guest_overview_viewport_rotate(rct_window* w)
{
    window_guest_viewport_init(w);
}

/**
 *
 *  rct2: 0x0069883C
 */
void window_guest_viewport_init(rct_window* w)
{
    if (w->page != WINDOW_GUEST_OVERVIEW)
        return;

    auto peep = GET_PEEP(w->number);
    if (peep != nullptr)
    {
        auto focus = viewport_update_smart_guest_follow(w, peep);
        bool reCreateViewport = false;
        uint16_t origViewportFlags{};
        if (w->viewport != nullptr)
        {
            // Check all combos, for now skipping y and rot
            if (focus.coordinate.x == w->viewport_focus_coordinates.x
                && (focus.coordinate.y & VIEWPORT_FOCUS_Y_MASK) == w->viewport_focus_coordinates.y
                && focus.coordinate.z == w->viewport_focus_coordinates.z
                && focus.coordinate.rotation == w->viewport_focus_coordinates.rotation)
                return;

            origViewportFlags = w->viewport->flags;

            reCreateViewport = true;
            w->viewport->width = 0;
            w->viewport = nullptr;
        }

        window_event_invalidate_call(w);

        w->viewport_focus_coordinates.x = focus.coordinate.x;
        w->viewport_focus_coordinates.y = focus.coordinate.y;
        w->viewport_focus_coordinates.z = focus.coordinate.z;
        w->viewport_focus_coordinates.rotation = focus.coordinate.rotation;

        if (peep->state != PEEP_STATE_PICKED && w->viewport == nullptr)
        {
            auto view_widget = &w->widgets[WIDX_VIEWPORT];
            auto screenPos = ScreenCoordsXY{ view_widget->left + 1 + w->windowPos.x, view_widget->top + 1 + w->windowPos.y };
            int32_t width = view_widget->right - view_widget->left - 1;
            int32_t height = view_widget->bottom - view_widget->top - 1;

            viewport_create(
                w, screenPos, width, height, 0,
                { focus.coordinate.x, focus.coordinate.y & VIEWPORT_FOCUS_Y_MASK, focus.coordinate.z },
                focus.sprite.type & VIEWPORT_FOCUS_TYPE_MASK, focus.sprite.sprite_id);
            if (w->viewport != nullptr && reCreateViewport)
            {
                w->viewport->flags = origViewportFlags;
            }
            w->flags |= WF_NO_SCROLLING;
            w->Invalidate();
        }
        w->Invalidate();
    }
}

/**
 *
 *  rct2: 0x6983dd
 * used by window_staff as well
 */
static void window_guest_overview_tab_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    if (w->disabled_widgets & (1 << WIDX_TAB_1))
        return;

    rct_widget* widget = &w->widgets[WIDX_TAB_1];
    int32_t width = widget->right - widget->left - 1;
    int32_t height = widget->bottom - widget->top - 1;
    int32_t x = widget->left + 1 + w->windowPos.x;
    int32_t y = widget->top + 1 + w->windowPos.y;
    if (w->page == WINDOW_GUEST_OVERVIEW)
        height++;

    rct_drawpixelinfo clip_dpi;
    if (!clip_drawpixelinfo(&clip_dpi, dpi, x, y, width, height))
    {
        return;
    }

    x = 14;
    y = 20;

    Peep* peep = GET_PEEP(w->number);

    if (peep->type == PEEP_TYPE_STAFF && peep->staff_type == STAFF_TYPE_ENTERTAINER)
        y++;

    int32_t animationFrame = g_peep_animation_entries[peep->sprite_type].sprite_animation->base_image + 1;

    int32_t animationFrameOffset = 0;

    if (w->page == WINDOW_GUEST_OVERVIEW)
    {
        animationFrameOffset = w->var_496;
        animationFrameOffset &= 0xFFFC;
    }
    animationFrame += animationFrameOffset;

    int32_t sprite_id = animationFrame | SPRITE_ID_PALETTE_COLOUR_2(peep->tshirt_colour, peep->trousers_colour);
    gfx_draw_sprite(&clip_dpi, sprite_id, x, y, 0);

    // If holding a balloon
    if (animationFrame >= 0x2A1D && animationFrame < 0x2A3D)
    {
        animationFrame += 32;
        animationFrame |= SPRITE_ID_PALETTE_COLOUR_1(peep->balloon_colour);
        gfx_draw_sprite(&clip_dpi, animationFrame, x, y, 0);
    }

    // If holding umbrella
    if (animationFrame >= 0x2BBD && animationFrame < 0x2BDD)
    {
        animationFrame += 32;
        animationFrame |= SPRITE_ID_PALETTE_COLOUR_1(peep->umbrella_colour);
        gfx_draw_sprite(&clip_dpi, animationFrame, x, y, 0);
    }

    // If wearing hat
    if (animationFrame >= 0x29DD && animationFrame < 0x29FD)
    {
        animationFrame += 32;
        animationFrame |= SPRITE_ID_PALETTE_COLOUR_1(peep->hat_colour);
        gfx_draw_sprite(&clip_dpi, animationFrame, x, y, 0);
    }
}

/**
 *
 *  rct2: 0x69869b
 */
static void window_guest_stats_tab_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    if (w->disabled_widgets & (1 << WIDX_TAB_2))
        return;

    rct_widget* widget = &w->widgets[WIDX_TAB_2];
    int32_t x = widget->left + w->windowPos.x;
    int32_t y = widget->top + w->windowPos.y;

    Peep* peep = GET_PEEP(w->number);
    int32_t image_id = get_peep_face_sprite_large(peep);
    if (w->page == WINDOW_GUEST_STATS)
    {
        // If currently viewing this tab animate tab
        // if it is very sick or angry.
        switch (image_id)
        {
            case SPR_PEEP_LARGE_FACE_VERY_VERY_SICK_0:
                image_id += (w->frame_no / 4) & 0xF;
                break;
            case SPR_PEEP_LARGE_FACE_VERY_SICK_0:
                image_id += (w->frame_no / 8) & 0x3;
                break;
            case SPR_PEEP_LARGE_FACE_ANGRY_0:
                image_id += (w->frame_no / 8) & 0x3;
                break;
        }
    }
    gfx_draw_sprite(dpi, image_id, x, y, 0);
}

/**
 *
 *  rct2: 0x69861F
 */
static void window_guest_rides_tab_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    if (w->disabled_widgets & (1 << WIDX_TAB_3))
        return;

    rct_widget* widget = &w->widgets[WIDX_TAB_3];
    int32_t x = widget->left + w->windowPos.x;
    int32_t y = widget->top + w->windowPos.y;

    int32_t image_id = SPR_TAB_RIDE_0;

    if (w->page == WINDOW_GUEST_RIDES)
    {
        image_id += (w->frame_no / 4) & 0xF;
    }

    gfx_draw_sprite(dpi, image_id, x, y, 0);
}

/**
 *
 *  rct2: 0x698597
 */
static void window_guest_finance_tab_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    if (w->disabled_widgets & (1 << WIDX_TAB_4))
        return;

    rct_widget* widget = &w->widgets[WIDX_TAB_4];
    int32_t x = widget->left + w->windowPos.x;
    int32_t y = widget->top + w->windowPos.y;

    int32_t image_id = SPR_TAB_FINANCES_SUMMARY_0;

    if (w->page == WINDOW_GUEST_FINANCE)
    {
        image_id += (w->frame_no / 2) & 0x7;
    }

    gfx_draw_sprite(dpi, image_id, x, y, 0);
}

/**
 *
 *  rct2: 0x6985D8
 */
static void window_guest_thoughts_tab_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    if (w->disabled_widgets & (1 << WIDX_TAB_5))
        return;

    rct_widget* widget = &w->widgets[WIDX_TAB_5];
    int32_t x = widget->left + w->windowPos.x;
    int32_t y = widget->top + w->windowPos.y;

    int32_t image_id = SPR_TAB_THOUGHTS_0;

    if (w->page == WINDOW_GUEST_THOUGHTS)
    {
        image_id += (w->frame_no / 2) & 0x7;
    }

    gfx_draw_sprite(dpi, image_id, x, y, 0);
}

/**
 *
 *  rct2: 0x698661
 */
static void window_guest_inventory_tab_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    if (w->disabled_widgets & (1 << WIDX_TAB_6))
        return;

    rct_widget* widget = &w->widgets[WIDX_TAB_6];
    int32_t x = widget->left + w->windowPos.x;
    int32_t y = widget->top + w->windowPos.y;

    int32_t image_id = SPR_TAB_GUEST_INVENTORY;

    gfx_draw_sprite(dpi, image_id, x, y, 0);
}

static void window_guest_debug_tab_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    if (w->disabled_widgets & (1 << WIDX_TAB_7))
        return;

    rct_widget* widget = &w->widgets[WIDX_TAB_7];
    int32_t x = widget->left + w->windowPos.x;
    int32_t y = widget->top + w->windowPos.y;

    int32_t image_id = SPR_TAB_GEARS_0;
    if (w->page == WINDOW_GUEST_DEBUG)
    {
        image_id += (w->frame_no / 2) & 0x3;
    }

    gfx_draw_sprite(dpi, image_id, x, y, 0);
}

/**
 *
 *  rct2: 0x696887
 */
void window_guest_overview_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    window_draw_widgets(w, dpi);
    window_guest_overview_tab_paint(w, dpi);
    window_guest_stats_tab_paint(w, dpi);
    window_guest_rides_tab_paint(w, dpi);
    window_guest_finance_tab_paint(w, dpi);
    window_guest_thoughts_tab_paint(w, dpi);
    window_guest_inventory_tab_paint(w, dpi);
    window_guest_debug_tab_paint(w, dpi);

    // Draw the viewport no sound sprite
    if (w->viewport)
    {
        window_draw_viewport(dpi, w);
        rct_viewport* viewport = w->viewport;
        if (viewport->flags & VIEWPORT_FLAG_SOUND_ON)
        {
            gfx_draw_sprite(dpi, SPR_HEARING_VIEWPORT, w->windowPos.x + 2, w->windowPos.y + 2, 0);
        }
    }

    // Draw the centred label
    Peep* peep = GET_PEEP(w->number);
    peep->FormatActionTo(gCommonFormatArgs);
    rct_widget* widget = &w->widgets[WIDX_ACTION_LBL];
    int32_t x = (widget->left + widget->right) / 2 + w->windowPos.x;
    int32_t y = w->windowPos.y + widget->top - 1;
    int32_t width = widget->right - widget->left;
    gfx_draw_string_centred_clipped(dpi, STR_BLACK_STRING, gCommonFormatArgs, COLOUR_BLACK, x, y, width);

    // Draw the marquee thought
    widget = &w->widgets[WIDX_MARQUEE];
    width = widget->right - widget->left - 3;
    int32_t left = widget->left + 2 + w->windowPos.x;
    int32_t top = widget->top + w->windowPos.y;
    int32_t height = widget->bottom - widget->top;
    rct_drawpixelinfo dpi_marquee;
    if (!clip_drawpixelinfo(&dpi_marquee, dpi, left, top, width, height))
    {
        return;
    }

    int32_t i = 0;
    for (; i < PEEP_MAX_THOUGHTS; ++i)
    {
        if (peep->thoughts[i].type == PEEP_THOUGHT_TYPE_NONE)
        {
            w->list_information_type = 0;
            return;
        }
        if (peep->thoughts[i].freshness == 1)
        { // If a fresh thought
            break;
        }
    }
    if (i == PEEP_MAX_THOUGHTS)
    {
        w->list_information_type = 0;
        return;
    }

    x = widget->right - widget->left - w->list_information_type;
    peep_thought_set_format_args(&peep->thoughts[i]);
    gfx_draw_string_left(&dpi_marquee, STR_WINDOW_COLOUR_2_STRINGID, gCommonFormatArgs, COLOUR_BLACK, x, 0);
}

/**
 *
 *  rct2: 0x696749
 */
void window_guest_overview_invalidate(rct_window* w)
{
    window_guest_common_invalidate(w);

    auto peep = GET_PEEP(w->number);
    w->pressed_widgets &= ~(1 << WIDX_TRACK);
    if (peep->peep_flags & PEEP_FLAGS_TRACKING)
    {
        w->pressed_widgets |= (1 << WIDX_TRACK);
    }

    window_guest_overview_widgets[WIDX_VIEWPORT].right = w->width - 26;
    window_guest_overview_widgets[WIDX_VIEWPORT].bottom = w->height - 14;

    window_guest_overview_widgets[WIDX_ACTION_LBL].top = w->height - 12;
    window_guest_overview_widgets[WIDX_ACTION_LBL].bottom = w->height - 3;
    window_guest_overview_widgets[WIDX_ACTION_LBL].right = w->width - 24;

    window_guest_overview_widgets[WIDX_MARQUEE].right = w->width - 24;

    window_guest_overview_widgets[WIDX_PICKUP].right = w->width - 2;
    window_guest_overview_widgets[WIDX_RENAME].right = w->width - 2;
    window_guest_overview_widgets[WIDX_LOCATE].right = w->width - 2;
    window_guest_overview_widgets[WIDX_TRACK].right = w->width - 2;

    window_guest_overview_widgets[WIDX_PICKUP].left = w->width - 25;
    window_guest_overview_widgets[WIDX_RENAME].left = w->width - 25;
    window_guest_overview_widgets[WIDX_LOCATE].left = w->width - 25;
    window_guest_overview_widgets[WIDX_TRACK].left = w->width - 25;
}

/**
 *
 *  rct2: 0x696F45
 */
void window_guest_overview_update(rct_window* w)
{
    int32_t newAnimationFrame = w->var_496;
    newAnimationFrame++;
    newAnimationFrame %= 24;
    w->var_496 = newAnimationFrame;

    widget_invalidate(w, WIDX_TAB_1);
    widget_invalidate(w, WIDX_TAB_2);

    Peep* peep = GET_PEEP(w->number);
    if (peep != nullptr && peep->window_invalidate_flags & PEEP_INVALIDATE_PEEP_ACTION)
    {
        peep->window_invalidate_flags &= ~PEEP_INVALIDATE_PEEP_ACTION;
        widget_invalidate(w, WIDX_ACTION_LBL);
    }

    w->list_information_type += 2;

    if ((w->highlighted_item & 0xFFFF) == 0xFFFF)
        w->highlighted_item &= 0xFFFF0000;
    else
        w->highlighted_item++;

    // Disable peep watching thought for multiplayer as it's client specific
    if (network_get_mode() == NETWORK_MODE_NONE)
    {
        // Create the "I have the strangest feeling I am being watched thought"
        if ((w->highlighted_item & 0xFFFF) >= 3840)
        {
            if (!(w->highlighted_item & 0x3FF))
            {
                int32_t random = util_rand() & 0xFFFF;
                if (random <= 0x2AAA)
                {
                    peep->InsertNewThought(PEEP_THOUGHT_TYPE_WATCHED, PEEP_THOUGHT_ITEM_NONE);
                }
            }
        }
    }
}

/* rct2: 0x696A6A */
void window_guest_overview_text_input(rct_window* w, rct_widgetindex widgetIndex, char* text)
{
    if (widgetIndex != WIDX_RENAME)
        return;

    if (text == nullptr)
        return;
    guest_set_name(w->number, text);
}

/**
 *
 *  rct2: 0x696A5F
 */
void window_guest_overview_tool_update(rct_window* w, rct_widgetindex widgetIndex, const ScreenCoordsXY& screenCoords)
{
    if (widgetIndex != WIDX_PICKUP)
        return;

    map_invalidate_selection_rect();

    gMapSelectFlags &= ~MAP_SELECT_FLAG_ENABLE;

    auto mapCoords = footpath_get_coordinates_from_pos({ screenCoords.x, screenCoords.y + 16 }, nullptr, nullptr);
    if (!mapCoords.isNull())
    {
        gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE;
        gMapSelectType = MAP_SELECT_TYPE_FULL;
        gMapSelectPositionA = mapCoords;
        gMapSelectPositionB = mapCoords;
        map_invalidate_selection_rect();
    }

    gPickupPeepImage = UINT32_MAX;

    int32_t interactionType;
    CoordsXY unusedCoords;
    get_map_coordinates_from_pos(
        screenCoords, VIEWPORT_INTERACTION_MASK_NONE, unusedCoords, &interactionType, nullptr, nullptr);
    if (interactionType == VIEWPORT_INTERACTION_ITEM_NONE)
        return;

    gPickupPeepX = screenCoords.x - 1;
    gPickupPeepY = screenCoords.y + 16;
    w->picked_peep_frame++;
    if (w->picked_peep_frame >= 48)
    {
        w->picked_peep_frame = 0;
    }

    Peep* peep;
    peep = GET_PEEP(w->number);

    uint32_t imageId = g_peep_animation_entries[peep->sprite_type].sprite_animation[PEEP_ACTION_SPRITE_TYPE_UI].base_image;
    imageId += w->picked_peep_frame >> 2;

    imageId |= (peep->tshirt_colour << 19) | (peep->trousers_colour << 24) | IMAGE_TYPE_REMAP | IMAGE_TYPE_REMAP_2_PLUS;
    gPickupPeepImage = imageId;
}

/**
 *
 *  rct2: 0x696A54
 */
void window_guest_overview_tool_down(rct_window* w, rct_widgetindex widgetIndex, const ScreenCoordsXY& screenCoords)
{
    if (widgetIndex != WIDX_PICKUP)
        return;

    TileElement* tileElement;
    auto destCoords = footpath_get_coordinates_from_pos({ screenCoords.x, screenCoords.y + 16 }, nullptr, &tileElement);

    if (destCoords.isNull())
        return;

    PeepPickupAction pickupAction{
        PeepPickupType::Place, w->number, { destCoords, tileElement->base_height }, network_get_current_player_id()
    };
    pickupAction.SetCallback([](const GameAction* ga, const GameActionResult* result) {
        if (result->Error != GA_ERROR::OK)
            return;
        tool_cancel();
        gPickupPeepImage = UINT32_MAX;
    });
    GameActions::Execute(&pickupAction);
}

/**
 *
 *  rct2: 0x696A49
 */
void window_guest_overview_tool_abort(rct_window* w, rct_widgetindex widgetIndex)
{
    if (widgetIndex != WIDX_PICKUP)
        return;

    PeepPickupAction pickupAction{
        PeepPickupType::Cancel, w->number, { w->picked_peep_old_x, 0, 0 }, network_get_current_player_id()
    };
    GameActions::Execute(&pickupAction);
}

/**
 * This is a combination of 5 functions that were identical
 *  rct2: 0x69744F, 0x697795, 0x697BDD, 0x697E18, 0x698279
 */
void window_guest_mouse_up(rct_window* w, rct_widgetindex widgetIndex)
{
    switch (widgetIndex)
    {
        case WIDX_CLOSE:
            window_close(w);
            break;
        case WIDX_TAB_1:
        case WIDX_TAB_2:
        case WIDX_TAB_3:
        case WIDX_TAB_4:
        case WIDX_TAB_5:
        case WIDX_TAB_6:
        case WIDX_TAB_7:
            window_guest_set_page(w, widgetIndex - WIDX_TAB_1);
            break;
    }
}

/**
 *
 *  rct2: 0x69746A
 */
void window_guest_stats_update(rct_window* w)
{
    w->frame_no++;
    Peep* peep = GET_PEEP(w->number);
    peep->window_invalidate_flags &= ~PEEP_INVALIDATE_PEEP_STATS;

    w->Invalidate();
}

/**
 *
 *  rct2: 0x0066ECC1
 *
 *  ebp: colour, contains flag BAR_BLINK for blinking
 */
static void window_guest_stats_bars_paint(
    int32_t value, int32_t x, int32_t y, rct_window* w, rct_drawpixelinfo* dpi, int32_t colour)
{
    if (font_get_line_height(gCurrentFontSpriteBase) > 10)
    {
        y += 1;
    }

    gfx_fill_rect_inset(dpi, x + 61, y + 1, x + 61 + 121, y + 9, w->colours[1], INSET_RECT_F_30);

    int32_t blink_flag = colour & BAR_BLINK;
    colour &= ~BAR_BLINK;

    if (!blink_flag || game_is_paused() || (gCurrentRealTimeTicks & 8) == 0)
    {
        value *= 118;
        value >>= 8;

        if (value <= 2)
            return;

        gfx_fill_rect_inset(dpi, x + 63, y + 2, x + 63 + value - 1, y + 8, colour, 0);
    }
}

/**
 *
 *  rct2: 0x0069711D
 */
void window_guest_stats_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    window_draw_widgets(w, dpi);
    window_guest_overview_tab_paint(w, dpi);
    window_guest_stats_tab_paint(w, dpi);
    window_guest_rides_tab_paint(w, dpi);
    window_guest_finance_tab_paint(w, dpi);
    window_guest_thoughts_tab_paint(w, dpi);
    window_guest_inventory_tab_paint(w, dpi);
    window_guest_debug_tab_paint(w, dpi);

    // ebx
    Peep* peep = GET_PEEP(w->number);

    // Not sure why this is not stats widgets
    // cx
    int32_t x = w->windowPos.x + window_guest_rides_widgets[WIDX_PAGE_BACKGROUND].left + 4;
    // dx
    int32_t y = w->windowPos.y + window_guest_rides_widgets[WIDX_PAGE_BACKGROUND].top + 4;

    // Happiness
    gfx_draw_string_left(dpi, STR_GUEST_STAT_HAPPINESS_LABEL, gCommonFormatArgs, COLOUR_BLACK, x, y);

    int32_t happiness = peep->happiness;
    if (happiness < 10)
        happiness = 10;
    int32_t ebp = COLOUR_BRIGHT_GREEN;
    if (happiness < 50)
    {
        ebp |= BAR_BLINK;
    }
    window_guest_stats_bars_paint(happiness, x, y, w, dpi, ebp);

    // Energy
    y += LIST_ROW_HEIGHT;
    gfx_draw_string_left(dpi, STR_GUEST_STAT_ENERGY_LABEL, gCommonFormatArgs, COLOUR_BLACK, x, y);

    int32_t energy = ((peep->energy - PEEP_MIN_ENERGY) * 255) / (PEEP_MAX_ENERGY - PEEP_MIN_ENERGY);
    ebp = COLOUR_BRIGHT_GREEN;
    if (energy < 50)
    {
        ebp |= BAR_BLINK;
    }
    if (energy < 10)
        energy = 10;
    window_guest_stats_bars_paint(energy, x, y, w, dpi, ebp);

    // Hunger
    y += LIST_ROW_HEIGHT;
    gfx_draw_string_left(dpi, STR_GUEST_STAT_HUNGER_LABEL, gCommonFormatArgs, COLOUR_BLACK, x, y);

    int32_t hunger = peep->hunger;
    if (hunger > 190)
        hunger = 190;

    hunger -= 32;
    if (hunger < 0)
        hunger = 0;
    hunger *= 51;
    hunger /= 32;
    hunger = 0xFF & ~hunger;

    ebp = COLOUR_BRIGHT_RED;
    if (hunger > 170)
    {
        ebp |= BAR_BLINK;
    }
    window_guest_stats_bars_paint(hunger, x, y, w, dpi, ebp);

    // Thirst
    y += LIST_ROW_HEIGHT;
    gfx_draw_string_left(dpi, STR_GUEST_STAT_THIRST_LABEL, gCommonFormatArgs, COLOUR_BLACK, x, y);

    int32_t thirst = peep->thirst;
    if (thirst > 190)
        thirst = 190;

    thirst -= 32;
    if (thirst < 0)
        thirst = 0;
    thirst *= 51;
    thirst /= 32;
    thirst = 0xFF & ~thirst;

    ebp = COLOUR_BRIGHT_RED;
    if (thirst > 170)
    {
        ebp |= BAR_BLINK;
    }
    window_guest_stats_bars_paint(thirst, x, y, w, dpi, ebp);

    // Nausea
    y += LIST_ROW_HEIGHT;
    gfx_draw_string_left(dpi, STR_GUEST_STAT_NAUSEA_LABEL, gCommonFormatArgs, COLOUR_BLACK, x, y);

    int32_t nausea = peep->nausea - 32;

    if (nausea < 0)
        nausea = 0;
    nausea *= 36;
    nausea /= 32;

    ebp = COLOUR_BRIGHT_RED;
    if (nausea > 120)
    {
        ebp |= BAR_BLINK;
    }
    window_guest_stats_bars_paint(nausea, x, y, w, dpi, ebp);

    // Toilet
    y += LIST_ROW_HEIGHT;
    gfx_draw_string_left(dpi, STR_GUEST_STAT_TOILET_LABEL, gCommonFormatArgs, COLOUR_BLACK, x, y);

    int32_t toilet = peep->toilet - 32;
    if (toilet > 210)
        toilet = 210;

    toilet -= 32;
    if (toilet < 0)
        toilet = 0;
    toilet *= 45;
    toilet /= 32;

    ebp = COLOUR_BRIGHT_RED;
    if (toilet > 160)
    {
        ebp |= BAR_BLINK;
    }
    window_guest_stats_bars_paint(toilet, x, y, w, dpi, ebp);

    // Time in park
    y += LIST_ROW_HEIGHT + 1;
    if (peep->time_in_park != -1)
    {
        int32_t eax = gScenarioTicks;
        eax -= peep->time_in_park;
        eax >>= 11;
        set_format_arg(0, uint16_t, eax & 0xFFFF);
        gfx_draw_string_left(dpi, STR_GUEST_STAT_TIME_IN_PARK, gCommonFormatArgs, COLOUR_BLACK, x, y);
    }

    y += LIST_ROW_HEIGHT + 9;
    gfx_fill_rect_inset(dpi, x, y - 6, x + 179, y - 5, w->colours[1], INSET_RECT_FLAG_BORDER_INSET);

    // Preferred Ride
    gfx_draw_string_left(dpi, STR_GUEST_STAT_PREFERRED_RIDE, nullptr, COLOUR_BLACK, x, y);
    y += LIST_ROW_HEIGHT;

    // Intensity
    int32_t intensity = peep->intensity / 16;
    set_format_arg(0, uint16_t, intensity);
    int32_t string_id = STR_GUEST_STAT_PREFERRED_INTESITY_BELOW;
    if (peep->intensity & 0xF)
    {
        set_format_arg(0, uint16_t, peep->intensity & 0xF);
        set_format_arg(2, uint16_t, intensity);
        string_id = STR_GUEST_STAT_PREFERRED_INTESITY_BETWEEN;
        if (intensity == 15)
            string_id = STR_GUEST_STAT_PREFERRED_INTESITY_ABOVE;
    }

    gfx_draw_string_left(dpi, string_id, gCommonFormatArgs, COLOUR_BLACK, x + 4, y);

    // Nausea tolerance
    static constexpr const rct_string_id nauseaTolerances[] = {
        STR_PEEP_STAT_NAUSEA_TOLERANCE_NONE,
        STR_PEEP_STAT_NAUSEA_TOLERANCE_LOW,
        STR_PEEP_STAT_NAUSEA_TOLERANCE_AVERAGE,
        STR_PEEP_STAT_NAUSEA_TOLERANCE_HIGH,
    };
    y += LIST_ROW_HEIGHT;
    int32_t nausea_tolerance = peep->nausea_tolerance & 0x3;
    set_format_arg(0, rct_string_id, nauseaTolerances[nausea_tolerance]);
    gfx_draw_string_left(dpi, STR_GUEST_STAT_NAUSEA_TOLERANCE, gCommonFormatArgs, COLOUR_BLACK, x, y);
}

/**
 *
 *  rct2: 0x6977B0
 */
void window_guest_rides_update(rct_window* w)
{
    w->frame_no++;

    widget_invalidate(w, WIDX_TAB_2);
    widget_invalidate(w, WIDX_TAB_3);

    auto peep = GET_PEEP(w->number);
    auto guest = peep->AsGuest();
    if (guest != nullptr)
    {
        // Every 2048 ticks do a full window_invalidate
        int32_t number_of_ticks = gScenarioTicks - peep->time_in_park;
        if (!(number_of_ticks & 0x7FF))
            w->Invalidate();

        uint8_t curr_list_position = 0;
        for (const auto& ride : GetRideManager())
        {
            if (ride.IsRide() && guest->HasRidden(&ride))
            {
                w->list_item_positions[curr_list_position] = ride.id;
                curr_list_position++;
            }
        }

        // If there are new items
        if (w->no_list_items != curr_list_position)
        {
            w->no_list_items = curr_list_position;
            w->Invalidate();
        }
    }
}

/**
 *
 *  rct2: 0x69784E
 */
void window_guest_rides_scroll_get_size(rct_window* w, int32_t scrollIndex, int32_t* width, int32_t* height)
{
    *height = w->no_list_items * 10;

    if (w->selected_list_item != -1)
    {
        w->selected_list_item = -1;
        w->Invalidate();
    }

    int32_t visable_height = *height - window_guest_rides_widgets[WIDX_RIDE_SCROLL].bottom
        + window_guest_rides_widgets[WIDX_RIDE_SCROLL].top + 21;

    if (visable_height < 0)
        visable_height = 0;

    if (visable_height < w->scrolls[0].v_top)
    {
        w->scrolls[0].v_top = visable_height;
        w->Invalidate();
    }
}

/**
 *
 *  rct2: 0x006978CC
 */
void window_guest_rides_scroll_mouse_down(rct_window* w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords)
{
    int32_t index;

    index = screenCoords.y / 10;
    if (index >= w->no_list_items)
        return;

    auto intent = Intent(WC_RIDE);
    intent.putExtra(INTENT_EXTRA_RIDE_ID, w->list_item_positions[index]);
    context_open_intent(&intent);
}

/**
 *
 *  rct2: 0x0069789C
 */
void window_guest_rides_scroll_mouse_over(rct_window* w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords)
{
    int32_t index;

    index = screenCoords.y / 10;
    if (index >= w->no_list_items)
        return;

    if (index == w->selected_list_item)
        return;
    w->selected_list_item = index;

    w->Invalidate();
}

/**
 *
 *  rct2: 0x0069757A
 */
void window_guest_rides_invalidate(rct_window* w)
{
    window_guest_common_invalidate(w);

    window_guest_rides_widgets[WIDX_RIDE_SCROLL].right = w->width - 4;
    window_guest_rides_widgets[WIDX_RIDE_SCROLL].bottom = w->height - 15;
}

/**
 *
 *  rct2: 0x00697637
 */
void window_guest_rides_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    window_draw_widgets(w, dpi);
    window_guest_overview_tab_paint(w, dpi);
    window_guest_stats_tab_paint(w, dpi);
    window_guest_rides_tab_paint(w, dpi);
    window_guest_finance_tab_paint(w, dpi);
    window_guest_thoughts_tab_paint(w, dpi);
    window_guest_inventory_tab_paint(w, dpi);
    window_guest_debug_tab_paint(w, dpi);

    Peep* peep = GET_PEEP(w->number);

    // cx
    int32_t x = w->windowPos.x + window_guest_rides_widgets[WIDX_PAGE_BACKGROUND].left + 2;
    // dx
    int32_t y = w->windowPos.y + window_guest_rides_widgets[WIDX_PAGE_BACKGROUND].top + 2;

    gfx_draw_string_left(dpi, STR_GUEST_LABEL_RIDES_BEEN_ON, nullptr, COLOUR_BLACK, x, y);

    y = w->windowPos.y + window_guest_rides_widgets[WIDX_PAGE_BACKGROUND].bottom - 12;

    set_format_arg(0, rct_string_id, STR_PEEP_FAVOURITE_RIDE_NOT_AVAILABLE);
    if (peep->favourite_ride != RIDE_ID_NULL)
    {
        auto ride = get_ride(peep->favourite_ride);
        if (ride != nullptr)
        {
            ride->FormatNameTo(gCommonFormatArgs);
        }
    }
    gfx_draw_string_left_clipped(dpi, STR_FAVOURITE_RIDE, gCommonFormatArgs, COLOUR_BLACK, x, y, w->width - 14);
}

/**
 *
 *  rct2: 0x006976FC
 */
void window_guest_rides_scroll_paint(rct_window* w, rct_drawpixelinfo* dpi, int32_t scrollIndex)
{
    int32_t left = dpi->x;
    int32_t right = dpi->x + dpi->width - 1;
    int32_t top = dpi->y;
    int32_t bottom = dpi->y + dpi->height - 1;

    auto colour = ColourMapA[w->colours[1]].mid_light;
    gfx_fill_rect(dpi, left, top, right, bottom, colour);

    for (int32_t list_index = 0; list_index < w->no_list_items; list_index++)
    {
        auto y = list_index * 10;
        rct_string_id stringId = STR_BLACK_STRING;
        if (list_index == w->selected_list_item)
        {
            gfx_filter_rect(dpi, 0, y, 800, y + 9, PALETTE_DARKEN_1);
            stringId = STR_WINDOW_COLOUR_2_STRINGID;
        }

        auto ride = get_ride(w->list_item_positions[list_index]);
        if (ride != nullptr)
        {
            ride->FormatNameTo(gCommonFormatArgs);
            gfx_draw_string_left(dpi, stringId, gCommonFormatArgs, COLOUR_BLACK, 0, y - 1);
        }
    }
}

/**
 *
 *  rct2: 0x00697BF8
 */
void window_guest_finance_update(rct_window* w)
{
    w->frame_no++;

    widget_invalidate(w, WIDX_TAB_2);
    widget_invalidate(w, WIDX_TAB_4);
}

/**
 *
 *  rct2: 0x00697A08
 */
void window_guest_finance_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    window_draw_widgets(w, dpi);
    window_guest_overview_tab_paint(w, dpi);
    window_guest_stats_tab_paint(w, dpi);
    window_guest_rides_tab_paint(w, dpi);
    window_guest_finance_tab_paint(w, dpi);
    window_guest_thoughts_tab_paint(w, dpi);
    window_guest_inventory_tab_paint(w, dpi);
    window_guest_debug_tab_paint(w, dpi);

    Peep* peep = GET_PEEP(w->number);

    // cx
    int32_t x = w->windowPos.x + window_guest_finance_widgets[WIDX_PAGE_BACKGROUND].left + 4;
    // dx
    int32_t y = w->windowPos.y + window_guest_finance_widgets[WIDX_PAGE_BACKGROUND].top + 4;

    // Cash in pocket
    set_format_arg(0, money32, peep->cash_in_pocket);
    gfx_draw_string_left(dpi, STR_GUEST_STAT_CASH_IN_POCKET, gCommonFormatArgs, COLOUR_BLACK, x, y);

    // Cash spent
    y += LIST_ROW_HEIGHT;
    set_format_arg(0, money32, peep->cash_spent);
    gfx_draw_string_left(dpi, STR_GUEST_STAT_CASH_SPENT, gCommonFormatArgs, COLOUR_BLACK, x, y);

    y += LIST_ROW_HEIGHT * 2;
    gfx_fill_rect_inset(dpi, x, y - 6, x + 179, y - 5, w->colours[1], INSET_RECT_FLAG_BORDER_INSET);

    // Paid to enter
    set_format_arg(0, money32, peep->paid_to_enter);
    gfx_draw_string_left(dpi, STR_GUEST_EXPENSES_ENTRANCE_FEE, gCommonFormatArgs, COLOUR_BLACK, x, y);

    // Paid on rides
    y += LIST_ROW_HEIGHT;
    set_format_arg(0, money32, peep->paid_on_rides);
    set_format_arg(4, uint16_t, peep->no_of_rides);
    if (peep->no_of_rides != 1)
    {
        gfx_draw_string_left(dpi, STR_GUEST_EXPENSES_RIDE_PLURAL, gCommonFormatArgs, COLOUR_BLACK, x, y);
    }
    else
    {
        gfx_draw_string_left(dpi, STR_GUEST_EXPENSES_RIDE, gCommonFormatArgs, COLOUR_BLACK, x, y);
    }

    // Paid on food
    y += LIST_ROW_HEIGHT;
    set_format_arg(0, money32, peep->paid_on_food);
    set_format_arg(4, uint16_t, peep->no_of_food);
    if (peep->no_of_food != 1)
    {
        gfx_draw_string_left(dpi, STR_GUEST_EXPENSES_FOOD_PLURAL, gCommonFormatArgs, COLOUR_BLACK, x, y);
    }
    else
    {
        gfx_draw_string_left(dpi, STR_GUEST_EXPENSES_FOOD, gCommonFormatArgs, COLOUR_BLACK, x, y);
    }

    // Paid on drinks
    y += LIST_ROW_HEIGHT;
    set_format_arg(0, money32, peep->paid_on_drink);
    set_format_arg(4, uint16_t, peep->no_of_drinks);
    if (peep->no_of_drinks != 1)
    {
        gfx_draw_string_left(dpi, STR_GUEST_EXPENSES_DRINK_PLURAL, gCommonFormatArgs, COLOUR_BLACK, x, y);
    }
    else
    {
        gfx_draw_string_left(dpi, STR_GUEST_EXPENSES_DRINK, gCommonFormatArgs, COLOUR_BLACK, x, y);
    }

    // Paid on souvenirs
    y += LIST_ROW_HEIGHT;
    set_format_arg(0, money32, peep->paid_on_souvenirs);
    set_format_arg(4, uint16_t, peep->no_of_souvenirs);
    if (peep->no_of_souvenirs != 1)
    {
        gfx_draw_string_left(dpi, STR_GUEST_EXPENSES_SOUVENIR_PLURAL, gCommonFormatArgs, COLOUR_BLACK, x, y);
    }
    else
    {
        gfx_draw_string_left(dpi, STR_GUEST_EXPENSES_SOUVENIR, gCommonFormatArgs, COLOUR_BLACK, x, y);
    }
}

/**
 *
 *  rct2: 0x00697EB4
 */
void window_guest_thoughts_update(rct_window* w)
{
    w->frame_no++;

    widget_invalidate(w, WIDX_TAB_2);
    widget_invalidate(w, WIDX_TAB_5);

    auto peep = GET_PEEP(w->number);
    if (peep->window_invalidate_flags & PEEP_INVALIDATE_PEEP_THOUGHTS)
    {
        peep->window_invalidate_flags &= ~PEEP_INVALIDATE_PEEP_THOUGHTS;
        w->Invalidate();
    }
}

/**
 *
 *  rct2: 0x00697D2A
 */
void window_guest_thoughts_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    window_draw_widgets(w, dpi);
    window_guest_overview_tab_paint(w, dpi);
    window_guest_stats_tab_paint(w, dpi);
    window_guest_rides_tab_paint(w, dpi);
    window_guest_finance_tab_paint(w, dpi);
    window_guest_thoughts_tab_paint(w, dpi);
    window_guest_inventory_tab_paint(w, dpi);
    window_guest_debug_tab_paint(w, dpi);

    Peep* peep = GET_PEEP(w->number);

    // cx
    int32_t x = w->windowPos.x + window_guest_thoughts_widgets[WIDX_PAGE_BACKGROUND].left + 4;
    // dx
    int32_t y = w->windowPos.y + window_guest_thoughts_widgets[WIDX_PAGE_BACKGROUND].top + 4;

    gfx_draw_string_left(dpi, STR_GUEST_RECENT_THOUGHTS_LABEL, nullptr, COLOUR_BLACK, x, y);

    y += 10;
    for (rct_peep_thought* thought = peep->thoughts; thought < &peep->thoughts[PEEP_MAX_THOUGHTS]; ++thought)
    {
        if (thought->type == PEEP_THOUGHT_TYPE_NONE)
            return;
        if (thought->freshness == 0)
            continue;

        int32_t width = window_guest_thoughts_widgets[WIDX_PAGE_BACKGROUND].right
            - window_guest_thoughts_widgets[WIDX_PAGE_BACKGROUND].left - 8;

        peep_thought_set_format_args(thought);
        y += gfx_draw_string_left_wrapped(dpi, gCommonFormatArgs, x, y, width, STR_BLACK_STRING, COLOUR_BLACK);

        // If this is the last visible line end drawing.
        if (y > w->windowPos.y + window_guest_thoughts_widgets[WIDX_PAGE_BACKGROUND].bottom - 32)
            return;
    }
}

/**
 *
 *  rct2: 0x00698315
 */
void window_guest_inventory_update(rct_window* w)
{
    w->frame_no++;

    widget_invalidate(w, WIDX_TAB_2);
    widget_invalidate(w, WIDX_TAB_6);

    auto peep = GET_PEEP(w->number);
    if (peep->window_invalidate_flags & PEEP_INVALIDATE_PEEP_INVENTORY)
    {
        peep->window_invalidate_flags &= ~PEEP_INVALIDATE_PEEP_INVENTORY;
        w->Invalidate();
    }
}

static rct_string_id window_guest_inventory_format_item(Peep* peep, int32_t item)
{
    auto& park = OpenRCT2::GetContext()->GetGameState()->GetPark();
    auto parkName = park.Name.c_str();

    // Default arguments
    set_format_arg(0, uint32_t, ShopItems[item].Image);
    set_format_arg(4, rct_string_id, ShopItems[item].Naming.Display);
    set_format_arg(6, rct_string_id, STR_STRING);
    set_format_arg(8, const char*, parkName);

    // Special overrides
    Ride* ride{};
    switch (item)
    {
        case SHOP_ITEM_BALLOON:
            set_format_arg(0, uint32_t, SPRITE_ID_PALETTE_COLOUR_1(peep->balloon_colour) | ShopItems[item].Image);
            break;
        case SHOP_ITEM_PHOTO:
            ride = get_ride(peep->photo1_ride_ref);
            if (ride != nullptr)
                ride->FormatNameTo(gCommonFormatArgs + 6);
            break;
        case SHOP_ITEM_UMBRELLA:
            set_format_arg(0, uint32_t, SPRITE_ID_PALETTE_COLOUR_1(peep->umbrella_colour) | ShopItems[item].Image);
            break;
        case SHOP_ITEM_VOUCHER:
            switch (peep->voucher_type)
            {
                case VOUCHER_TYPE_PARK_ENTRY_FREE:
                    set_format_arg(6, rct_string_id, STR_PEEP_INVENTORY_VOUCHER_PARK_ENTRY_FREE);
                    set_format_arg(8, rct_string_id, STR_STRING);
                    set_format_arg(10, const char*, parkName);
                    break;
                case VOUCHER_TYPE_RIDE_FREE:
                    ride = get_ride(peep->voucher_arguments);
                    if (ride != nullptr)
                    {
                        set_format_arg(6, rct_string_id, STR_PEEP_INVENTORY_VOUCHER_RIDE_FREE);
                        ride->FormatNameTo(gCommonFormatArgs + 8);
                    }
                    break;
                case VOUCHER_TYPE_PARK_ENTRY_HALF_PRICE:
                    set_format_arg(6, rct_string_id, STR_PEEP_INVENTORY_VOUCHER_PARK_ENTRY_HALF_PRICE);
                    set_format_arg(8, rct_string_id, STR_STRING);
                    set_format_arg(10, const char*, parkName);
                    break;
                case VOUCHER_TYPE_FOOD_OR_DRINK_FREE:
                    set_format_arg(6, rct_string_id, STR_PEEP_INVENTORY_VOUCHER_FOOD_OR_DRINK_FREE);
                    set_format_arg(8, rct_string_id, ShopItems[peep->voucher_arguments].Naming.Singular);
                    break;
            }
            break;
        case SHOP_ITEM_HAT:
            set_format_arg(0, uint32_t, SPRITE_ID_PALETTE_COLOUR_1(peep->hat_colour) | ShopItems[item].Image);
            break;
        case SHOP_ITEM_TSHIRT:
            set_format_arg(0, uint32_t, SPRITE_ID_PALETTE_COLOUR_1(peep->tshirt_colour) | ShopItems[item].Image);
            break;
        case SHOP_ITEM_PHOTO2:
            ride = get_ride(peep->photo2_ride_ref);
            if (ride != nullptr)
                ride->FormatNameTo(gCommonFormatArgs + 6);
            break;
        case SHOP_ITEM_PHOTO3:
            ride = get_ride(peep->photo3_ride_ref);
            if (ride != nullptr)
                ride->FormatNameTo(gCommonFormatArgs + 6);
            break;
        case SHOP_ITEM_PHOTO4:
            ride = get_ride(peep->photo4_ride_ref);
            if (ride != nullptr)
                ride->FormatNameTo(gCommonFormatArgs + 6);
            break;
    }

    return STR_GUEST_ITEM_FORMAT;
}

/**
 *
 *  rct2: 0x00697F81
 */
void window_guest_inventory_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    window_draw_widgets(w, dpi);
    window_guest_overview_tab_paint(w, dpi);
    window_guest_stats_tab_paint(w, dpi);
    window_guest_rides_tab_paint(w, dpi);
    window_guest_finance_tab_paint(w, dpi);
    window_guest_thoughts_tab_paint(w, dpi);
    window_guest_inventory_tab_paint(w, dpi);
    window_guest_debug_tab_paint(w, dpi);

    const auto guest = (GET_PEEP(w->number))->AsGuest();
    if (guest != nullptr)
    {
        rct_widget* pageBackgroundWidget = &window_guest_inventory_widgets[WIDX_PAGE_BACKGROUND];
        int32_t x = w->windowPos.x + pageBackgroundWidget->left + 4;
        int32_t y = w->windowPos.y + pageBackgroundWidget->top + 2;
        int32_t itemNameWidth = pageBackgroundWidget->right - pageBackgroundWidget->left - 8;

        int32_t maxY = w->windowPos.y + w->height - 22;
        int32_t numItems = 0;

        gfx_draw_string_left(dpi, STR_CARRYING, nullptr, COLOUR_BLACK, x, y);
        y += 10;

        for (int32_t item = 0; item < SHOP_ITEM_COUNT; item++)
        {
            if (y >= maxY)
                break;
            if (!guest->HasItem(item))
                continue;

            rct_string_id stringId = window_guest_inventory_format_item(guest, item);
            y += gfx_draw_string_left_wrapped(dpi, gCommonFormatArgs, x, y, itemNameWidth, stringId, COLOUR_BLACK);
            numItems++;
        }

        if (numItems == 0)
        {
            gfx_draw_string_left(dpi, STR_NOTHING, nullptr, COLOUR_BLACK, x, y);
        }
    }
}

/**
 *
 *  rct2: 0x00698315
 */
void window_guest_debug_update(rct_window* w)
{
    w->frame_no++;
    w->Invalidate();
}

void window_guest_debug_paint(rct_window* w, rct_drawpixelinfo* dpi)
{
    char buffer[512]{};
    char buffer2[512]{};

    window_draw_widgets(w, dpi);
    window_guest_overview_tab_paint(w, dpi);
    window_guest_stats_tab_paint(w, dpi);
    window_guest_rides_tab_paint(w, dpi);
    window_guest_finance_tab_paint(w, dpi);
    window_guest_thoughts_tab_paint(w, dpi);
    window_guest_inventory_tab_paint(w, dpi);
    window_guest_debug_tab_paint(w, dpi);

    auto peep = GET_PEEP(w->number);
    auto x = w->windowPos.x + window_guest_debug_widgets[WIDX_PAGE_BACKGROUND].left + 4;
    auto y = w->windowPos.y + window_guest_debug_widgets[WIDX_PAGE_BACKGROUND].top + 4;
    {
        set_format_arg(0, uint32_t, peep->sprite_index);
        gfx_draw_string_left(dpi, STR_PEEP_DEBUG_SPRITE_INDEX, gCommonFormatArgs, 0, x, y);
    }
    y += LIST_ROW_HEIGHT;
    {
        int32_t args[] = { peep->x, peep->y, peep->x };
        gfx_draw_string_left(dpi, STR_PEEP_DEBUG_POSITION, args, 0, x, y);
    }
    y += LIST_ROW_HEIGHT;
    {
        int32_t args[] = { peep->NextLoc.x, peep->NextLoc.y, peep->NextLoc.z };
        format_string(buffer, sizeof(buffer), STR_PEEP_DEBUG_NEXT, args);
        if (peep->GetNextIsSurface())
        {
            format_string(buffer2, sizeof(buffer2), STR_PEEP_DEBUG_NEXT_SURFACE, nullptr);
            safe_strcat(buffer, buffer2, sizeof(buffer));
        }
        if (peep->GetNextIsSloped())
        {
            int32_t args2[1] = { peep->GetNextDirection() };
            format_string(buffer2, sizeof(buffer2), STR_PEEP_DEBUG_NEXT_SLOPE, args2);
            safe_strcat(buffer, buffer2, sizeof(buffer));
        }
        gfx_draw_string(dpi, buffer, 0, x, y);
    }
    y += LIST_ROW_HEIGHT;
    {
        int32_t args[] = { peep->destination_x, peep->destination_y, peep->destination_tolerance };
        gfx_draw_string_left(dpi, STR_PEEP_DEBUG_DEST, args, 0, x, y);
    }
    y += LIST_ROW_HEIGHT;
    {
        int32_t args[] = { peep->pathfind_goal.x, peep->pathfind_goal.y, peep->pathfind_goal.z, peep->pathfind_goal.direction };
        gfx_draw_string_left(dpi, STR_PEEP_DEBUG_PATHFIND_GOAL, args, 0, x, y);
    }
    y += LIST_ROW_HEIGHT;
    gfx_draw_string_left(dpi, STR_PEEP_DEBUG_PATHFIND_HISTORY, nullptr, 0, x, y);
    y += LIST_ROW_HEIGHT;

    x += 10;
    for (auto& point : peep->pathfind_history)
    {
        int32_t args[] = { point.x, point.y, point.z, point.direction };
        gfx_draw_string_left(dpi, STR_PEEP_DEBUG_PATHFIND_HISTORY_ITEM, args, 0, x, y);
        y += LIST_ROW_HEIGHT;
    }
    x -= 10;
}
