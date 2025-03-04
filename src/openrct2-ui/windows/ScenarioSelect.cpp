/*****************************************************************************
 * Copyright (c) 2014-2022 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "../interface/Theme.h"

#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/windows/Window.h>
#include <openrct2/audio/audio.h>
#include <openrct2/config/Config.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/localisation/Date.h>
#include <openrct2/localisation/Formatter.h>
#include <openrct2/localisation/Localisation.h>
#include <openrct2/localisation/LocalisationService.h>
#include <openrct2/ride/RideData.h>
#include <openrct2/scenario/Scenario.h>
#include <openrct2/scenario/ScenarioRepository.h>
#include <openrct2/scenario/ScenarioSources.h>
#include <openrct2/sprites.h>
#include <openrct2/util/Util.h>
#include <vector>

static constexpr const StringId WINDOW_TITLE = STR_SELECT_SCENARIO;
static constexpr const int32_t WW = 734;
static constexpr const int32_t WH = 384;
static constexpr const int32_t SidebarWidth = 180;
static constexpr const int32_t TabWidth = 92;
static constexpr const int32_t TabHeight = 34;
static constexpr const int32_t WidgetsStart = 17;
static constexpr const int32_t TabsStart = WidgetsStart;
#define INITIAL_NUM_UNLOCKED_SCENARIOS 5
constexpr const uint8_t NumTabs = 10;

// clang-format off
enum class ListItemType : uint8_t
{
    Heading,
    Scenario,
};

struct ScenarioListItem
{
    ListItemType type;
    union
    {
        struct
        {
            StringId string_id;
        } heading;
        struct
        {
            const scenario_index_entry * scenario;
            bool is_locked;
        } scenario;
    };
};

static std::vector<ScenarioListItem> _listItems;

enum {
    WIDX_BACKGROUND,
    WIDX_TITLEBAR,
    WIDX_CLOSE,
    WIDX_TABCONTENT,
    WIDX_TAB1,
    WIDX_TAB2,
    WIDX_TAB3,
    WIDX_TAB4,
    WIDX_TAB5,
    WIDX_TAB6,
    WIDX_TAB7,
    WIDX_TAB8,
    WIDX_TAB9,
    WIDX_TAB10,
    WIDX_SCENARIOLIST
};

static rct_widget window_scenarioselect_widgets[] = {
    WINDOW_SHIM(WINDOW_TITLE, WW, WH),
    MakeWidget     ({ TabWidth + 1, WidgetsStart }, { WW, 284}, WindowWidgetType::Resize, WindowColour::Secondary),                                                 // tab content panel
    MakeRemapWidget({  3, TabsStart + (TabHeight * 0) }, { TabWidth,  TabHeight}, WindowWidgetType::Tab,    WindowColour::Secondary, SPR_G2_SIDEWAYS_TAB),   // tab 01
    MakeRemapWidget({  3, TabsStart + (TabHeight * 1) }, { TabWidth,  TabHeight}, WindowWidgetType::Tab,    WindowColour::Secondary, SPR_G2_SIDEWAYS_TAB),   // tab 02
    MakeRemapWidget({  3, TabsStart + (TabHeight * 2) }, { TabWidth,  TabHeight}, WindowWidgetType::Tab,    WindowColour::Secondary, SPR_G2_SIDEWAYS_TAB),   // tab 03
    MakeRemapWidget({  3, TabsStart + (TabHeight * 3) }, { TabWidth,  TabHeight}, WindowWidgetType::Tab,    WindowColour::Secondary, SPR_G2_SIDEWAYS_TAB),   // tab 04
    MakeRemapWidget({  3, TabsStart + (TabHeight * 4) }, { TabWidth,  TabHeight}, WindowWidgetType::Tab,    WindowColour::Secondary, SPR_G2_SIDEWAYS_TAB),   // tab 05
    MakeRemapWidget({  3, TabsStart + (TabHeight * 5) }, { TabWidth,  TabHeight}, WindowWidgetType::Tab,    WindowColour::Secondary, SPR_G2_SIDEWAYS_TAB),   // tab 06
    MakeRemapWidget({  3, TabsStart + (TabHeight * 6) }, { TabWidth,  TabHeight}, WindowWidgetType::Tab,    WindowColour::Secondary, SPR_G2_SIDEWAYS_TAB),   // tab 07
    MakeRemapWidget({  3, TabsStart + (TabHeight * 7) }, { TabWidth,  TabHeight}, WindowWidgetType::Tab,    WindowColour::Secondary, SPR_G2_SIDEWAYS_TAB),   // tab 08
    MakeRemapWidget({  3, TabsStart + (TabHeight * 8) }, { TabWidth,  TabHeight}, WindowWidgetType::Tab,    WindowColour::Secondary, SPR_G2_SIDEWAYS_TAB),   // tab 09
    MakeRemapWidget({  3, TabsStart + (TabHeight * 8) }, { TabWidth,  TabHeight}, WindowWidgetType::Tab,    WindowColour::Secondary, SPR_G2_SIDEWAYS_TAB),   // tab 10
    MakeWidget     ({  TabWidth + 3, WidgetsStart + 1 }, { WW - SidebarWidth, 276 }, WindowWidgetType::Scroll, WindowColour::Secondary, SCROLL_VERTICAL),    // level list
    WIDGETS_END,
};

static constexpr const StringId ScenarioOriginStringIds[] = {
    STR_SCENARIO_CATEGORY_RCT1,
    STR_SCENARIO_CATEGORY_RCT1_AA,
    STR_SCENARIO_CATEGORY_RCT1_LL,
    STR_SCENARIO_CATEGORY_RCT2,
    STR_SCENARIO_CATEGORY_RCT2_WW,
    STR_SCENARIO_CATEGORY_RCT2_TT,
    STR_SCENARIO_CATEGORY_UCES,
    STR_SCENARIO_CATEGORY_REAL_PARKS,
    STR_SCENARIO_CATEGORY_EXTRAS_PARKS,
    STR_SCENARIO_CATEGORY_OTHER_PARKS,
};

static void WindowScenarioselectInitTabs(rct_window *w);

static void WindowScenarioselectClose(rct_window *w);
static void WindowScenarioselectMouseup(rct_window *w, WidgetIndex widgetIndex);
static void WindowScenarioselectMousedown(rct_window *w, WidgetIndex widgetIndex, rct_widget* widget);
static void WindowScenarioselectScrollgetsize(rct_window *w, int32_t scrollIndex, int32_t *width, int32_t *height);
static void WindowScenarioselectScrollmousedown(rct_window *w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords);
static void WindowScenarioselectScrollmouseover(rct_window *w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords);
static void WindowScenarioselectInvalidate(rct_window *w);
static void WindowScenarioselectPaint(rct_window *w, rct_drawpixelinfo *dpi);
static void WindowScenarioselectScrollpaint(rct_window *w, rct_drawpixelinfo *dpi, int32_t scrollIndex);

static bool ScenarioSelectUseSmallFont()
{
    return ThemeGetFlags() & UITHEME_FLAG_USE_ALTERNATIVE_SCENARIO_SELECT_FONT;
}

static WindowEventList window_scenarioselect_events([](auto& events)
{
    events.close = &WindowScenarioselectClose;
    events.mouse_up = &WindowScenarioselectMouseup;
    events.mouse_down = &WindowScenarioselectMousedown;
    events.get_scroll_size = &WindowScenarioselectScrollgetsize;
    events.scroll_mousedown = &WindowScenarioselectScrollmousedown;
    events.scroll_mouseover = &WindowScenarioselectScrollmouseover;
    events.invalidate = &WindowScenarioselectInvalidate;
    events.paint = &WindowScenarioselectPaint;
    events.scroll_paint = &WindowScenarioselectScrollpaint;
});
// clang-format on

static void DrawCategoryHeading(
    rct_window* w, rct_drawpixelinfo* dpi, int32_t left, int32_t right, int32_t y, StringId stringId);
static void InitialiseListItems(rct_window* w);
static bool IsScenarioVisible(rct_window* w, const scenario_index_entry* scenario);
static bool IsLockingEnabled(rct_window* w);

static std::function<void(std::string_view)> _callback;
static bool _showLockedInformation = false;
static bool _titleEditor = false;
static bool _disableLocking{};

rct_window* WindowScenarioselectOpen(scenarioselect_callback callback, bool titleEditor)
{
    if (_titleEditor != titleEditor)
    {
        _titleEditor = titleEditor;
        window_close_by_class(WindowClass::ScenarioSelect);
    }

    auto window = window_bring_to_front_by_class(WindowClass::ScenarioSelect);
    if (window != nullptr)
        return window;

    return WindowScenarioselectOpen(
        [callback](std::string_view scenario) { callback(std::string(scenario).c_str()); }, titleEditor, titleEditor);
}

/**
 *
 *  rct2: 0x006781B5
 */
rct_window* WindowScenarioselectOpen(std::function<void(std::string_view)> callback, bool titleEditor, bool disableLocking)
{
    rct_window* window;

    _callback = callback;
    _disableLocking = disableLocking;

    // Load scenario list
    scenario_repository_scan();

    window = WindowCreateCentred(
        WW, WH, &window_scenarioselect_events, WindowClass::ScenarioSelect, WF_10 | (titleEditor ? WF_STICK_TO_FRONT : 0));
    window->widgets = window_scenarioselect_widgets;

    WindowScenarioselectInitTabs(window);
    InitialiseListItems(window);

    WindowInitScrollWidgets(*window);
    window->highlighted_scenario = nullptr;

    return window;
}

/**
 *
 *  rct2: 0x00677C8A
 */
static void WindowScenarioselectInitTabs(rct_window* w)
{
    int32_t showPages = 0;
    size_t numScenarios = scenario_repository_get_count();
    for (size_t i = 0; i < numScenarios; i++)
    {
        const scenario_index_entry* scenario = scenario_repository_get_by_index(i);
        if (gConfigGeneral.ScenarioSelectMode == SCENARIO_SELECT_MODE_ORIGIN || _titleEditor)
        {
            if (_titleEditor && scenario->source_game == ScenarioSource::Other)
                continue;
            showPages |= 1 << static_cast<uint8_t>(scenario->source_game);
        }
        else
        {
            int32_t category = scenario->category;
            if (category > SCENARIO_CATEGORY_OTHER)
            {
                category = SCENARIO_CATEGORY_OTHER;
            }
            showPages |= 1 << category;
        }
    }

    if (showPages & (1 << gConfigInterface.ScenarioselectLastTab))
    {
        w->selected_tab = gConfigInterface.ScenarioselectLastTab;
    }
    else
    {
        int32_t firstPage = bitscanforward(showPages);
        if (firstPage != -1)
        {
            w->selected_tab = firstPage;
        }
    }

    int32_t y = TabsStart;
    for (int32_t i = 0; i < NumTabs; i++)
    {
        auto& widget = w->widgets[i + WIDX_TAB1];
        if (!(showPages & (1 << i)))
        {
            widget.type = WindowWidgetType::Empty;
            continue;
        }

        widget.type = WindowWidgetType::Tab;
        widget.top = y;
        widget.bottom = y + (TabHeight - 1);
        y += TabHeight;
    }
}

static void WindowScenarioselectClose(rct_window* w)
{
    _listItems.clear();
    _listItems.shrink_to_fit();
}

static void WindowScenarioselectMouseup(rct_window* w, WidgetIndex widgetIndex)
{
    if (widgetIndex == WIDX_CLOSE)
    {
        window_close(*w);
    }
}

static void WindowScenarioselectMousedown(rct_window* w, WidgetIndex widgetIndex, rct_widget* widget)
{
    if (widgetIndex >= WIDX_TAB1 && widgetIndex <= WIDX_TAB10)
    {
        w->selected_tab = widgetIndex - 4;
        w->highlighted_scenario = nullptr;
        gConfigInterface.ScenarioselectLastTab = w->selected_tab;
        ConfigSaveDefault();
        InitialiseListItems(w);
        w->Invalidate();
        window_event_resize_call(w);
        window_event_invalidate_call(w);
        WindowInitScrollWidgets(*w);
        w->Invalidate();
    }
}

static int32_t GetScenarioListItemSize()
{
    if (!LocalisationService_UseTrueTypeFont())
        return 24;

    // Scenario title
    int32_t lineHeight = font_get_line_height(FontStyle::Medium);

    // 'Completed by' line
    lineHeight += font_get_line_height(FontStyle::Small);

    return lineHeight;
}

static void WindowScenarioselectScrollgetsize(rct_window* w, int32_t scrollIndex, int32_t* width, int32_t* height)
{
    const int32_t scenarioItemHeight = GetScenarioListItemSize();

    int32_t y = 0;
    for (const auto& listItem : _listItems)
    {
        switch (listItem.type)
        {
            case ListItemType::Heading:
                y += 18;
                break;
            case ListItemType::Scenario:
                y += scenarioItemHeight;
                break;
        }
    }
    *height = y;
}

/**
 *
 *  rct2: 0x6780FE
 */
static void WindowScenarioselectScrollmousedown(rct_window* w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords)
{
    const int32_t scenarioItemHeight = GetScenarioListItemSize();

    auto mutableScreenCoords = screenCoords;
    for (const auto& listItem : _listItems)
    {
        switch (listItem.type)
        {
            case ListItemType::Heading:
                mutableScreenCoords.y -= 18;
                break;
            case ListItemType::Scenario:
                mutableScreenCoords.y -= scenarioItemHeight;
                if (mutableScreenCoords.y < 0 && !listItem.scenario.is_locked)
                {
                    OpenRCT2::Audio::Play(OpenRCT2::Audio::SoundId::Click1, 0, w->windowPos.x + (w->width / 2));
                    gFirstTimeSaving = true;
                    _callback(listItem.scenario.scenario->path);
                    if (_titleEditor)
                    {
                        window_close(*w);
                    }
                }
                break;
        }
        if (mutableScreenCoords.y < 0)
        {
            break;
        }
    }
}

/**
 *
 *  rct2: 0x678162
 */
static void WindowScenarioselectScrollmouseover(rct_window* w, int32_t scrollIndex, const ScreenCoordsXY& screenCoords)
{
    const int32_t scenarioItemHeight = GetScenarioListItemSize();

    bool originalShowLockedInformation = _showLockedInformation;
    _showLockedInformation = false;
    const scenario_index_entry* selected = nullptr;
    auto mutableScreenCoords = screenCoords;
    for (const auto& listItem : _listItems)
    {
        switch (listItem.type)
        {
            case ListItemType::Heading:
                mutableScreenCoords.y -= 18;
                break;
            case ListItemType::Scenario:
                mutableScreenCoords.y -= scenarioItemHeight;
                if (mutableScreenCoords.y < 0)
                {
                    if (listItem.scenario.is_locked)
                    {
                        _showLockedInformation = true;
                    }
                    else
                    {
                        selected = listItem.scenario.scenario;
                    }
                }
                break;
        }
        if (mutableScreenCoords.y < 0)
        {
            break;
        }
    }

    if (w->highlighted_scenario != selected)
    {
        w->highlighted_scenario = selected;
        w->Invalidate();
    }
    else if (_showLockedInformation != originalShowLockedInformation)
    {
        w->Invalidate();
    }
}

static void WindowScenarioselectInvalidate(rct_window* w)
{
    w->pressed_widgets &= ~(
        (1uLL << WIDX_CLOSE) | (1uLL << WIDX_TAB1) | (1uLL << WIDX_TAB2) | (1uLL << WIDX_TAB3) | (1uLL << WIDX_TAB4)
        | (1uLL << WIDX_TAB5) | (1uLL << WIDX_TAB6) | (1uLL << WIDX_TAB7) | (1uLL << WIDX_TAB8) | (1uLL << WIDX_TAB9)
        | (1uLL << WIDX_TAB10));

    w->pressed_widgets |= 1LL << (w->selected_tab + WIDX_TAB1);

    w->ResizeFrameWithPage();

    const int32_t bottomMargin = gConfigGeneral.DebuggingTools ? 17 : 5;
    window_scenarioselect_widgets[WIDX_SCENARIOLIST].right = w->width - 179;
    window_scenarioselect_widgets[WIDX_SCENARIOLIST].bottom = w->height - bottomMargin;
}

static void WindowScenarioselectPaint(rct_window* w, rct_drawpixelinfo* dpi)
{
    int32_t format;
    const scenario_index_entry* scenario;

    WindowDrawWidgets(*w, dpi);

    format = ScenarioSelectUseSmallFont() ? STR_SMALL_WINDOW_COLOUR_2_STRINGID : STR_WINDOW_COLOUR_2_STRINGID;
    FontStyle fontStyle = ScenarioSelectUseSmallFont() ? FontStyle::Small : FontStyle::Medium;

    // Text for each tab
    for (uint32_t i = 0; i < std::size(ScenarioOriginStringIds); i++)
    {
        rct_widget* widget = &window_scenarioselect_widgets[WIDX_TAB1 + i];
        if (widget->type == WindowWidgetType::Empty)
            continue;

        auto ft = Formatter();
        if (gConfigGeneral.ScenarioSelectMode == SCENARIO_SELECT_MODE_ORIGIN || _titleEditor)
        {
            ft.Add<StringId>(ScenarioOriginStringIds[i]);
        }
        else
        { // old-style
            ft.Add<StringId>(ScenarioCategoryStringIds[i]);
        }

        ScreenCoordsXY stringCoords(widget->midX() + w->windowPos.x, widget->midY() + w->windowPos.y - 3);
        DrawTextWrapped(dpi, stringCoords, 87, format, ft, { COLOUR_AQUAMARINE, fontStyle, TextAlignment::CENTRE });
    }

    // Return if no scenario highlighted
    scenario = w->highlighted_scenario;
    if (scenario == nullptr)
    {
        if (_showLockedInformation)
        {
            // Show locked information
            auto screenPos = w->windowPos
                + ScreenCoordsXY{ window_scenarioselect_widgets[WIDX_SCENARIOLIST].right + 4,
                                  window_scenarioselect_widgets[WIDX_TABCONTENT].top + 5 };
            DrawTextEllipsised(
                dpi, screenPos + ScreenCoordsXY{ 85, 0 }, 170, STR_SCENARIO_LOCKED, {}, { TextAlignment::CENTRE });
            DrawTextWrapped(dpi, screenPos + ScreenCoordsXY{ 0, 15 }, 170, STR_SCENARIO_LOCKED_DESC);
        }
        else
        {
            // Show general information about how to start.
            auto screenPos = w->windowPos
                + ScreenCoordsXY{ window_scenarioselect_widgets[WIDX_SCENARIOLIST].right + 4,
                                  window_scenarioselect_widgets[WIDX_TABCONTENT].top + 5 };

            DrawTextWrapped(dpi, screenPos + ScreenCoordsXY{ 0, 15 }, 170, STR_SCENARIO_HOVER_HINT);
        }
        return;
    }

    // Scenario path
    if (gConfigGeneral.DebuggingTools)
    {
        utf8 path[MAX_PATH];

        shorten_path(path, sizeof(path), scenario->path, w->width - 6 - TabWidth, FontStyle::Medium);

        const utf8* pathPtr = path;
        auto ft = Formatter();
        ft.Add<const char*>(pathPtr);
        DrawTextBasic(
            dpi, w->windowPos + ScreenCoordsXY{ TabWidth + 3, w->height - 3 - 11 }, STR_STRING, ft, { w->colours[1] });
    }

    // Scenario name
    auto screenPos = w->windowPos
        + ScreenCoordsXY{ window_scenarioselect_widgets[WIDX_SCENARIOLIST].right + 4,
                          window_scenarioselect_widgets[WIDX_TABCONTENT].top + 5 };
    auto ft = Formatter();
    ft.Add<StringId>(STR_STRING);
    ft.Add<const char*>(scenario->name);
    DrawTextEllipsised(
        dpi, screenPos + ScreenCoordsXY{ 85, 0 }, 170, STR_WINDOW_COLOUR_2_STRINGID, ft, { TextAlignment::CENTRE });
    screenPos.y += 15;

    // Scenario details
    ft = Formatter();
    ft.Add<StringId>(STR_STRING);
    ft.Add<const char*>(scenario->details);
    screenPos.y += DrawTextWrapped(dpi, screenPos, 170, STR_BLACK_STRING, ft) + 5;

    // Scenario objective
    ft = Formatter();
    ft.Add<StringId>(ObjectiveNames[scenario->objective_type]);
    if (scenario->objective_type == OBJECTIVE_BUILD_THE_BEST)
    {
        StringId rideTypeString = STR_NONE;
        auto rideTypeId = scenario->objective_arg_3;
        if (rideTypeId != RIDE_TYPE_NULL && rideTypeId < RIDE_TYPE_COUNT)
        {
            rideTypeString = GetRideTypeDescriptor(rideTypeId).Naming.Name;
        }
        ft.Add<StringId>(rideTypeString);
    }
    else
    {
        ft.Add<int16_t>(scenario->objective_arg_3);
        ft.Add<int16_t>(date_get_total_months(MONTH_OCTOBER, scenario->objective_arg_1));
        if (scenario->objective_type == OBJECTIVE_FINISH_5_ROLLERCOASTERS)
            ft.Add<uint16_t>(scenario->objective_arg_2);
        else
            ft.Add<money64>(scenario->objective_arg_2);
    }
    screenPos.y += DrawTextWrapped(dpi, screenPos, 170, STR_OBJECTIVE, ft) + 5;

    // Scenario score
    if (scenario->highscore != nullptr)
    {
        // TODO: Should probably be translatable
        const utf8* completedByName = "???";
        if (!str_is_null_or_empty(scenario->highscore->name))
        {
            completedByName = scenario->highscore->name;
        }
        ft = Formatter();
        ft.Add<StringId>(STR_STRING);
        ft.Add<const char*>(completedByName);
        ft.Add<money64>(scenario->highscore->company_value);
        screenPos.y += DrawTextWrapped(dpi, screenPos, 170, STR_COMPLETED_BY_WITH_COMPANY_VALUE, ft);
    }
}

static void WindowScenarioselectScrollpaint(rct_window* w, rct_drawpixelinfo* dpi, int32_t scrollIndex)
{
    uint8_t paletteIndex = ColourMapA[w->colours[1]].mid_light;
    gfx_clear(dpi, paletteIndex);

    StringId highlighted_format = ScenarioSelectUseSmallFont() ? STR_WHITE_STRING : STR_WINDOW_COLOUR_2_STRINGID;
    StringId unhighlighted_format = ScenarioSelectUseSmallFont() ? STR_WHITE_STRING : STR_BLACK_STRING;

    const auto& listWidget = w->widgets[WIDX_SCENARIOLIST];
    int32_t listWidth = listWidget.width() - 12;

    const int32_t scenarioItemHeight = GetScenarioListItemSize();

    // Scenario title
    int32_t scenarioTitleHeight = font_get_line_height(FontStyle::Medium);

    int32_t y = 0;
    for (const auto& listItem : _listItems)
    {
        if (y > dpi->y + dpi->height)
        {
            continue;
        }

        switch (listItem.type)
        {
            case ListItemType::Heading:
            {
                const int32_t horizontalRuleMargin = 4;
                DrawCategoryHeading(
                    w, dpi, horizontalRuleMargin, listWidth - horizontalRuleMargin, y + 2, listItem.heading.string_id);
                y += 18;
                break;
            }
            case ListItemType::Scenario:
            {
                // Draw hover highlight
                const scenario_index_entry* scenario = listItem.scenario.scenario;
                bool isHighlighted = w->highlighted_scenario == scenario;
                if (isHighlighted)
                {
                    gfx_filter_rect(dpi, { 0, y, w->width, y + scenarioItemHeight - 1 }, FilterPaletteID::PaletteDarken1);
                }

                bool isCompleted = scenario->highscore != nullptr;
                bool isDisabled = listItem.scenario.is_locked;

                // Draw scenario name
                char buffer[64];
                safe_strcpy(buffer, scenario->name, sizeof(buffer));
                StringId format = isDisabled ? static_cast<StringId>(STR_STRINGID)
                                             : (isHighlighted ? highlighted_format : unhighlighted_format);
                auto ft = Formatter();
                ft.Add<StringId>(STR_STRING);
                ft.Add<char*>(buffer);
                colour_t colour = isDisabled ? w->colours[1] | COLOUR_FLAG_INSET : COLOUR_BLACK;
                auto darkness = isDisabled ? TextDarkness::Dark : TextDarkness::Regular;
                const auto scrollCentre = window_scenarioselect_widgets[WIDX_SCENARIOLIST].width() / 2;

                DrawTextBasic(
                    dpi, { scrollCentre, y + 1 }, format, ft, { colour, FontStyle::Medium, TextAlignment::CENTRE, darkness });

                // Check if scenario is completed
                if (isCompleted)
                {
                    // Draw completion tick
                    gfx_draw_sprite(
                        dpi, ImageId(SPR_MENU_CHECKMARK),
                        { window_scenarioselect_widgets[WIDX_SCENARIOLIST].width() - 45, y + 1 });

                    // Draw completion score
                    const utf8* completedByName = "???";
                    if (!str_is_null_or_empty(scenario->highscore->name))
                    {
                        completedByName = scenario->highscore->name;
                    }
                    safe_strcpy(buffer, completedByName, 64);
                    ft = Formatter();
                    ft.Add<StringId>(STR_COMPLETED_BY);
                    ft.Add<StringId>(STR_STRING);
                    ft.Add<char*>(buffer);
                    DrawTextBasic(
                        dpi, { scrollCentre, y + scenarioTitleHeight + 1 }, format, ft,
                        { FontStyle::Small, TextAlignment::CENTRE });
                }

                y += scenarioItemHeight;
                break;
            }
        }
    }
}

static void DrawCategoryHeading(
    rct_window* w, rct_drawpixelinfo* dpi, int32_t left, int32_t right, int32_t y, StringId stringId)
{
    colour_t baseColour = w->colours[1];
    colour_t lightColour = ColourMapA[baseColour].lighter;
    colour_t darkColour = ColourMapA[baseColour].mid_dark;

    // Draw string
    int32_t centreX = (left + right) / 2;
    DrawTextBasic(dpi, { centreX, y }, stringId, {}, { baseColour, TextAlignment::CENTRE });

    // Get string dimensions
    utf8 buffer[CommonTextBufferSize];
    auto bufferPtr = buffer;
    format_string(bufferPtr, sizeof(buffer), stringId, nullptr);
    int32_t categoryStringHalfWidth = (gfx_get_string_width(bufferPtr, FontStyle::Medium) / 2) + 4;
    int32_t strLeft = centreX - categoryStringHalfWidth;
    int32_t strRight = centreX + categoryStringHalfWidth;

    // Draw light horizontal rule
    int32_t lineY = y + 4;
    auto lightLineLeftTop1 = ScreenCoordsXY{ left, lineY };
    auto lightLineRightBottom1 = ScreenCoordsXY{ strLeft, lineY };
    gfx_draw_line(dpi, { lightLineLeftTop1, lightLineRightBottom1 }, lightColour);

    auto lightLineLeftTop2 = ScreenCoordsXY{ strRight, lineY };
    auto lightLineRightBottom2 = ScreenCoordsXY{ right, lineY };
    gfx_draw_line(dpi, { lightLineLeftTop2, lightLineRightBottom2 }, lightColour);

    // Draw dark horizontal rule
    lineY++;
    auto darkLineLeftTop1 = ScreenCoordsXY{ left, lineY };
    auto darkLineRightBottom1 = ScreenCoordsXY{ strLeft, lineY };
    gfx_draw_line(dpi, { darkLineLeftTop1, darkLineRightBottom1 }, darkColour);

    auto darkLineLeftTop2 = ScreenCoordsXY{ strRight, lineY };
    auto darkLineRightBottom2 = ScreenCoordsXY{ right, lineY };
    gfx_draw_line(dpi, { darkLineLeftTop2, darkLineRightBottom2 }, darkColour);
}

static void InitialiseListItems(rct_window* w)
{
    size_t numScenarios = scenario_repository_get_count();
    _listItems.clear();

    // Mega park unlock
    const uint32_t rct1RequiredCompletedScenarios = (1 << SC_MEGA_PARK) - 1;
    uint32_t rct1CompletedScenarios = 0;
    size_t megaParkListItemIndex = SIZE_MAX;

    int32_t numUnlocks = INITIAL_NUM_UNLOCKED_SCENARIOS;
    uint8_t currentHeading = UINT8_MAX;
    for (size_t i = 0; i < numScenarios; i++)
    {
        const scenario_index_entry* scenario = scenario_repository_get_by_index(i);

        if (!IsScenarioVisible(w, scenario))
            continue;
        if (_titleEditor && scenario->source_game == ScenarioSource::Other)
            continue;

        // Category heading
        StringId headingStringId = STR_NONE;
        if (gConfigGeneral.ScenarioSelectMode == SCENARIO_SELECT_MODE_ORIGIN || _titleEditor)
        {
            if (w->selected_tab != static_cast<uint8_t>(ScenarioSource::Real) && currentHeading != scenario->category)
            {
                currentHeading = scenario->category;
                headingStringId = ScenarioCategoryStringIds[currentHeading];
            }
        }
        else
        {
            if (w->selected_tab <= SCENARIO_CATEGORY_EXPERT)
            {
                if (currentHeading != static_cast<uint8_t>(scenario->source_game))
                {
                    currentHeading = static_cast<uint8_t>(scenario->source_game);
                    headingStringId = ScenarioOriginStringIds[currentHeading];
                }
            }
            else if (w->selected_tab == SCENARIO_CATEGORY_OTHER)
            {
                int32_t category = scenario->category;
                if (category <= SCENARIO_CATEGORY_REAL)
                {
                    category = SCENARIO_CATEGORY_OTHER;
                }
                if (currentHeading != category)
                {
                    currentHeading = category;
                    headingStringId = ScenarioCategoryStringIds[category];
                }
            }
        }

        if (headingStringId != STR_NONE)
        {
            ScenarioListItem headerItem;
            headerItem.type = ListItemType::Heading;
            headerItem.heading.string_id = headingStringId;
            _listItems.push_back(std::move(headerItem));
        }

        // Scenario
        ScenarioListItem scenarioItem;
        scenarioItem.type = ListItemType::Scenario;
        scenarioItem.scenario.scenario = scenario;
        if (IsLockingEnabled(w))
        {
            scenarioItem.scenario.is_locked = numUnlocks <= 0;
            if (scenario->highscore == nullptr)
            {
                numUnlocks--;
            }
            else
            {
                // Mark RCT1 scenario as completed
                if (scenario->sc_id < SC_MEGA_PARK)
                {
                    rct1CompletedScenarios |= 1 << scenario->sc_id;
                }
            }

            // If scenario is Mega Park, keep a reference to it
            if (scenario->sc_id == SC_MEGA_PARK)
            {
                megaParkListItemIndex = _listItems.size() - 1;
            }
        }
        else
        {
            scenarioItem.scenario.is_locked = false;
        }
        _listItems.push_back(std::move(scenarioItem));
    }

    // Mega park handling
    if (megaParkListItemIndex != SIZE_MAX)
    {
        bool megaParkLocked = (rct1CompletedScenarios & rct1RequiredCompletedScenarios) != rct1RequiredCompletedScenarios;
        _listItems[megaParkListItemIndex].scenario.is_locked = megaParkLocked;
        if (megaParkLocked && gConfigGeneral.ScenarioHideMegaPark)
        {
            // Remove mega park
            _listItems.pop_back();

            // Remove empty headings
            for (auto it = _listItems.begin(); it != _listItems.end();)
            {
                const auto& listItem = *it;
                if (listItem.type == ListItemType::Heading)
                {
                    auto nextIt = std::next(it);
                    if (nextIt == _listItems.end() || nextIt->type == ListItemType::Heading)
                    {
                        it = _listItems.erase(it);
                        continue;
                    }
                }
                ++it;
            }
        }
    }
}

static bool IsScenarioVisible(rct_window* w, const scenario_index_entry* scenario)
{
    if (gConfigGeneral.ScenarioSelectMode == SCENARIO_SELECT_MODE_ORIGIN || _titleEditor)
    {
        if (static_cast<uint8_t>(scenario->source_game) != w->selected_tab)
        {
            return false;
        }
    }
    else
    {
        int32_t category = scenario->category;
        if (category > SCENARIO_CATEGORY_OTHER)
        {
            category = SCENARIO_CATEGORY_OTHER;
        }
        if (category != w->selected_tab)
        {
            return false;
        }
    }
    return true;
}

static bool IsLockingEnabled(rct_window* w)
{
    if (gConfigGeneral.ScenarioSelectMode != SCENARIO_SELECT_MODE_ORIGIN)
        return false;
    if (!gConfigGeneral.ScenarioUnlockingEnabled)
        return false;
    if (w->selected_tab >= 6)
        return false;
    if (_titleEditor)
        return false;

    return true;
}
