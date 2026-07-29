#include "menu_screen.h"
#include <Arduino.h>
#include <algorithm>
#include "../../config/constants.h"
#include "../../logging/grind_logging.h"
#include "../../system/statistics_manager.h"
#include "../../hardware/hardware_manager.h"
#include "grinding_screen.h"
#include "../event_bridge_lvgl.h"
#include "../../config/logging.h"
#include "../components/blocking_overlay.h"
#include "../fonts/custom_icons.h"

// Defined further below, alongside the other flat-toggle-row helpers; forward
// declared here so update_logging_toggle() and create_flat_toggle_row() can use them
// before their definitions later in the file.
static void set_toggle_state_caption(lv_obj_t* state_label, bool checked,
                                     lv_color_t accent_color = lv_color_hex(THEME_COLOR_MENU_SETTINGS));
static void toggle_state_caption_event_cb(lv_event_t* e);

static void back_event_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target_obj(e);
    lv_obj_t * menu = (lv_obj_t *)lv_event_get_user_data(e);

    if(lv_menu_back_button_is_root(menu, obj)) {
        // Call the EventBridge handle_event directly with MENU_BACK event
        EventBridgeLVGL::handle_event(EventBridgeLVGL::EventType::MENU_BACK, e);
    }
}

void MenuScreen::create(BluetoothManager* bluetooth, GrindController* grind_ctrl, GrindingScreen* grind_screen, class HardwareManager* hw_mgr, DiagnosticsController* diag_ctrl) {
    bluetooth_manager = bluetooth;
    grind_controller = grind_ctrl;
    grinding_screen = grind_screen;
    hardware_manager = hw_mgr;
    diagnostics_controller = diag_ctrl;

    screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
    lv_obj_align(screen, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    visible = false;
    scale_active = false;
    scale_page = nullptr;
    scale_weight_label = nullptr;
    scale_tare_button = nullptr;
    scale_item = nullptr;
    grinder_purge_mode_radio_group = nullptr;
    grinder_purge_amount_slider = nullptr;
    grinder_purge_amount_label = nullptr;
    grind_freshness_hours_slider = nullptr;
    grind_freshness_hours_label = nullptr;
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);

    // Create menu UI immediately at boot for instant access
    create_menu_ui();
}

void MenuScreen::create_menu_ui() {
    if (menu) {
        return; // Already created
    }

    LOG_BLE("[%lums MENU] Creating menu UI\n", millis());

    // Create menu instead of tabview
    menu = lv_menu_create(screen);
    lv_obj_set_size(menu, LV_PCT(100), LV_PCT(100));
    lv_obj_align(menu, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(menu, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(menu, 0, 0);
    lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
    lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);

    // Get header and set up flex layout
    lv_obj_t* header = lv_menu_get_main_header(menu);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_coord_t title_base_height = lv_font_get_line_height(&lv_font_montserrat_36);
    lv_coord_t title_target_height = (title_base_height * 3) / 2; // ~50% taller
    lv_coord_t title_padding = (title_target_height - title_base_height) / 2;
    lv_obj_set_style_min_height(header, title_target_height, 0);
    lv_obj_set_style_pad_top(header, title_padding, 0);
    lv_obj_set_style_pad_bottom(header, title_padding, 0);
    // Theme's default menu header style adds horizontal padding; zero it so
    // the back/close button's absolute-position offset below lines up with
    // the header's true left edge (matching the Ready screen's icon).
    lv_obj_set_style_pad_left(header, 0, 0);
    lv_obj_set_style_pad_right(header, 0, 0);

    // Get the header label - grown to fill the header so its centered text
    // is centered across the whole row, independent of the back/close button
    // (which is pulled out of layout flow below and no longer needs a
    // matching spacer to balance it).
    lv_obj_t* header_label = lv_obj_get_child(header, -1);
    if (header_label) {
        lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(header_label, &lv_font_montserrat_36, 0);
        lv_obj_set_style_text_color(header_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
        // Drop the title into its own row entirely below the top icon strip
        // (back/close button bottom edge is at y=46; the warning/Bluetooth
        // status icons in the top-right are shorter) so it can never collide
        // with any of them, regardless of title length. Text is top-anchored
        // within its content box, so pad_top directly controls where it
        // starts; ~4px of clearance below the lowest icon puts that at 50.
        // header's own pad_top (title_padding) already pushes header_label's
        // box down before this padding is even applied, so subtract it here
        // to land on an absolute 50px from the screen's top edge. Applies to
        // every page's title since they all share this one label.
        lv_coord_t title_pad_top = 50 - title_padding;
        lv_coord_t title_pad_bottom = title_padding;
        lv_obj_set_style_min_height(header_label, title_base_height + title_pad_top + title_pad_bottom, 0);
        lv_obj_set_style_pad_top(header_label, title_pad_top, 0);
        lv_obj_set_style_pad_bottom(header_label, title_pad_bottom, 0);
        lv_obj_set_flex_grow(header_label, 1);
    }

    // Back/close button - pulled out of the header's flex flow and pinned to
    // the exact same on-screen spot as the Ready screen's menu icon (44x44
    // circle, top-left at 8,2) so it reads as one button morphing in place:
    // "X" to close the menu from the root page, chevron to go back from a
    // sub-page. LV_ALIGN offsets land relative to the parent's padded
    // content box, so the vertical offset subtracts header's top padding to
    // land at an absolute 2px from the header's top edge.
    lv_obj_t* back_chevron = lv_menu_get_main_header_back_button(menu);
    lv_obj_add_flag(back_chevron, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(back_chevron, 44, 44);
    lv_obj_set_flex_align(back_chevron, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_radius(back_chevron, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(back_chevron, lv_color_hex(THEME_COLOR_BACKGROUND), 0);
    lv_obj_set_style_bg_opa(back_chevron, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(back_chevron, 0, 0);
    lv_obj_set_style_shadow_width(back_chevron, 0, 0);
    lv_obj_set_style_pad_all(back_chevron, 0, 0);
    lv_obj_set_style_text_font(back_chevron, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(back_chevron, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_align(back_chevron, LV_ALIGN_TOP_LEFT, 8, 2 - title_padding);
    // Corner buttons are where capacitive touch panels are least accurate and
    // where thumbs tend to overshoot; widen the invisible hit area well past
    // the visual icon so near-misses still register (same pattern used for
    // the toggle/slider controls elsewhere on this screen).
    lv_obj_set_ext_click_area(back_chevron, 20);

    // Root page starts on "Menu" (closes the menu), so start the icon as "X"
    lv_obj_t* back_icon = lv_obj_get_child(back_chevron, 0);
    if (back_icon) {
        lv_image_set_src(back_icon, LV_SYMBOL_CLOSE);
    }

    // Create main page last
    main_page = lv_menu_page_create(menu, "Menu");
    lv_obj_set_layout(main_page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(main_page, 0, 0);
    lv_obj_set_style_pad_gap(main_page, 0, 0);
    lv_obj_set_scroll_dir(main_page, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(main_page, LV_SCROLLBAR_MODE_AUTO);

    // Create sub-pages with titles
    info_page = lv_menu_page_create(menu, "Info");
    create_info_page(info_page);

    bluetooth_page = lv_menu_page_create(menu, "Bluetooth");
    create_bluetooth_page(bluetooth_page);

    display_page = lv_menu_page_create(menu, "Display");
    create_display_page(display_page);
    
    grind_mode_page = lv_menu_page_create(menu, "Grind Setup");
    create_grind_mode_page(grind_mode_page);

    profile_mode_page = lv_menu_page_create(menu, "Profile Mode");
    create_profile_mode_page(profile_mode_page);

    grind_type_page = lv_menu_page_create(menu, "Grind Mode");
    create_grind_type_page(grind_type_page);

    scale_page = lv_menu_page_create(menu, "Scale");
    create_scale_page(scale_page);

    motor_test_page = lv_menu_page_create(menu, "Motor Test");
    create_motor_test_page(motor_test_page);

    data_page = lv_menu_page_create(menu, "Logs");
    create_data_page(data_page);

    stats_page = lv_menu_page_create(menu, "Statistics");
    create_stats_page(stats_page);

    diagnostics_page = lv_menu_page_create(menu, "Monitor");
    create_diagnostics_page(diagnostics_page);

    // Create menu items grouped under section headers, each group with its own icon color.
    lv_color_t general_color = lv_color_hex(THEME_COLOR_MENU_GENERAL);
    lv_color_t calibration_color = lv_color_hex(THEME_COLOR_MENU_CALIBRATION);
    lv_color_t settings_color = lv_color_hex(THEME_COLOR_MENU_SETTINGS);
    lv_color_t system_color = lv_color_hex(THEME_COLOR_MENU_SYSTEM);

    create_section_header(main_page, "GENERAL");

    lv_obj_t* profile_mode_item = create_menu_item(main_page, "PROFILE MODE", ICON_PROFILE, general_color);
    lv_menu_set_load_page_event(menu, profile_mode_item, profile_mode_page);

    lv_obj_t* grind_type_item = create_menu_item(main_page, "GRIND MODE", ICON_GRIND_MODE, general_color);
    lv_menu_set_load_page_event(menu, grind_type_item, grind_type_page);

    create_section_header(main_page, "CALIBRATION");

    scale_item = create_menu_item(main_page, "SCALE", ICON_SCALE, calibration_color);
    cal_button = create_menu_item(main_page, "CALIBRATE", ICON_CALIBRATE, calibration_color);
    autotune_button = create_menu_item(main_page, "TUNE PULSES", ICON_TUNE_PULSE, calibration_color);
    motor_test_button = create_menu_item(main_page, "MOTOR TEST", ICON_MOTOR, calibration_color);

    lv_menu_set_load_page_event(menu, scale_item, scale_page);
    lv_menu_set_load_page_event(menu, motor_test_button, motor_test_page);

    lv_obj_add_flag(scale_item, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cal_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(autotune_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(motor_test_button, LV_OBJ_FLAG_CLICKABLE);

    using ET = EventBridgeLVGL::EventType;
    if (cal_button) {
        lv_obj_add_event_cb(cal_button, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::MENU_CALIBRATE)));
    }
    if (autotune_button) {
        lv_obj_add_event_cb(autotune_button, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::MENU_AUTOTUNE)));
    }

    create_section_header(main_page, "SETTINGS");

    lv_obj_t* bluetooth_item = create_menu_item(main_page, "BLUETOOTH", ICON_BLUETOOTH, settings_color);
    lv_menu_set_load_page_event(menu, bluetooth_item, bluetooth_page);

    lv_obj_t* display_item = create_menu_item(main_page, "DISPLAY", ICON_DISPLAY, settings_color);
    lv_menu_set_load_page_event(menu, display_item, display_page);

    lv_obj_t* grind_mode_item = create_menu_item(main_page, "GRIND SETTINGS", ICON_GRIND_SETTINGS, settings_color);
    lv_menu_set_load_page_event(menu, grind_mode_item, grind_mode_page);

    create_section_header(main_page, "SYSTEM");

    lv_obj_t* diagnostics_item = create_menu_item(main_page, "MONITOR", ICON_MONITOR, system_color);
    lv_menu_set_load_page_event(menu, diagnostics_item, diagnostics_page);

    lv_obj_t* info_item = create_menu_item(main_page, "SYSTEM INFO", ICON_INFO, system_color);
    lv_menu_set_load_page_event(menu, info_item, info_page);

    lv_obj_t* data_item = create_menu_item(main_page, "LOGS & DATA", ICON_LOGS, system_color);
    lv_menu_set_load_page_event(menu, data_item, data_page);

    lv_obj_t* stats_item = create_menu_item(main_page, "LIFETIME STATS", ICON_STATS, system_color);
    lv_menu_set_load_page_event(menu, stats_item, stats_page);

    // Set main page as active (menu will be the landing page)
    lv_menu_set_page(menu, main_page);

    // Refresh lifetime/log statistics whenever the Data or Stats page is displayed
    auto changing_page_callback = [](lv_event_t * e) {
        MenuScreen * self = static_cast<MenuScreen*>(lv_event_get_user_data(e));
        lv_obj_t * menu = static_cast<lv_obj_t *>(lv_event_get_target(e));
        lv_obj_t * cur = lv_menu_get_cur_main_page(menu);

        // Swap the shared back button's glyph: "X" closes the menu from the
        // root page, chevron goes back up one level from any sub-page.
        lv_obj_t * back_btn = lv_menu_get_main_header_back_button(menu);
        lv_obj_t * back_icon = lv_obj_get_child(back_btn, 0);
        if (back_icon) {
            lv_image_set_src(back_icon, cur == self->main_page ? LV_SYMBOL_CLOSE : LV_SYMBOL_LEFT);
        }

        if (cur == self->data_page || cur == self->stats_page) {
            self->refresh_statistics();
        }

        bool on_scale = (cur == self->scale_page);
        if (on_scale) {
            if (!self->scale_active) {
                self->scale_active = true;
                self->reset_scale_display();
                EventBridgeLVGL::handle_event(EventBridgeLVGL::EventType::MENU_SCALE_OPEN, e);
            }
        } else if (self->scale_active) {
            self->scale_active = false;
        }
    };

    lv_obj_add_event_cb(menu, changing_page_callback, LV_EVENT_VALUE_CHANGED, this);

    LOG_BLE("[%lums MENU] Menu UI created successfully\n", millis());
}

void MenuScreen::create_info_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 0, 0);

    // Enable vertical scrolling for the info page content
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Device identity and health at a glance.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    // Hero: Device Uptime, same slot/weight as the Data page's Sessions Stored and
    // the Stats page's Total Grinds - the number to check first after a reboot
    // or an unexpected freeze.
    lv_obj_t* hero = lv_obj_create(parent);
    lv_obj_set_size(hero, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hero, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hero, 0, 0);
    lv_obj_set_style_pad_all(hero, 2, 0);
    lv_obj_clear_flag(hero, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(hero, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hero, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(hero, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(hero, 2, 0);

    lv_obj_t* hero_label = lv_label_create(hero);
    lv_label_set_text(hero_label, "DEVICE UPTIME");
    lv_obj_set_style_text_font(hero_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hero_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_obj_set_style_text_letter_space(hero_label, 1, 0);

    uptime_label = lv_label_create(hero);
    lv_label_set_text(uptime_label, "00:00:00");
    lv_obj_set_style_text_font(uptime_label, &lv_font_montserrat_56, 0);
    lv_obj_set_style_text_color(uptime_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    // Memory: same capacity-bar shape as the Data page's Sessions Stored bar, but
    // the fill tracks heap *used* against the live heap size (not a fixed cap), and
    // recolors to warning once free memory runs low (see update_info()).
    lv_obj_t* memory = lv_obj_create(parent);
    lv_obj_set_size(memory, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(memory, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(memory, 0, 0);
    lv_obj_set_style_pad_hor(memory, 10, 0);
    lv_obj_set_style_pad_ver(memory, 4, 0);
    lv_obj_clear_flag(memory, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(memory, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(memory, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(memory, 6, 0);

    lv_obj_t* memory_labels = lv_obj_create(memory);
    lv_obj_set_size(memory_labels, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(memory_labels, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(memory_labels, 0, 0);
    lv_obj_set_style_pad_all(memory_labels, 0, 0);
    lv_obj_clear_flag(memory_labels, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(memory_labels, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(memory_labels, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(memory_labels, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    info_mem_used_label = lv_label_create(memory_labels);
    lv_label_set_text(info_mem_used_label, "Used");
    lv_obj_set_style_text_font(info_mem_used_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(info_mem_used_label, lv_color_hex(THEME_COLOR_MENU_SYSTEM), 0);

    lv_obj_t* memory_free_caption = lv_label_create(memory_labels);
    lv_label_set_text(memory_free_caption, "Free");
    lv_obj_set_style_text_font(memory_free_caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(memory_free_caption, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    lv_obj_t* memory_bar = lv_obj_create(memory);
    lv_obj_set_size(memory_bar, LV_PCT(100), 10);
    lv_obj_set_style_radius(memory_bar, 5, 0);
    lv_obj_set_style_bg_color(memory_bar, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_opa(memory_bar, LV_OPA_20, 0);
    lv_obj_set_style_border_width(memory_bar, 0, 0);
    lv_obj_set_style_pad_all(memory_bar, 0, 0);
    lv_obj_set_style_clip_corner(memory_bar, true, 0);
    lv_obj_clear_flag(memory_bar, LV_OBJ_FLAG_SCROLLABLE);

    info_mem_fill = lv_obj_create(memory_bar);
    lv_obj_set_size(info_mem_fill, 0, LV_PCT(100));
    lv_obj_set_style_radius(info_mem_fill, 0, 0);
    lv_obj_set_style_bg_color(info_mem_fill, lv_color_hex(THEME_COLOR_MENU_SYSTEM), 0);
    lv_obj_set_style_border_width(info_mem_fill, 0, 0);
    lv_obj_clear_flag(info_mem_fill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* memory_counts = lv_obj_create(memory);
    lv_obj_set_size(memory_counts, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(memory_counts, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(memory_counts, 0, 0);
    lv_obj_set_style_pad_all(memory_counts, 0, 0);
    lv_obj_clear_flag(memory_counts, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(memory_counts, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(memory_counts, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(memory_counts, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    info_mem_used_value_label = lv_label_create(memory_counts);
    lv_label_set_text(info_mem_used_value_label, "0 kB");
    lv_obj_set_style_text_font(info_mem_used_value_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(info_mem_used_value_label, lv_color_hex(THEME_COLOR_MENU_SYSTEM), 0);

    info_mem_free_value_label = lv_label_create(memory_counts);
    lv_label_set_text(info_mem_free_value_label, "0 kB");
    lv_obj_set_style_text_font(info_mem_free_value_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(info_mem_free_value_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    info_mem_caption_label = create_description_label(memory, "0% of 0 kB heap used.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    // Firmware
    create_separator(parent, nullptr, LV_OPA_30);
    lv_obj_t* firmware_heading = create_description_label(parent, "FIRMWARE",
                                                           &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));
    lv_obj_set_style_text_align(firmware_heading, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t* version_value = nullptr;
    create_flat_data_row(parent, "Version", &version_value, false, &lv_font_montserrat_20);
    lv_label_set_text(version_value, "v" BUILD_FIRMWARE_VERSION);

    char build_number_text[16];
    snprintf(build_number_text, sizeof(build_number_text), "#%d", BUILD_NUMBER);
    lv_obj_t* build_value = nullptr;
    create_flat_data_row(parent, "Build", &build_value, false, &lv_font_montserrat_20);
    lv_label_set_text(build_value, build_number_text);

    // Live reading
    create_separator(parent, nullptr, LV_OPA_30);
    lv_obj_t* live_heading = create_description_label(parent, "LIVE READING",
                                                       &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));
    lv_obj_set_style_text_align(live_heading, LV_TEXT_ALIGN_CENTER, 0);

    create_flat_data_row(parent, "Instant", &instant_label, false, &lv_font_montserrat_20);
    create_flat_data_row(parent, "Samples", &samples_label, false, &lv_font_montserrat_20);
    create_flat_data_row(parent, "Raw ADC", &raw_label, false, &lv_font_montserrat_20);
}


void MenuScreen::create_bluetooth_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Enable vertical scrolling on the menu page
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Configure Bluetooth connectivity and behavior.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    create_flat_toggle_row(parent, "ENABLE", &ble_toggle, false, &ble_state_label);

    // Advertising status, nested under Enable since it only applies while BLE is on.
    // Carries the hairline divider itself so the whole block sits above one line.
    ble_status_section = lv_obj_create(parent);
    lv_obj_set_size(ble_status_section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(ble_status_section, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(ble_status_section, 0, 0);
    lv_obj_set_style_border_width(ble_status_section, 1, 0);
    lv_obj_set_style_border_side(ble_status_section, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(ble_status_section, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_opa(ble_status_section, LV_OPA_30, 0);
    lv_obj_set_style_pad_hor(ble_status_section, 10, 0);
    lv_obj_set_style_pad_top(ble_status_section, 4, 0);
    lv_obj_set_style_pad_bottom(ble_status_section, 18, 0);
    lv_obj_set_style_pad_gap(ble_status_section, 2, 0);
    lv_obj_set_layout(ble_status_section, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(ble_status_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ble_status_section, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(ble_status_section, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* status_name_label = lv_label_create(ble_status_section);
    lv_label_set_text(status_name_label, "ADVERTISING STATUS");
    lv_obj_set_style_text_font(status_name_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(status_name_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    // BLE Status label - stacked under the name, not beside it, so it can't collide
    ble_status_label = lv_label_create(ble_status_section);
    lv_label_set_text(ble_status_label, "Advertising");
    lv_obj_set_style_text_font(ble_status_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(ble_status_label, lv_color_hex(THEME_COLOR_MENU_SETTINGS), 0);
    lv_obj_clear_flag(ble_status_label, LV_OBJ_FLAG_SCROLLABLE);

    // BLE Timer label
    ble_timer_label = lv_label_create(ble_status_section);
    lv_label_set_text(ble_timer_label, "");
    lv_obj_set_style_text_font(ble_timer_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(ble_timer_label, lv_color_hex(THEME_COLOR_WARNING), 0);
    lv_obj_clear_flag(ble_timer_label, LV_OBJ_FLAG_SCROLLABLE);

    create_flat_toggle_row(parent, "STARTUP", &ble_startup_toggle, true, &ble_startup_state_label);

    // Register events for the toggles (done here because widgets are created lazily)
    using ET = EventBridgeLVGL::EventType;
    if (ble_toggle) {
        lv_obj_add_event_cb(ble_toggle, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::BLE_TOGGLE)));
    }
    if (ble_startup_toggle) {
        lv_obj_add_event_cb(ble_startup_toggle, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::BLE_STARTUP_TOGGLE)));
    }
}

void MenuScreen::create_display_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // Enable vertical scrolling on the menu page
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Configure display brightness and behavior.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    create_display_slider_row(parent, "BRIGHTNESS", &brightness_normal_label, &brightness_normal_slider);
    create_display_slider_row(parent, "DIMMED", &brightness_screensaver_label, &brightness_screensaver_slider);
    // Auto-Dim: 18 raw positions (0-17) map to 5-90 seconds in 5-second steps.
    create_display_slider_row(parent, "AUTO-DIM", &auto_dim_timeout_label, &auto_dim_timeout_slider, 0, 17);

    // Register events for the sliders (done here because widgets are created lazily)
    using ET = EventBridgeLVGL::EventType;
    if (brightness_normal_slider) {
        lv_obj_add_event_cb(brightness_normal_slider, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::BRIGHTNESS_NORMAL_SLIDER)));
        lv_obj_add_event_cb(brightness_normal_slider, EventBridgeLVGL::dispatch_event, LV_EVENT_RELEASED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::BRIGHTNESS_NORMAL_SLIDER_RELEASED)));
    }
    if (brightness_screensaver_slider) {
        lv_obj_add_event_cb(brightness_screensaver_slider, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::BRIGHTNESS_SCREENSAVER_SLIDER)));
        lv_obj_add_event_cb(brightness_screensaver_slider, EventBridgeLVGL::dispatch_event, LV_EVENT_RELEASED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::BRIGHTNESS_SCREENSAVER_SLIDER_RELEASED)));
    }
    if (auto_dim_timeout_slider) {
        lv_obj_add_event_cb(auto_dim_timeout_slider, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::AUTO_DIM_TIMEOUT_SLIDER)));
        lv_obj_add_event_cb(auto_dim_timeout_slider, EventBridgeLVGL::dispatch_event, LV_EVENT_RELEASED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::AUTO_DIM_TIMEOUT_SLIDER_RELEASED)));
    }
}


// Callback for grinder purge mode radio button selection
static void grinder_purge_mode_callback(int selected_index, void* user_data) {
    // Trigger the event system instead of handling directly
    EventBridgeLVGL::handle_event(EventBridgeLVGL::EventType::GRINDER_PURGE_MODE_RADIO_BUTTON, nullptr);
}

void MenuScreen::create_grind_mode_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Enable vertical scrolling on the grind mode page
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Configure automatic grinding and purge behavior.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));
    create_separator(parent, nullptr, LV_OPA_30);

    // Automatic actions section
    create_description_label(parent, "AUTOMATION", &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));
    create_flat_toggle_desc_row(parent, "Auto Start", "Start grinding when a cup is detected.",
                               &auto_start_toggle, &auto_start_state_label);
    create_flat_toggle_desc_row(parent, "Auto Exit", "Close completion screen when the cup is removed.",
                               &auto_return_toggle, &auto_return_state_label);
    create_separator(parent, nullptr, LV_OPA_30);

    // Grinder Purging section
    create_description_label(parent, "PURGING", &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));
    create_description_label(parent, "Prime keeps the saturation grinds. Purge asks you to discard them.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    // Segmented control for grinder purge mode (Prime/Purge), inset 10px from each edge
    // so it doesn't run edge-to-edge like the full-width rows above and below it.
    lv_obj_t* segmented_wrapper = lv_obj_create(parent);
    lv_obj_set_size(segmented_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(segmented_wrapper, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(segmented_wrapper, 0, 0);
    lv_obj_set_style_pad_hor(segmented_wrapper, 10, 0);
    lv_obj_set_style_pad_ver(segmented_wrapper, 0, 0);
    lv_obj_clear_flag(segmented_wrapper, LV_OBJ_FLAG_SCROLLABLE);

    const char* grinder_purge_modes[] = {"PRIME", "PURGE"};
    grinder_purge_mode_radio_group = create_segmented_control(
        segmented_wrapper,
        grinder_purge_modes,
        2,
        1,  // Purge initially selected (index 1)
        lv_color_hex(THEME_COLOR_MENU_SETTINGS),
        grinder_purge_mode_callback,
        this
    );

    // Slider for grinder purge amount (uses kPurgeSliderScale for resolution)
    const uint32_t slider_min_units = static_cast<uint32_t>(GRIND_PURGE_AMOUNT_MIN_G * kPurgeSliderScale + 0.5f);
    const uint32_t slider_max_units = static_cast<uint32_t>(GRIND_PURGE_AMOUNT_MAX_G * kPurgeSliderScale + 0.5f);
    create_display_slider_row(parent, "Purge Amount", &grinder_purge_amount_label, &grinder_purge_amount_slider,
                             slider_min_units, slider_max_units, "Minimum target only.", &lv_font_montserrat_20);

    // Slider for grind freshness hours (discrete steps: 0.5, 1, 2, 3, 4, 8, 12, 24, 48)
    create_display_slider_row(parent, "Freshness", &grind_freshness_hours_label, &grind_freshness_hours_slider,
                             0, 8, "Time before a purge reminder.", &lv_font_montserrat_20);  // 9 positions (0-8)

    // Register events for the toggles (done here because widgets are created lazily)
    using ET = EventBridgeLVGL::EventType;
    if (auto_start_toggle) {
        lv_obj_add_event_cb(auto_start_toggle, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::AUTO_START_TOGGLE)));
    }
    if (auto_return_toggle) {
        lv_obj_add_event_cb(auto_return_toggle, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::AUTO_RETURN_TOGGLE)));
    }
    if (grinder_purge_amount_slider) {
        lv_obj_add_event_cb(grinder_purge_amount_slider, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::GRINDER_PURGE_AMOUNT_SLIDER)));
        lv_obj_add_event_cb(grinder_purge_amount_slider, EventBridgeLVGL::dispatch_event, LV_EVENT_RELEASED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::GRINDER_PURGE_AMOUNT_SLIDER_RELEASED)));
    }
    if (grind_freshness_hours_slider) {
        lv_obj_add_event_cb(grind_freshness_hours_slider, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::GRIND_FRESHNESS_HOURS_SLIDER)));
        lv_obj_add_event_cb(grind_freshness_hours_slider, EventBridgeLVGL::dispatch_event, LV_EVENT_RELEASED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::GRIND_FRESHNESS_HOURS_SLIDER_RELEASED)));
    }
}

void MenuScreen::create_profile_mode_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(parent, 12, 0);

    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Choose how presets are displayed.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));
    create_separator(parent, nullptr, LV_OPA_30);

    profile_style_drip_row = create_profile_style_row(parent, "DRIP COFFEE", "Cup-based presets",
                                                       "2, 4, 6, 8, 10 Cups", &profile_style_drip_dot);
    create_separator(parent, nullptr, LV_OPA_30);

    profile_style_espresso_row = create_profile_style_row(parent, "ESPRESSO", "Dose-based presets",
                                                           "Single, Double", &profile_style_espresso_dot);
    create_separator(parent, nullptr, LV_OPA_30);

    create_description_label(parent, "Changing this only affects the preset pages.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    using ET = EventBridgeLVGL::EventType;
    if (profile_style_drip_row) {
        lv_obj_add_event_cb(profile_style_drip_row, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::PROFILE_STYLE_SELECT_DRIP)));
    }
    if (profile_style_espresso_row) {
        lv_obj_add_event_cb(profile_style_espresso_row, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::PROFILE_STYLE_SELECT_ESPRESSO)));
    }
}

void MenuScreen::create_grind_type_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(parent, 12, 0);

    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Choose how grinding is controlled.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));
    create_separator(parent, nullptr, LV_OPA_30);

    grind_type_weight_row = create_profile_style_row(parent, "WEIGHT & TIME",
                                                      "Supports both weight and timed grinding.",
                                                      nullptr, &grind_type_weight_dot);

    // Swipe nests inside the Weight & Time row (same indent as its description,
    // hidden along with it) since the gesture only switches modes within this
    // style - it has no effect once Time Only is locked in.
    lv_obj_t* swipe_col = lv_obj_create(grind_type_weight_row);
    lv_obj_set_size(swipe_col, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(swipe_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(swipe_col, 0, 0);
    lv_obj_set_style_pad_all(swipe_col, 0, 0);
    lv_obj_set_style_pad_left(swipe_col, kModeRowIndent, 0);
    lv_obj_set_style_pad_top(swipe_col, 10, 0);
    lv_obj_clear_flag(swipe_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(swipe_col, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(swipe_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(swipe_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(swipe_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(swipe_col, 6, 0);

    lv_obj_t* swipe_label = lv_label_create(swipe_col);
    lv_label_set_text(swipe_label, "Swipe Gestures");
    lv_obj_set_style_text_font(swipe_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(swipe_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    // Switch sits under its header, sized down and recolored to match the
    // radio dots above instead of the default theme accent.
    grind_mode_swipe_toggle = lv_switch_create(swipe_col);
    lv_obj_set_size(grind_mode_swipe_toggle, 50, 26);
    lv_obj_set_ext_click_area(grind_mode_swipe_toggle, 20);
    lv_obj_set_style_bg_color(grind_mode_swipe_toggle, lv_color_hex(THEME_COLOR_MENU_GENERAL),
                             (lv_style_selector_t)LV_PART_INDICATOR | LV_STATE_CHECKED);

    grind_mode_swipe_state_label = lv_label_create(swipe_col);
    lv_obj_set_style_text_font(grind_mode_swipe_state_label, &lv_font_montserrat_20, 0);
    lv_obj_set_user_data(grind_mode_swipe_state_label,
                        reinterpret_cast<void*>(static_cast<uintptr_t>(THEME_COLOR_MENU_GENERAL)));
    set_toggle_state_caption(grind_mode_swipe_state_label, false, lv_color_hex(THEME_COLOR_MENU_GENERAL));
    lv_obj_add_event_cb(grind_mode_swipe_toggle, toggle_state_caption_event_cb, LV_EVENT_VALUE_CHANGED,
                       grind_mode_swipe_state_label);

    lv_obj_t* swipe_desc = lv_label_create(swipe_col);
    lv_label_set_text(swipe_desc, "Swipe up or down on the Ready screen to reach time mode.");
    lv_obj_set_style_text_font(swipe_desc, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(swipe_desc, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_label_set_long_mode(swipe_desc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(swipe_desc, LV_PCT(100));

    grind_mode_swipe_row = swipe_col;

    create_separator(parent, nullptr, LV_OPA_30);

    grind_type_time_only_row = create_profile_style_row(parent, "TIME ONLY", "Uses timed grinding only.",
                                                         "Weight calibration is disabled.", &grind_type_time_only_dot);

    using ET = EventBridgeLVGL::EventType;
    if (grind_type_weight_row) {
        lv_obj_add_event_cb(grind_type_weight_row, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::GRIND_MODE_SELECT_WEIGHT_TIME)));
    }
    if (grind_type_time_only_row) {
        lv_obj_add_event_cb(grind_type_time_only_row, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::GRIND_MODE_SELECT_TIME_ONLY)));
    }
    if (grind_mode_swipe_toggle) {
        lv_obj_add_event_cb(grind_mode_swipe_toggle, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::GRIND_MODE_SWIPE_TOGGLE)));
    }
}

void MenuScreen::create_scale_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 24, 0);
    lv_obj_set_style_pad_gap(parent, 28, 0);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, 0);

    lv_obj_t* subtitle = lv_label_create(parent);
    lv_label_set_text(subtitle, "Live weight");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);

    scale_weight_label = lv_label_create(parent);
    lv_label_set_text(scale_weight_label, "0.0g");
    lv_obj_set_style_text_font(scale_weight_label, &lv_font_montserrat_60, 0);
    lv_obj_set_style_text_color(scale_weight_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_align(scale_weight_label, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t* spacer = lv_obj_create(parent);
    lv_obj_set_size(spacer, LV_PCT(100), 0);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_flex_grow(spacer, 1);

    // Matches the Motor Test page's RUN button: 52px tall, montserrat_24, filled amber.
    scale_tare_button = create_button(parent, "TARE", lv_color_hex(THEME_COLOR_MENU_CALIBRATION), LV_PCT(100), 52, &lv_font_montserrat_24);
    lv_obj_set_style_text_color(scale_tare_button, lv_color_hex(THEME_COLOR_BACKGROUND), 0);
    using ET = EventBridgeLVGL::EventType;
    if (scale_tare_button) {
        lv_obj_add_event_cb(scale_tare_button, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::MENU_SCALE_TARE)));
    }
}

void MenuScreen::create_motor_test_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Initiate motor test.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));
    create_separator(parent, nullptr, LV_OPA_30);

    // Test Precautions section
    create_description_label(parent, "TEST PRECAUTIONS", &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));

    // Precaution row: warning icon beside wrapped description text, matching
    // the 280px content width used by the other description labels on this page.
    lv_obj_t* precaution_row = lv_obj_create(parent);
    lv_obj_set_size(precaution_row, 280, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(precaution_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(precaution_row, 0, 0);
    lv_obj_set_style_pad_hor(precaution_row, 10, 0);
    lv_obj_set_style_pad_ver(precaution_row, 0, 0);
    lv_obj_set_style_pad_gap(precaution_row, 12, 0);
    lv_obj_set_style_margin_bottom(precaution_row, 12, 0);
    lv_obj_clear_flag(precaution_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(precaution_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(precaution_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(precaution_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* precaution_icon = lv_label_create(precaution_row);
    lv_label_set_text(precaution_icon, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_font(precaution_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(precaution_icon, lv_color_hex(THEME_COLOR_WARNING), 0);

    lv_obj_t* precaution_text = lv_label_create(precaution_row);
    lv_label_set_text(precaution_text, "Ensure chamber is clear.");
    lv_obj_set_style_text_font(precaution_text, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(precaution_text, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_label_set_long_mode(precaution_text, LV_LABEL_LONG_WRAP);
    lv_obj_set_flex_grow(precaution_text, 1);

    create_description_label(parent, "Motor will run for 1s.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));
    create_separator(parent, nullptr, LV_OPA_30);

    // Spacer pushes the RUN button toward the bottom of the page, matching the Scale page.
    lv_obj_t* spacer = lv_obj_create(parent);
    lv_obj_set_size(spacer, LV_PCT(100), 0);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);
    lv_obj_set_flex_grow(spacer, 1);

    // Inset wrapper matches the Logs & Data / Monitor pages' action buttons,
    // clearing the page's scrollbar instead of colliding with it.
    lv_obj_t* run_wrapper = lv_obj_create(parent);
    lv_obj_set_size(run_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(run_wrapper, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(run_wrapper, 0, 0);
    lv_obj_set_style_pad_hor(run_wrapper, 20, 0);
    lv_obj_set_style_pad_ver(run_wrapper, 0, 0);
    lv_obj_set_style_margin_bottom(run_wrapper, 18, 0);
    lv_obj_clear_flag(run_wrapper, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(run_wrapper, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(run_wrapper, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(run_wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    motor_test_run_button = create_button(run_wrapper, "RUN", lv_color_hex(THEME_COLOR_MENU_CALIBRATION), LV_PCT(100), 52, &lv_font_montserrat_24);
    lv_obj_set_style_text_color(motor_test_run_button, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    using ET = EventBridgeLVGL::EventType;
    if (motor_test_run_button) {
        lv_obj_add_event_cb(motor_test_run_button, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::MENU_MOTOR_TEST)));
    }
}

void MenuScreen::create_data_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Enable vertical scrolling on the reset page
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Manage stored grind logs.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    // Hero: Sessions Stored, same slot as the Stats page's Total Grinds hero, but the
    // label stays white (not neutral-gray) since this number is tied to a hard capacity
    // limit rather than an open-ended lifetime count.
    lv_obj_t* hero = lv_obj_create(parent);
    lv_obj_set_size(hero, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hero, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hero, 0, 0);
    lv_obj_set_style_pad_all(hero, 2, 0);
    lv_obj_clear_flag(hero, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(hero, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hero, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(hero, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(hero, 2, 0);

    lv_obj_t* hero_label = lv_label_create(hero);
    lv_label_set_text(hero_label, "SESSIONS STORED");
    lv_obj_set_style_text_font(hero_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hero_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_letter_space(hero_label, 1, 0);

    data_hero_sessions_label = lv_label_create(hero);
    lv_label_set_text(data_hero_sessions_label, "0");
    lv_obj_set_style_text_font(data_hero_sessions_label, &lv_font_montserrat_56, 0);
    lv_obj_set_style_text_color(data_hero_sessions_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    char capacity_caption[24];
    snprintf(capacity_caption, sizeof(capacity_caption), "of %d max", MAX_STORED_SESSIONS_FLASH);
    lv_obj_t* hero_of_label = lv_label_create(hero);
    lv_label_set_text(hero_of_label, capacity_caption);
    lv_obj_set_style_text_font(hero_of_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hero_of_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    // Capacity bar: one meaningful value (Stored) against a fixed capacity, so unlike the
    // Stats page's weight/time split bar this only needs a single fill segment sized by
    // percentage, not two flex_grow-weighted children.
    lv_obj_t* capacity = lv_obj_create(parent);
    lv_obj_set_size(capacity, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(capacity, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(capacity, 0, 0);
    lv_obj_set_style_pad_hor(capacity, 10, 0);
    lv_obj_set_style_pad_ver(capacity, 4, 0);
    lv_obj_clear_flag(capacity, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(capacity, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(capacity, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(capacity, 6, 0);

    lv_obj_t* capacity_labels = lv_obj_create(capacity);
    lv_obj_set_size(capacity_labels, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(capacity_labels, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(capacity_labels, 0, 0);
    lv_obj_set_style_pad_all(capacity_labels, 0, 0);
    lv_obj_clear_flag(capacity_labels, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(capacity_labels, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(capacity_labels, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(capacity_labels, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* capacity_stored_caption = lv_label_create(capacity_labels);
    lv_label_set_text(capacity_stored_caption, "Stored");
    lv_obj_set_style_text_font(capacity_stored_caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(capacity_stored_caption, lv_color_hex(THEME_COLOR_MENU_SYSTEM), 0);

    lv_obj_t* capacity_free_caption = lv_label_create(capacity_labels);
    lv_label_set_text(capacity_free_caption, "Free");
    lv_obj_set_style_text_font(capacity_free_caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(capacity_free_caption, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    lv_obj_t* capacity_bar = lv_obj_create(capacity);
    lv_obj_set_size(capacity_bar, LV_PCT(100), 10);
    lv_obj_set_style_radius(capacity_bar, 5, 0);
    lv_obj_set_style_bg_color(capacity_bar, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_opa(capacity_bar, LV_OPA_20, 0);
    lv_obj_set_style_border_width(capacity_bar, 0, 0);
    lv_obj_set_style_pad_all(capacity_bar, 0, 0);
    lv_obj_set_style_clip_corner(capacity_bar, true, 0);
    lv_obj_clear_flag(capacity_bar, LV_OBJ_FLAG_SCROLLABLE);

    data_capacity_fill = lv_obj_create(capacity_bar);
    lv_obj_set_size(data_capacity_fill, 0, LV_PCT(100));
    lv_obj_set_style_radius(data_capacity_fill, 0, 0);
    lv_obj_set_style_bg_color(data_capacity_fill, lv_color_hex(THEME_COLOR_MENU_SYSTEM), 0);
    lv_obj_set_style_border_width(data_capacity_fill, 0, 0);
    lv_obj_clear_flag(data_capacity_fill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* capacity_counts = lv_obj_create(capacity);
    lv_obj_set_size(capacity_counts, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(capacity_counts, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(capacity_counts, 0, 0);
    lv_obj_set_style_pad_all(capacity_counts, 0, 0);
    lv_obj_clear_flag(capacity_counts, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(capacity_counts, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(capacity_counts, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(capacity_counts, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    data_capacity_stored_label = lv_label_create(capacity_counts);
    lv_label_set_text(data_capacity_stored_label, "0");
    lv_obj_set_style_text_font(data_capacity_stored_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(data_capacity_stored_label, lv_color_hex(THEME_COLOR_MENU_SYSTEM), 0);

    data_capacity_free_label = lv_label_create(capacity_counts);
    lv_label_set_text(data_capacity_free_label, "0");
    lv_obj_set_style_text_font(data_capacity_free_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(data_capacity_free_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    create_description_label(capacity, "Oldest session rotates out once storage is full.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    // Contents - Events and Measurements as a two-column comparison, same divided-column
    // shape as the Stats page's Performance row (Accuracy | Total Pulses).
    create_separator(parent, nullptr, LV_OPA_30);
    lv_obj_t* contents_heading = create_description_label(parent, "CONTENTS",
                                                           &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));
    lv_obj_set_style_text_align(contents_heading, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t* contents_row = lv_obj_create(parent);
    lv_obj_set_size(contents_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(contents_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(contents_row, 0, 0);
    lv_obj_set_style_pad_hor(contents_row, 10, 0);
    lv_obj_clear_flag(contents_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(contents_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(contents_row, LV_FLEX_FLOW_ROW);

    lv_obj_t* events_col = lv_obj_create(contents_row);
    lv_obj_set_size(events_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(events_col, 1);
    lv_obj_set_style_bg_opa(events_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(events_col, 0, 0);
    lv_obj_set_style_pad_all(events_col, 6, 0);
    lv_obj_clear_flag(events_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(events_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(events_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(events_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* events_caption = lv_label_create(events_col);
    lv_label_set_text(events_caption, "EVENTS");
    lv_obj_set_style_text_font(events_caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(events_caption, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_label_set_long_mode(events_caption, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(events_caption, LV_PCT(100));
    lv_obj_set_style_text_align(events_caption, LV_TEXT_ALIGN_CENTER, 0);

    events_label = lv_label_create(events_col);
    lv_label_set_text(events_label, "0");
    lv_obj_set_style_text_font(events_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(events_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    lv_obj_t* measurements_col = lv_obj_create(contents_row);
    lv_obj_set_size(measurements_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(measurements_col, 1);
    lv_obj_set_style_bg_opa(measurements_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(measurements_col, 1, 0);
    lv_obj_set_style_border_side(measurements_col, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(measurements_col, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_opa(measurements_col, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(measurements_col, 6, 0);
    lv_obj_clear_flag(measurements_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(measurements_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(measurements_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(measurements_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* measurements_caption = lv_label_create(measurements_col);
    lv_label_set_text(measurements_caption, "SAMPLES");
    lv_obj_set_style_text_font(measurements_caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(measurements_caption, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_label_set_long_mode(measurements_caption, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(measurements_caption, LV_PCT(100));
    lv_obj_set_style_text_align(measurements_caption, LV_TEXT_ALIGN_CENTER, 0);

    measurements_label = lv_label_create(measurements_col);
    lv_label_set_text(measurements_label, "0");
    lv_obj_set_style_text_font(measurements_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(measurements_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    data_measurements_avg_label = lv_label_create(measurements_col);
    lv_label_set_text(data_measurements_avg_label, "avg\n0/session");
    lv_obj_set_style_text_font(data_measurements_avg_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(data_measurements_avg_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_align(data_measurements_avg_label, LV_TEXT_ALIGN_CENTER, 0);

    // Logging toggle
    create_separator(parent, nullptr, LV_OPA_30);
    create_flat_toggle_desc_row(parent, "LOGGING", "Record grind sessions.",
                               &logging_toggle, &logging_state_label,
                               lv_color_hex(THEME_COLOR_MENU_SYSTEM), &lv_font_montserrat_24);

    // Maintenance section
    create_separator(parent, nullptr, LV_OPA_30);
    create_description_label(parent, "MAINTENANCE", &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));

    create_description_label(parent, "Purge stored logs.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));
    // Inset wrapper widens the button's clearance from the page's scrollbar
    // (matches the 20px used for the Stored Data values above) - the button
    // itself stays full width of this narrowed wrapper, same size as before.
    lv_obj_t* purge_wrapper = lv_obj_create(parent);
    lv_obj_set_size(purge_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(purge_wrapper, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(purge_wrapper, 0, 0);
    lv_obj_set_style_pad_hor(purge_wrapper, 20, 0);
    lv_obj_set_style_pad_ver(purge_wrapper, 0, 0);
    lv_obj_clear_flag(purge_wrapper, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(purge_wrapper, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(purge_wrapper, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(purge_wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // Filled button (color bg + black text/icon), matching Scale's Tare and Motor Test's Run
    // buttons - outline was the odd treatment out on this page.
    purge_button = create_button(purge_wrapper, LV_SYMBOL_TRASH "  Purge Logs", lv_color_hex(THEME_COLOR_WARNING),
                                 LV_PCT(100), 52, &lv_font_montserrat_24);
    lv_obj_set_style_text_color(purge_button, lv_color_hex(THEME_COLOR_BACKGROUND), 0);
    lv_obj_set_style_margin_bottom(purge_wrapper, 18, 0);

    create_description_label(parent, "Restore factory settings.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));
    lv_obj_t* reset_wrapper = lv_obj_create(parent);
    lv_obj_set_size(reset_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(reset_wrapper, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(reset_wrapper, 0, 0);
    lv_obj_set_style_pad_hor(reset_wrapper, 20, 0);
    lv_obj_set_style_pad_ver(reset_wrapper, 0, 0);
    lv_obj_clear_flag(reset_wrapper, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(reset_wrapper, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(reset_wrapper, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(reset_wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    reset_button = create_button(reset_wrapper, LV_SYMBOL_WARNING "  Factory Reset", lv_color_hex(THEME_COLOR_ERROR),
                                 LV_PCT(100), 52, &lv_font_montserrat_24);
    lv_obj_set_style_text_color(reset_button, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    // Register events for the toggle and buttons (done here because widgets are created lazily)
    using ET = EventBridgeLVGL::EventType;
    if (logging_toggle) {
        lv_obj_add_event_cb(logging_toggle, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::LOGGING_TOGGLE)));
    }
    if (purge_button) {
        lv_obj_add_event_cb(purge_button, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::MENU_PURGE)));
    }
    if (reset_button) {
        lv_obj_add_event_cb(reset_button, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::MENU_RESET)));
    }
}

void MenuScreen::create_stats_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Lifetime totals for the grinder.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    // Hero: Total Grinds is the headline stat, promoted to the same font tier as
    // the Ready screen's weight display so it reads before anything else on the page.
    lv_obj_t* hero = lv_obj_create(parent);
    lv_obj_set_size(hero, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hero, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hero, 0, 0);
    lv_obj_set_style_pad_all(hero, 2, 0);
    lv_obj_clear_flag(hero, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(hero, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hero, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(hero, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(hero, 2, 0);

    lv_obj_t* hero_label = lv_label_create(hero);
    lv_label_set_text(hero_label, "TOTAL GRINDS");
    lv_obj_set_style_text_font(hero_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hero_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_obj_set_style_text_letter_space(hero_label, 1, 0);

    stat_total_grinds_label = lv_label_create(hero);
    lv_label_set_text(stat_total_grinds_label, "0");
    lv_obj_set_style_text_font(stat_total_grinds_label, &lv_font_montserrat_56, 0);
    lv_obj_set_style_text_color(stat_total_grinds_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    // Weight vs. Time split - proportion bar in the same orange/blue already used
    // for grind mode elsewhere (arc + Ready screen), instead of a "44 / 3" string.
    lv_obj_t* split = lv_obj_create(parent);
    lv_obj_set_size(split, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(split, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(split, 0, 0);
    lv_obj_set_style_pad_hor(split, 10, 0);
    lv_obj_set_style_pad_ver(split, 4, 0);
    lv_obj_clear_flag(split, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(split, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(split, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(split, 6, 0);

    lv_obj_t* split_labels = lv_obj_create(split);
    lv_obj_set_size(split_labels, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(split_labels, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(split_labels, 0, 0);
    lv_obj_set_style_pad_all(split_labels, 0, 0);
    lv_obj_clear_flag(split_labels, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(split_labels, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(split_labels, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(split_labels, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* split_weight_label = lv_label_create(split_labels);
    lv_label_set_text(split_weight_label, "Weight");
    lv_obj_set_style_text_font(split_weight_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(split_weight_label, lv_color_hex(THEME_COLOR_ARC_WEIGHT), 0);

    lv_obj_t* split_time_label = lv_label_create(split_labels);
    lv_label_set_text(split_time_label, "Time");
    lv_obj_set_style_text_font(split_time_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(split_time_label, lv_color_hex(THEME_COLOR_ARC_TIME), 0);

    lv_obj_t* split_bar = lv_obj_create(split);
    lv_obj_set_size(split_bar, LV_PCT(100), 10);
    lv_obj_set_style_radius(split_bar, 5, 0);
    lv_obj_set_style_bg_color(split_bar, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_opa(split_bar, LV_OPA_20, 0);
    lv_obj_set_style_border_width(split_bar, 0, 0);
    lv_obj_set_style_pad_all(split_bar, 0, 0);
    lv_obj_set_style_clip_corner(split_bar, true, 0);
    lv_obj_clear_flag(split_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(split_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(split_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(split_bar, 0, 0);

    stat_mode_weight_fill = lv_obj_create(split_bar);
    lv_obj_set_size(stat_mode_weight_fill, 0, LV_PCT(100));
    lv_obj_set_style_radius(stat_mode_weight_fill, 0, 0);
    lv_obj_set_style_bg_color(stat_mode_weight_fill, lv_color_hex(THEME_COLOR_ARC_WEIGHT), 0);
    lv_obj_set_style_border_width(stat_mode_weight_fill, 0, 0);
    lv_obj_clear_flag(stat_mode_weight_fill, LV_OBJ_FLAG_SCROLLABLE);

    stat_mode_time_fill = lv_obj_create(split_bar);
    lv_obj_set_size(stat_mode_time_fill, 0, LV_PCT(100));
    lv_obj_set_style_radius(stat_mode_time_fill, 0, 0);
    lv_obj_set_style_bg_color(stat_mode_time_fill, lv_color_hex(THEME_COLOR_ARC_TIME), 0);
    lv_obj_set_style_border_width(stat_mode_time_fill, 0, 0);
    lv_obj_clear_flag(stat_mode_time_fill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* split_counts = lv_obj_create(split);
    lv_obj_set_size(split_counts, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(split_counts, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(split_counts, 0, 0);
    lv_obj_set_style_pad_all(split_counts, 0, 0);
    lv_obj_clear_flag(split_counts, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(split_counts, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(split_counts, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(split_counts, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    stat_mode_weight_count_label = lv_label_create(split_counts);
    lv_label_set_text(stat_mode_weight_count_label, "0");
    lv_obj_set_style_text_font(stat_mode_weight_count_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(stat_mode_weight_count_label, lv_color_hex(THEME_COLOR_ARC_WEIGHT), 0);

    stat_mode_time_count_label = lv_label_create(split_counts);
    lv_label_set_text(stat_mode_time_count_label, "0");
    lv_obj_set_style_text_font(stat_mode_time_count_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(stat_mode_time_count_label, lv_color_hex(THEME_COLOR_ARC_TIME), 0);

    // Grinds by profile - Drip cup sizes + Espresso breakdown, replacing the old
    // unlabeled "12/34/5/2/1/8" / "9/6/1" slash-separated strings.
    create_separator(parent, nullptr, LV_OPA_30);
    lv_obj_t* profile_heading = create_description_label(parent, "GRINDS BY PROFILE",
                                                          &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));
    lv_obj_set_style_text_align(profile_heading, LV_TEXT_ALIGN_CENTER, 0);

    static const char* const kDripPairLabels[6] = {"2C", "4C", "6C", "8C", "10C", "Cus"};
    create_stat_pair_row(parent, "Drip", stat_drip_labels, kDripPairLabels, 6);

    static const char* const kEspressoPairLabels[3] = {"S", "D", "Cus"};
    create_stat_pair_row(parent, "Espresso", stat_espresso_labels, kEspressoPairLabels, 3);

    // Performance - Avg Accuracy (colors itself against the grind tolerance) and
    // Total Pulses, as two centered columns divided by a hairline.
    create_separator(parent, nullptr, LV_OPA_30);
    lv_obj_t* perf_heading = create_description_label(parent, "PERFORMANCE",
                                                       &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));
    lv_obj_set_style_text_align(perf_heading, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t* perf_row = lv_obj_create(parent);
    lv_obj_set_size(perf_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(perf_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(perf_row, 0, 0);
    lv_obj_set_style_pad_hor(perf_row, 10, 0);
    lv_obj_clear_flag(perf_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(perf_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(perf_row, LV_FLEX_FLOW_ROW);

    lv_obj_t* accuracy_col = lv_obj_create(perf_row);
    lv_obj_set_size(accuracy_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(accuracy_col, 1);
    lv_obj_set_style_bg_opa(accuracy_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(accuracy_col, 0, 0);
    lv_obj_set_style_pad_all(accuracy_col, 6, 0);
    lv_obj_clear_flag(accuracy_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(accuracy_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(accuracy_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(accuracy_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* accuracy_caption = lv_label_create(accuracy_col);
    lv_label_set_text(accuracy_caption, "ACC");
    lv_obj_set_style_text_font(accuracy_caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(accuracy_caption, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_label_set_long_mode(accuracy_caption, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(accuracy_caption, LV_PCT(100));
    lv_obj_set_style_text_align(accuracy_caption, LV_TEXT_ALIGN_CENTER, 0);

    stat_avg_accuracy_label = lv_label_create(accuracy_col);
    lv_label_set_text(stat_avg_accuracy_label, "0.00g");
    lv_obj_set_style_text_font(stat_avg_accuracy_label, &lv_font_montserrat_20, 0);

    lv_obj_t* pulses_col = lv_obj_create(perf_row);
    lv_obj_set_size(pulses_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(pulses_col, 1);
    lv_obj_set_style_bg_opa(pulses_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pulses_col, 1, 0);
    lv_obj_set_style_border_side(pulses_col, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_color(pulses_col, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_opa(pulses_col, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(pulses_col, 6, 0);
    lv_obj_clear_flag(pulses_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(pulses_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pulses_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(pulses_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* pulses_caption = lv_label_create(pulses_col);
    lv_label_set_text(pulses_caption, "TOTAL PULSES");
    lv_obj_set_style_text_font(pulses_caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(pulses_caption, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_label_set_long_mode(pulses_caption, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(pulses_caption, LV_PCT(100));
    lv_obj_set_style_text_align(pulses_caption, LV_TEXT_ALIGN_CENTER, 0);

    stat_total_pulses_label = lv_label_create(pulses_col);
    lv_label_set_text(stat_total_pulses_label, "0");
    lv_obj_set_style_text_font(stat_total_pulses_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(stat_total_pulses_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    stat_avg_pulses_label = lv_label_create(pulses_col);
    lv_label_set_text(stat_avg_pulses_label, "avg 0.0");
    lv_obj_set_style_text_font(stat_avg_pulses_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(stat_avg_pulses_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);

    // Usage - flat rows, same shape as the Diagnostics/Logs pages (divider only
    // trails the last row of the group, not between every row).
    create_separator(parent, nullptr, LV_OPA_30);
    create_description_label(parent, "USAGE", &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));
    create_flat_data_row(parent, "Motor Runtime", &stat_motor_runtime_label, false, &lv_font_montserrat_20);
    create_flat_data_row(parent, "Device Uptime", &stat_device_uptime_label, false, &lv_font_montserrat_20);
    create_flat_data_row(parent, "Total Weight", &stat_total_weight_label, true, &lv_font_montserrat_20);

    refresh_stats_button = create_button(parent, "Refresh Stats", lv_color_hex(THEME_COLOR_ACCENT),
                                         LV_PCT(100), 52, &lv_font_montserrat_24);
    lv_obj_set_style_margin_top(refresh_stats_button, 10, 0);
    // Black label text for contrast against the accent-blue fill (create_button always
    // sets THEME_COLOR_TEXT_PRIMARY, so override the label it created directly).
    lv_obj_t* refresh_label = lv_obj_get_child(refresh_stats_button, 0);
    lv_obj_set_style_text_color(refresh_label, lv_color_hex(0x000000), 0);

    // Register event for the button (done here because widgets are created lazily)
    using ET = EventBridgeLVGL::EventType;
    if (refresh_stats_button) {
        lv_obj_add_event_cb(refresh_stats_button, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::MENU_REFRESH_STATS)));
    }
}

void MenuScreen::create_diagnostics_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Enable vertical scrolling
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);

    create_description_label(parent, "Load cell health and grinder response diagnostics.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    // Hero: System Status, same slot/weight as the other System-group pages' heroes (Device
    // Uptime, Sessions Stored, Total Grinds). Label stays white (not neutral-gray) since this
    // is a pass/fail health check rather than an open-ended count, same reasoning as the Data
    // page's Sessions Stored label. Caption below mirrors the Info page's memory-bar caption -
    // a positive default message, swapped for the active diagnostic's message on warning.
    lv_obj_t* hero = lv_obj_create(parent);
    lv_obj_set_size(hero, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hero, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hero, 0, 0);
    lv_obj_set_style_pad_all(hero, 2, 0);
    lv_obj_clear_flag(hero, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(hero, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hero, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(hero, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(hero, 2, 0);

    lv_obj_t* hero_label = lv_label_create(hero);
    lv_label_set_text(hero_label, "SYSTEM STATUS");
    lv_obj_set_style_text_font(hero_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hero_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_letter_space(hero_label, 1, 0);

    diag_status_label = lv_label_create(hero);
    lv_label_set_text(diag_status_label, "OK");
    lv_obj_set_style_text_font(diag_status_label, &lv_font_montserrat_56, 0);
    lv_obj_set_style_text_color(diag_status_label, lv_color_hex(THEME_COLOR_SUCCESS), 0);
    // Center so the warning icon and "Warning" line up when they wrap to two lines
    // (icon on its own line, text below) - see update_diagnostics().
    lv_obj_set_style_text_align(diag_status_label, LV_TEXT_ALIGN_CENTER, 0);

    diag_info_label = lv_label_create(hero);
    lv_label_set_text(diag_info_label, "All diagnostics nominal.");
    lv_obj_set_style_text_font(diag_info_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(diag_info_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_obj_set_style_text_align(diag_info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_top(diag_info_label, 4, 0);
    lv_label_set_long_mode(diag_info_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(diag_info_label, 240);

    create_separator(parent, nullptr, LV_OPA_30);

    // Load Cell section
    create_description_label(parent, "LOAD CELL", &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));
    create_flat_data_row(parent, "Cal. factor", &diag_calibration_factor_label, true);

    // Inset wrapper matches the Logs & Data page's Purge/Reset action buttons. Solid fill with
    // black text, matching every other action button in the app (Purge Logs, Factory Reset,
    // Refresh Stats, Tare, Run) - outline was the odd treatment out on this page.
    lv_obj_t* reset_wrapper = lv_obj_create(parent);
    lv_obj_set_size(reset_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(reset_wrapper, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(reset_wrapper, 0, 0);
    lv_obj_set_style_pad_hor(reset_wrapper, 20, 0);
    lv_obj_set_style_pad_ver(reset_wrapper, 0, 0);
    lv_obj_set_style_margin_bottom(reset_wrapper, 18, 0);
    lv_obj_clear_flag(reset_wrapper, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(reset_wrapper, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(reset_wrapper, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(reset_wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    diag_reset_button = create_button(reset_wrapper, "Reset Diagnostics", lv_color_hex(THEME_COLOR_WARNING),
                                      LV_PCT(100), 52, &lv_font_montserrat_24);
    lv_obj_set_style_text_color(diag_reset_button, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    create_separator(parent, nullptr, LV_OPA_30);

    // Noise Floor section
    create_description_label(parent, "NOISE FLOOR", &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));

    // Bar: current standard deviation against the settling tolerance - same capacity-bar shape
    // as the Info page's memory bar, recoloring to warning past 100% the same way.
    lv_obj_t* noise = lv_obj_create(parent);
    lv_obj_set_size(noise, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(noise, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(noise, 0, 0);
    lv_obj_set_style_pad_hor(noise, 10, 0);
    lv_obj_set_style_pad_ver(noise, 4, 0);
    lv_obj_clear_flag(noise, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(noise, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(noise, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(noise, 6, 0);

    lv_obj_t* noise_labels = lv_obj_create(noise);
    lv_obj_set_size(noise_labels, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(noise_labels, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(noise_labels, 0, 0);
    lv_obj_set_style_pad_all(noise_labels, 0, 0);
    lv_obj_clear_flag(noise_labels, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(noise_labels, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(noise_labels, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(noise_labels, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    diag_noise_signal_label = lv_label_create(noise_labels);
    lv_label_set_text(diag_noise_signal_label, "Signal");
    lv_obj_set_style_text_font(diag_noise_signal_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(diag_noise_signal_label, lv_color_hex(THEME_COLOR_MENU_SYSTEM), 0);

    lv_obj_t* noise_threshold_caption = lv_label_create(noise_labels);
    lv_label_set_text(noise_threshold_caption, "Threshold");
    lv_obj_set_style_text_font(noise_threshold_caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(noise_threshold_caption, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    lv_obj_t* noise_bar = lv_obj_create(noise);
    lv_obj_set_size(noise_bar, LV_PCT(100), 10);
    lv_obj_set_style_radius(noise_bar, 5, 0);
    lv_obj_set_style_bg_color(noise_bar, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_opa(noise_bar, LV_OPA_20, 0);
    lv_obj_set_style_border_width(noise_bar, 0, 0);
    lv_obj_set_style_pad_all(noise_bar, 0, 0);
    lv_obj_set_style_clip_corner(noise_bar, true, 0);
    lv_obj_clear_flag(noise_bar, LV_OBJ_FLAG_SCROLLABLE);

    diag_noise_fill = lv_obj_create(noise_bar);
    lv_obj_set_size(diag_noise_fill, 0, LV_PCT(100));
    lv_obj_set_style_radius(diag_noise_fill, 0, 0);
    lv_obj_set_style_bg_color(diag_noise_fill, lv_color_hex(THEME_COLOR_MENU_SYSTEM), 0);
    lv_obj_set_style_border_width(diag_noise_fill, 0, 0);
    lv_obj_clear_flag(diag_noise_fill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* noise_counts = lv_obj_create(noise);
    lv_obj_set_size(noise_counts, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(noise_counts, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(noise_counts, 0, 0);
    lv_obj_set_style_pad_all(noise_counts, 0, 0);
    lv_obj_clear_flag(noise_counts, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(noise_counts, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(noise_counts, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(noise_counts, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    diag_std_dev_g_label = lv_label_create(noise_counts);
    lv_label_set_text(diag_std_dev_g_label, "0.0000 g");
    lv_obj_set_style_text_font(diag_std_dev_g_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(diag_std_dev_g_label, lv_color_hex(THEME_COLOR_MENU_SYSTEM), 0);

    char threshold_text[24];
    snprintf(threshold_text, sizeof(threshold_text), "%.4f g", GRIND_SCALE_SETTLING_TOLERANCE_G);
    lv_obj_t* noise_threshold_value = lv_label_create(noise_counts);
    lv_label_set_text(noise_threshold_value, threshold_text);
    lv_obj_set_style_text_font(noise_threshold_value, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(noise_threshold_value, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    diag_noise_caption_label = create_description_label(noise, "0% of settling tolerance.",
                            &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    create_flat_data_row(parent, "Std Dev (ADC)", &diag_std_dev_adc_label, false, &lv_font_montserrat_20);

    create_separator(parent, nullptr, LV_OPA_30);

    // Motor Response section
    create_description_label(parent, "MOTOR RESPONSE", &lv_font_montserrat_24, lv_color_hex(THEME_COLOR_TEXT_PRIMARY));
    create_flat_data_row(parent, "Motor Latency", &diag_motor_latency_label, false, &lv_font_montserrat_20);

    // Register event for the button (done here because widgets are created lazily)
    using ET = EventBridgeLVGL::EventType;
    if (diag_reset_button) {
        lv_obj_add_event_cb(diag_reset_button, EventBridgeLVGL::dispatch_event, LV_EVENT_CLICKED,
                           reinterpret_cast<void*>(static_cast<intptr_t>(ET::MENU_DIAGNOSTIC_RESET)));
    }
}

void MenuScreen::show() {
    LOG_BLE("[%lums MENU] Showing menu screen\n", millis());

    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = true;
    update_ble_status();
    update_brightness_sliders();
    update_bluetooth_startup_toggle();
    update_logging_toggle();
    update_grind_mode_toggles();

    LOG_BLE("[%lums MENU] Menu screen shown successfully\n", millis());
}

void MenuScreen::show_main_page() {
    if (menu && main_page) {
        lv_menu_set_page(menu, main_page);
    }
}

void MenuScreen::hide() {
    if (!visible) {
        return; // Already hidden, nothing to do
    }

    LOG_BLE("[%lums MENU] Hiding menu screen\n", millis());

    // Keep the menu in memory for instant access next time
    // Just hide the screen instead of deleting the menu
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = false;
    LOG_BLE("[%lums MENU] Menu screen hidden successfully\n", millis());
}

void MenuScreen::update_info(const WeightSensor* weight_sensor, unsigned long uptime_ms, size_t free_heap) {
    if (!visible) return;

    set_label_text_float(instant_label, weight_sensor->get_instant_weight(), "g");
    set_label_text_int(samples_label, weight_sensor->get_sample_count());
    set_label_text_int(raw_label, weight_sensor->get_raw_adc_instant());

    // Update uptime - use compact format to avoid horizontal scrolling
    unsigned long seconds = uptime_ms / 1000;
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;

    char uptime_text[48];
    snprintf(uptime_text, sizeof(uptime_text), "%02lu:%02lu:%02lu", hours, minutes, seconds);
    lv_label_set_text(uptime_label, uptime_text);

    // Memory bar: fill tracks heap used against the live heap size, recoloring to
    // warning once free memory drops below the threshold (mirrors the diagnostics
    // status color-swap between THEME_COLOR_SUCCESS and THEME_COLOR_WARNING).
    size_t heap_total = ESP.getHeapSize();
    size_t heap_used = heap_total > free_heap ? heap_total - free_heap : 0;
    uint8_t used_pct = heap_total > 0 ? static_cast<uint8_t>((heap_used * 100) / heap_total) : 0;
    bool low_memory = used_pct >= SYS_MENU_MEMORY_WARNING_THRESHOLD_PCT;
    lv_color_t used_color = lv_color_hex(low_memory ? THEME_COLOR_WARNING : THEME_COLOR_MENU_SYSTEM);

    lv_obj_set_width(info_mem_fill, LV_PCT(used_pct));
    lv_obj_set_style_bg_color(info_mem_fill, used_color, 0);
    lv_obj_set_style_text_color(info_mem_used_label, used_color, 0);
    lv_obj_set_style_text_color(info_mem_used_value_label, used_color, 0);
    set_label_text_int(info_mem_used_value_label, heap_used / 1024, "kB");
    set_label_text_int(info_mem_free_value_label, free_heap / 1024, "kB");

    char mem_caption[48];
    if (low_memory) {
        snprintf(mem_caption, sizeof(mem_caption), "Low memory - consider restarting.");
    } else {
        snprintf(mem_caption, sizeof(mem_caption), "%u%% of %u kB heap used.",
                 (unsigned)used_pct, (unsigned)(heap_total / 1024));
    }
    lv_label_set_text(info_mem_caption_label, mem_caption);
    lv_obj_set_style_text_color(info_mem_caption_label, lv_color_hex(low_memory ? THEME_COLOR_WARNING : THEME_COLOR_NEUTRAL), 0);
}

void MenuScreen::update_diagnostics(WeightSensor* weight_sensor) {
    if (!visible || !diagnostics_controller) return;

    // Update standard deviations only every 1 second to reduce noise
    static unsigned long last_std_dev_update = 0;
    unsigned long now = millis();
    if (now - last_std_dev_update >= 1000) {  // Update every 1 second
        last_std_dev_update = now;

        // Get standard deviations using same window as grind control precision settling
        float std_dev_g = weight_sensor->get_standard_deviation_g(GRIND_SCALE_PRECISION_SETTLING_TIME_MS);  // 500ms window
        int32_t std_dev_adc = weight_sensor->get_standard_deviation_adc(GRIND_SCALE_PRECISION_SETTLING_TIME_MS);  // 500ms window

        set_label_text_int(diag_std_dev_adc_label, std_dev_adc);

        // Check noise level using WeightSensor diagnostic method
        bool noise_acceptable = weight_sensor->noise_level_diagnostic();

        // Noise bar: fill tracks the current std dev against the settling tolerance, recoloring
        // to warning past 100% (mirrors the Info page's memory-used bar color-swap).
        float noise_pct_f = (std_dev_g / GRIND_SCALE_SETTLING_TOLERANCE_G) * 100.0f;
        uint8_t noise_pct = static_cast<uint8_t>(std::min(std::max(noise_pct_f, 0.0f), 100.0f));
        lv_color_t noise_color = lv_color_hex(noise_acceptable ? THEME_COLOR_MENU_SYSTEM : THEME_COLOR_WARNING);

        lv_obj_set_width(diag_noise_fill, LV_PCT(noise_pct));
        lv_obj_set_style_bg_color(diag_noise_fill, noise_color, 0);
        lv_obj_set_style_text_color(diag_noise_signal_label, noise_color, 0);
        lv_obj_set_style_text_color(diag_std_dev_g_label, noise_color, 0);

        char std_dev_g_text[32];
        snprintf(std_dev_g_text, sizeof(std_dev_g_text), "%.4f g", std_dev_g);
        lv_label_set_text(diag_std_dev_g_label, std_dev_g_text);

        char noise_caption[64];
        if (noise_acceptable) {
            snprintf(noise_caption, sizeof(noise_caption), "%u%% of settling tolerance.", (unsigned)noise_pct);
        } else {
            snprintf(noise_caption, sizeof(noise_caption), "Exceeds settling tolerance - check for vibration or airflow.");
        }
        lv_label_set_text(diag_noise_caption_label, noise_caption);
        lv_obj_set_style_text_color(diag_noise_caption_label, noise_acceptable ? lv_color_hex(THEME_COLOR_NEUTRAL) : noise_color, 0);
    }

    // Update calibration factor
    float cal_factor = weight_sensor->get_calibration_factor();
    char cal_factor_text[32];
    snprintf(cal_factor_text, sizeof(cal_factor_text), "%.2f", cal_factor);
    lv_label_set_text(diag_calibration_factor_label, cal_factor_text);

    // Update motor latency
    if (grind_controller) {
        float motor_latency = grind_controller->get_motor_response_latency();
        char latency_text[32];
        snprintf(latency_text, sizeof(latency_text), "%.0f ms", motor_latency);
        lv_label_set_text(diag_motor_latency_label, latency_text);
    } else {
        lv_label_set_text(diag_motor_latency_label, "-- ms");
    }

    // Get highest priority diagnostic
    DiagnosticCode diagnostic = diagnostics_controller->get_highest_priority_warning();

    // Update hero status value + caption
    if (diagnostic == DiagnosticCode::NONE) {
        lv_label_set_text(diag_status_label, "OK");
        lv_obj_set_style_text_color(diag_status_label, lv_color_hex(THEME_COLOR_SUCCESS), 0);
        lv_label_set_text(diag_info_label, "All diagnostics nominal.");
        lv_obj_set_style_text_color(diag_info_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    } else {
        lv_label_set_text(diag_status_label, LV_SYMBOL_WARNING "\n" "Warning");
        lv_obj_set_style_text_color(diag_status_label, lv_color_hex(THEME_COLOR_WARNING), 0);
        lv_obj_set_style_text_color(diag_info_label, lv_color_hex(THEME_COLOR_WARNING), 0);

        // Show appropriate warning message
        if (diagnostic == DiagnosticCode::LOAD_CELL_NOT_CALIBRATED) {
            lv_label_set_text(diag_info_label, "Loadcell not calibrated");
        } else {
            // For future noise/mechanical warnings, show in info label
            const char* message = diagnostics_controller->get_diagnostic_message(diagnostic);
            lv_label_set_text(diag_info_label, message);
        }
    }
}

void MenuScreen::update_ble_status() {
    if (!visible || !bluetooth_manager) return;
    
    // Update toggle state
    if (bluetooth_manager->is_enabled()) {
        lv_obj_add_state(ble_toggle, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(ble_toggle, LV_STATE_CHECKED);
    }
    set_toggle_state_caption(ble_state_label, bluetooth_manager->is_enabled());

    // Update status text
    if (bluetooth_manager->is_enabled()) {
        if (bluetooth_manager->is_connected()) {
            lv_label_set_text(ble_status_label, "Connected");
        } else {
            lv_label_set_text(ble_status_label, "Advertising");
        }
        lv_obj_clear_flag(ble_status_section, LV_OBJ_FLAG_HIDDEN);

        // Show remaining time
        unsigned long remaining_ms = bluetooth_manager->get_bluetooth_timeout_remaining_ms();
        unsigned long remaining_min = remaining_ms / (60 * 1000);
        char timer_text[64];
        snprintf(timer_text, sizeof(timer_text), "Auto-disable in: %lu min", remaining_min);
        lv_label_set_text(ble_timer_label, timer_text);
        lv_obj_clear_flag(ble_timer_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Nothing to show while BLE is off - hide the whole section
        lv_obj_add_flag(ble_status_section, LV_OBJ_FLAG_HIDDEN);
    }
}

void MenuScreen::refresh_statistics(bool show_overlay) {
    if (!visible) return;

    // Define the statistics loading operation
    auto load_statistics_operation = [this]() {
        // Log data - Sessions Stored hero + capacity bar against the fixed
        // MAX_STORED_SESSIONS_FLASH rotation limit.
        uint32_t sessions_stored = grind_logger.get_total_flash_sessions();
        set_label_text_int(data_hero_sessions_label, sessions_stored);

        uint32_t sessions_clamped = std::min<uint32_t>(sessions_stored, MAX_STORED_SESSIONS_FLASH);
        uint32_t sessions_free = MAX_STORED_SESSIONS_FLASH - sessions_clamped;
        uint8_t capacity_pct = static_cast<uint8_t>((sessions_clamped * 100) / MAX_STORED_SESSIONS_FLASH);
        lv_obj_set_width(data_capacity_fill, LV_PCT(capacity_pct));
        set_label_text_int(data_capacity_stored_label, sessions_stored);
        set_label_text_int(data_capacity_free_label, sessions_free);

        set_label_text_int(events_label, grind_logger.count_total_events_in_flash());

        // Measurements + average per session (single flash scan, reused for both)
        uint32_t total_measurements = grind_logger.count_total_measurements_in_flash();
        set_label_text_int(measurements_label, total_measurements);
        uint32_t avg_measurements = sessions_stored > 0 ? total_measurements / sessions_stored : 0;
        char avg_text[24];
        snprintf(avg_text, sizeof(avg_text), "avg\n%lu/session", static_cast<unsigned long>(avg_measurements));
        lv_label_set_text(data_measurements_avg_label, avg_text);

        // Lifetime statistics
        set_label_text_int(stat_total_grinds_label, statistics_manager.get_total_grinds());

        // Grinds by profile - Drip cup sizes (2/4/6/8/10/Custom)
        for (int i = 0; i < 6; i++) {
            set_label_text_int(stat_drip_labels[i], statistics_manager.get_profile_shots(i));
        }

        // Espresso (Single/Double/Custom) - tracked independently of Drip
        for (int i = 0; i < 3; i++) {
            set_label_text_int(stat_espresso_labels[i], statistics_manager.get_profile_shots(i, ProfileStyle::ESPRESSO));
        }

        // Motor runtime (convert seconds to hours:minutes)
        uint64_t runtime_ms = statistics_manager.get_motor_runtime_ms();
        uint32_t runtime_sec = static_cast<uint32_t>(runtime_ms / 1000ULL);
        uint32_t hours = runtime_sec / 3600;
        uint32_t minutes = (runtime_sec % 3600) / 60;
        uint32_t seconds = runtime_sec % 60;
        char runtime_text[32];
        if (runtime_sec >= 3600) {
            snprintf(runtime_text, sizeof(runtime_text), "%luh %lum", hours, minutes);
        } else if (runtime_sec >= 60) {
            snprintf(runtime_text, sizeof(runtime_text), "%lum %lus", minutes, seconds);
        } else if (runtime_ms >= 1000) {
            float seconds_float = static_cast<float>(runtime_ms) / 1000.0f;
            snprintf(runtime_text, sizeof(runtime_text), "%.1fs", seconds_float);
        } else {
            snprintf(runtime_text, sizeof(runtime_text), "%llums", static_cast<unsigned long long>(runtime_ms));
        }
        lv_label_set_text(stat_motor_runtime_label, runtime_text);

        // Device uptime
        char uptime_text[32];
        uint32_t uptime_hours = statistics_manager.get_device_uptime_hrs();
        uint32_t uptime_minutes = statistics_manager.get_device_uptime_min_remainder();
        snprintf(uptime_text, sizeof(uptime_text), "%luh %lum", uptime_hours, uptime_minutes);
        lv_label_set_text(stat_device_uptime_label, uptime_text);

        // Total weight
        char weight_text[32];
        snprintf(weight_text, sizeof(weight_text), "%.2f kg", statistics_manager.get_total_weight_kg());
        lv_label_set_text(stat_total_weight_label, weight_text);

        // Weight vs. Time split - proportion bar + counts. LVGL's flex_grow does the
        // proportional sizing, so this just needs the two segments' integer weights.
        uint32_t weight_grinds = statistics_manager.get_weight_mode_grinds();
        uint32_t time_grinds = statistics_manager.get_time_mode_grinds();
        uint32_t mode_total = weight_grinds + time_grinds;
        uint8_t weight_pct = mode_total > 0 ? static_cast<uint8_t>((weight_grinds * 100) / mode_total) : 50;
        uint8_t time_pct = 100 - weight_pct;
        lv_obj_set_flex_grow(stat_mode_weight_fill, weight_pct);
        lv_obj_set_flex_grow(stat_mode_time_fill, time_pct);
        set_label_text_int(stat_mode_weight_count_label, weight_grinds);
        set_label_text_int(stat_mode_time_count_label, time_grinds);

        // Average accuracy - colors itself against 2x the grind tolerance, the only
        // place on this page where color carries meaning rather than decoration.
        float avg_accuracy = statistics_manager.get_avg_accuracy_g();
        char accuracy_text[16];
        snprintf(accuracy_text, sizeof(accuracy_text), "%.2fg", avg_accuracy);
        lv_label_set_text(stat_avg_accuracy_label, accuracy_text);
        bool accuracy_good = avg_accuracy <= (GRIND_ACCURACY_TOLERANCE_G * 2.0f);
        lv_obj_set_style_text_color(stat_avg_accuracy_label,
                                    lv_color_hex(accuracy_good ? THEME_COLOR_SUCCESS : THEME_COLOR_WARNING), 0);

        // Total pulses (average shown as a separate sub-line)
        set_label_text_int(stat_total_pulses_label, statistics_manager.get_total_pulses());
        char avg_pulses_text[16];
        snprintf(avg_pulses_text, sizeof(avg_pulses_text), "avg %.1f", statistics_manager.get_avg_pulses());
        lv_label_set_text(stat_avg_pulses_label, avg_pulses_text);
    };

    // Used for when we reload the statistics after a data purge
    if (!show_overlay){
        load_statistics_operation();
        return;
    }

    // Show blocking overlay while loading statistics
    auto& overlay = BlockingOperationOverlay::getInstance();
    overlay.show_and_execute(BlockingOperation::LOADING_STATISTICS, load_statistics_operation);
}

void MenuScreen::update_brightness_sliders() {
    if (!hardware_manager || !brightness_normal_slider || !brightness_screensaver_slider) return;
    
    // Read from the dedicated "brightness" namespace using a local Preferences
    // instance so we don't interfere with the global shared handle.
    Preferences prefs;
    prefs.begin("brightness", false); // writable so namespace is created on first use
    
    // Load brightness values from preferences (default to compile-time values)
    float normal_brightness = prefs.getFloat("normal", USER_SCREEN_BRIGHTNESS_NORMAL);
    float screensaver_brightness = prefs.getFloat("screensaver", USER_SCREEN_BRIGHTNESS_DIMMED);
    int auto_dim_seconds = prefs.getInt("autodim_sec", USER_SCREEN_AUTO_DIM_TIMEOUT_MS / 1000);
    prefs.end();
    
    // Convert from 0.0-1.0 to 15-100 range
    int normal_percent = (int)(normal_brightness * 100);
    int screensaver_percent = (int)(screensaver_brightness * 100);
    
    // Enforce each slider's own minimum
    if (normal_percent < HW_DISPLAY_MINIMAL_BRIGHTNESS_PERCENT) normal_percent = HW_DISPLAY_MINIMAL_BRIGHTNESS_PERCENT;
    if (screensaver_percent < HW_DISPLAY_MINIMAL_DIMMED_BRIGHTNESS_PERCENT) screensaver_percent = HW_DISPLAY_MINIMAL_DIMMED_BRIGHTNESS_PERCENT;

    // Update sliders
    lv_slider_set_value(brightness_normal_slider, normal_percent, LV_ANIM_OFF);
    lv_slider_set_value(brightness_screensaver_slider, screensaver_percent, LV_ANIM_OFF);

    update_brightness_labels(normal_percent, screensaver_percent);

    if (auto_dim_timeout_slider) {
        auto_dim_seconds = std::clamp(auto_dim_seconds, 5, 90);
        int slider_index = std::clamp((auto_dim_seconds / 5) - 1, 0, 17);
        lv_slider_set_value(auto_dim_timeout_slider, slider_index, LV_ANIM_OFF);
        update_auto_dim_timeout_label(auto_dim_seconds);
    }
}

void MenuScreen::update_brightness_labels(int normal_percent, int screensaver_percent) {
    if (brightness_normal_label && normal_percent >= 0) {
        char normal_text[16];
        snprintf(normal_text, sizeof(normal_text), "%d%%", normal_percent);
        lv_label_set_text(brightness_normal_label, normal_text);
    }

    if (brightness_screensaver_label && screensaver_percent >= 0) {
        char screensaver_text[16];
        snprintf(screensaver_text, sizeof(screensaver_text), "%d%%", screensaver_percent);
        lv_label_set_text(brightness_screensaver_label, screensaver_text);
    }
}

void MenuScreen::update_grinder_purge_amount_label(float amount_g) {
    if (grinder_purge_amount_label) {
        char buffer[16];
        float clamped_amount = amount_g;
        if (clamped_amount < GRIND_PURGE_AMOUNT_MIN_G) clamped_amount = GRIND_PURGE_AMOUNT_MIN_G;
        if (clamped_amount > GRIND_PURGE_AMOUNT_MAX_G) clamped_amount = GRIND_PURGE_AMOUNT_MAX_G;
        snprintf(buffer, sizeof(buffer), "%.1f g", clamped_amount);
        lv_label_set_text(grinder_purge_amount_label, buffer);
    }
}

void MenuScreen::update_grind_freshness_hours_label(float hours) {
    if (grind_freshness_hours_label) {
        char buffer[24];
        if (hours < 1.0f) {
            snprintf(buffer, sizeof(buffer), "%.1f h", hours);
        } else {
            snprintf(buffer, sizeof(buffer), "%.0f h", hours);
        }
        lv_label_set_text(grind_freshness_hours_label, buffer);
    }
}

void MenuScreen::update_auto_dim_timeout_label(int seconds) {
    if (auto_dim_timeout_label) {
        char buffer[8];
        snprintf(buffer, sizeof(buffer), "%ds", seconds);
        lv_label_set_text(auto_dim_timeout_label, buffer);
    }
}

lv_obj_t* MenuScreen::create_separator(lv_obj_t* parent, const char* text, lv_opa_t line_opa) {
    // Create separator container
    lv_obj_t* separator_container = lv_obj_create(parent);
    lv_obj_set_size(separator_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(separator_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(separator_container, 0, 0);
    lv_obj_set_layout(separator_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(separator_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(separator_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(separator_container, LV_OBJ_FLAG_SCROLLABLE);

    // Create left line
    lv_obj_t* left_line = lv_obj_create(separator_container);
    lv_obj_set_size(left_line, LV_SIZE_CONTENT, 2);
    lv_obj_set_flex_grow(left_line, 1);
    lv_obj_set_style_bg_color(left_line, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_opa(left_line, line_opa, 0);
    lv_obj_set_style_border_width(left_line, 0, 0);

    if (!text) {
        // If no text, make the line take full width
        return separator_container;
    }

    // Create text label
    lv_obj_t* separator_label = lv_label_create(separator_container);
    lv_label_set_text(separator_label, text);
    lv_obj_set_style_text_font(separator_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(separator_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_pad_left(separator_label, 10, 0);
    lv_obj_set_style_pad_right(separator_label, 10, 0);

    // Create right line
    lv_obj_t* right_line = lv_obj_create(separator_container);
    lv_obj_set_size(right_line, LV_SIZE_CONTENT, 2);
    lv_obj_set_flex_grow(right_line, 1);
    lv_obj_set_style_bg_color(right_line, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_opa(right_line, line_opa, 0);
    lv_obj_set_style_border_width(right_line, 0, 0);

    return separator_container;
}

lv_obj_t* MenuScreen::create_section_header(lv_obj_t* parent, const char* text) {
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_hor(container, 10, 0);
    lv_obj_set_style_pad_top(container, 20, 0);
    lv_obj_set_style_pad_bottom(container, 4, 0);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* label = lv_label_create(container);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    return container;
}

void MenuScreen::update_bluetooth_startup_toggle() {
    if (!ble_startup_toggle) return;

    // Read from the "bluetooth" namespace using a local Preferences instance
    Preferences prefs;
    prefs.begin("bluetooth", true); // read-only

    // Load startup value from preferences (default to false)
    bool startup_enabled = prefs.getBool("startup", true);
    prefs.end();

    // Update toggle state
    if (startup_enabled) {
        lv_obj_add_state(ble_startup_toggle, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(ble_startup_toggle, LV_STATE_CHECKED);
    }
    set_toggle_state_caption(ble_startup_state_label, startup_enabled);
}

void MenuScreen::update_logging_toggle() {
    if (!logging_toggle) return;

    // Read from the "logging" namespace using a local Preferences instance
    Preferences prefs;
    prefs.begin("logging", true); // read-only

    // Load logging enabled value from preferences (default to false)
    bool logging_enabled = prefs.getBool("enabled", false);
    prefs.end();

    // Update toggle state
    if (logging_enabled) {
        lv_obj_add_state(logging_toggle, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(logging_toggle, LV_STATE_CHECKED);
    }
    set_toggle_state_caption(logging_state_label, logging_enabled, lv_color_hex(THEME_COLOR_MENU_SYSTEM));
}

lv_obj_t* MenuScreen::create_menu_item(lv_obj_t* parent, const char* text, const char* icon_char, lv_color_t icon_color) {
    lv_obj_t* cont = lv_menu_cont_create(parent);

    // Flat list row: no card background/radius, just a bottom hairline divider
    lv_obj_set_width(cont, LV_PCT(100));
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_set_style_pad_hor(cont, 10, 0);
    lv_obj_set_style_pad_ver(cont, 16, 0);
    lv_obj_set_style_text_font(cont, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(cont, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_border_width(cont, 1, 0);
    lv_obj_set_style_border_side(cont, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(cont, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_opa(cont, LV_OPA_30, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    // Set layout
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Icon + label grouped together on the left, chevron pinned right
    lv_obj_t* left_group = lv_obj_create(cont);
    lv_obj_set_size(left_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(left_group, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_group, 0, 0);
    lv_obj_set_style_pad_all(left_group, 0, 0);
    lv_obj_clear_flag(left_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(left_group, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(left_group, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left_group, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(left_group, 14, 0);

    lv_obj_t* icon_label = lv_label_create(left_group);
    lv_label_set_text(icon_label, icon_char);
    lv_obj_set_style_text_font(icon_label, &lv_font_custom_icons_24, 0);
    lv_obj_set_style_text_color(icon_label, icon_color, 0);

    lv_obj_t* label = lv_label_create(left_group);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    lv_obj_t* chevron = lv_label_create(cont);
    lv_label_set_text(chevron, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(chevron, lv_color_hex(THEME_COLOR_SECONDARY), 0);

    return cont;
}

lv_obj_t* MenuScreen::create_toggle_row(lv_obj_t* parent, const char* text, lv_obj_t** out_toggle) {
    lv_obj_t* row_container = lv_obj_create(parent);
    style_as_button(row_container);
    lv_obj_set_style_margin_bottom(row_container, 10, 0);

    // Set layout
    lv_obj_set_layout(row_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* label = lv_label_create(row_container);
    lv_label_set_text(label, text);

    *out_toggle = lv_switch_create(row_container);
    lv_obj_set_size(*out_toggle, 80, 40);
    lv_obj_set_ext_click_area(*out_toggle, 20);
    
    return row_container;
}

lv_obj_t* MenuScreen::create_display_slider_row(lv_obj_t* parent, const char* name,
                                                 lv_obj_t** value_label, lv_obj_t** slider,
                                                 uint32_t min, uint32_t max,
                                                 const char* description, const lv_font_t* name_font) {
    // Flat row: name + value on one line, full-width slider below, bottom hairline divider
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_opa(row, LV_OPA_30, 0);
    lv_obj_set_style_pad_hor(row, 10, 0);
    lv_obj_set_style_pad_top(row, 14, 0);
    lv_obj_set_style_pad_bottom(row, 18, 0);
    lv_obj_set_style_pad_gap(row, 14, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Header sub-row: name on the left, value on the right
    lv_obj_t* header = lv_obj_create(row);
    lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* name_label = lv_label_create(header);
    lv_label_set_text(name_label, name);
    lv_obj_set_style_text_font(name_label, name_font, 0);
    lv_obj_set_style_text_color(name_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    *value_label = lv_label_create(header);
    lv_obj_set_style_text_font(*value_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(*value_label, lv_color_hex(THEME_COLOR_MENU_SETTINGS), 0);

    *slider = lv_slider_create(row);
    lv_obj_set_size(*slider, LV_PCT(100), 8);
    lv_obj_set_ext_click_area(*slider, 30);
    lv_slider_set_range(*slider, min, max);
    lv_obj_set_style_radius(*slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(*slider, lv_color_hex(THEME_COLOR_NEUTRAL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(*slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(*slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(*slider, lv_color_hex(THEME_COLOR_MENU_SETTINGS), LV_PART_INDICATOR);
    lv_obj_set_style_radius(*slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_bg_color(*slider, lv_color_hex(THEME_COLOR_MENU_SETTINGS), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(*slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_pad_all(*slider, 8, LV_PART_KNOB);

    if (description) {
        lv_obj_t* desc_label = lv_label_create(row);
        lv_label_set_text(desc_label, description);
        lv_obj_set_style_text_font(desc_label, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(desc_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
        lv_label_set_long_mode(desc_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(desc_label, LV_PCT(100));
    }

    return row;
}

lv_obj_t* MenuScreen::create_flat_toggle_row(lv_obj_t* parent, const char* name, lv_obj_t** out_toggle, bool with_divider,
                                              lv_obj_t** out_state_label, lv_color_t accent_color) {
    // Flat row: caps name on the left, small purple-accent switch + ON/OFF caption
    // on the right, bottom hairline divider - matches the Display page's flat row style.
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_style_border_width(row, with_divider ? 1 : 0, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_opa(row, LV_OPA_30, 0);
    lv_obj_set_style_pad_hor(row, 10, 0);
    lv_obj_set_style_pad_top(row, 14, 0);
    lv_obj_set_style_pad_bottom(row, 18, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* name_label = lv_label_create(row);
    lv_label_set_text(name_label, name);
    lv_obj_set_style_text_font(name_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(name_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    // Switch + ON/OFF caption stacked in their own column, same as create_flat_toggle_desc_row,
    // so the caption doesn't get squeezed by the space-between row layout.
    constexpr int32_t kToggleClickExtension = 35;
    lv_obj_t* toggle_col = lv_obj_create(row);
    lv_obj_set_size(toggle_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(toggle_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(toggle_col, 0, 0);
    lv_obj_set_style_pad_all(toggle_col, 0, 0);
    lv_obj_set_ext_click_area(toggle_col, kToggleClickExtension);
    lv_obj_clear_flag(toggle_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(toggle_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(toggle_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(toggle_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(toggle_col, 4, 0);

    *out_toggle = lv_switch_create(toggle_col);
    lv_obj_set_size(*out_toggle, 50, 26);
    lv_obj_set_ext_click_area(*out_toggle, kToggleClickExtension);
    lv_obj_set_style_bg_color(*out_toggle, accent_color, (lv_style_selector_t)LV_PART_INDICATOR | LV_STATE_CHECKED);

    lv_obj_t* state_label = lv_label_create(toggle_col);
    lv_obj_set_style_text_font(state_label, &lv_font_montserrat_20, 0);
    lv_obj_set_user_data(state_label, reinterpret_cast<void*>(static_cast<uintptr_t>(lv_color_to_u32(accent_color))));
    set_toggle_state_caption(state_label, false, accent_color);
    lv_obj_add_event_cb(*out_toggle, toggle_state_caption_event_cb, LV_EVENT_VALUE_CHANGED, state_label);

    if (out_state_label) {
        *out_state_label = state_label;
    }

    return row;
}

lv_obj_t* MenuScreen::create_flat_data_row(lv_obj_t* parent, const char* name, lv_obj_t** value_label, bool with_divider,
                                            const lv_font_t* font) {
    // Same flat row shell as create_flat_toggle_row, but with a right-aligned
    // value label instead of a switch - matches the Logs page's stat rows.
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_style_border_width(row, with_divider ? 1 : 0, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_opa(row, LV_OPA_30, 0);
    // Right padding is wider than the flat toggle rows' symmetric 10px so the
    // right-aligned value clears the page's scrollbar instead of colliding with it.
    lv_obj_set_style_pad_left(row, 10, 0);
    lv_obj_set_style_pad_right(row, 20, 0);
    lv_obj_set_style_pad_top(row, 6, 0);
    lv_obj_set_style_pad_bottom(row, 6, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* name_label = lv_label_create(row);
    lv_label_set_text(name_label, name);
    lv_obj_set_style_text_font(name_label, font, 0);
    lv_obj_set_style_text_color(name_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    *value_label = lv_label_create(row);
    lv_obj_set_style_text_font(*value_label, font, 0);
    lv_obj_set_style_text_color(*value_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);

    return row;
}

// One "Drip"/"Espresso" row on the Lifetime Stats page: a row label followed by
// `count` centered value/label pairs (e.g. a count over "2C", "4C", ...), replacing
// what used to be a single unlabeled slash-separated string of counts.
lv_obj_t* MenuScreen::create_stat_pair_row(lv_obj_t* parent, const char* row_label, lv_obj_t** value_labels,
                                           const char* const* pair_labels, int count) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_hor(row, 10, 0);
    lv_obj_set_style_pad_ver(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(row, 6, 0);

    lv_obj_t* label = lv_label_create(row);
    lv_label_set_text(label, row_label);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);

    // Wraps after 3 columns (fixed 30% width each) instead of squeezing all 6 Drip
    // pairs onto one line, which left them too cramped to read at 280px wide.
    lv_obj_t* pairs = lv_obj_create(row);
    lv_obj_set_size(pairs, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(pairs, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pairs, 0, 0);
    lv_obj_set_style_pad_all(pairs, 0, 0);
    lv_obj_set_style_pad_row(pairs, 10, 0);
    lv_obj_clear_flag(pairs, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(pairs, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pairs, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(pairs, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < count; i++) {
        lv_obj_t* pair = lv_obj_create(pairs);
        lv_obj_set_size(pair, LV_PCT(30), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(pair, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(pair, 0, 0);
        lv_obj_set_style_pad_all(pair, 0, 0);
        lv_obj_clear_flag(pair, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_layout(pair, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(pair, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(pair, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        value_labels[i] = lv_label_create(pair);
        lv_label_set_text(value_labels[i], "0");
        lv_obj_set_style_text_font(value_labels[i], &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(value_labels[i], lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

        lv_obj_t* pair_caption = lv_label_create(pair);
        lv_label_set_text(pair_caption, pair_labels[i]);
        lv_obj_set_style_text_font(pair_caption, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(pair_caption, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    }

    return row;
}

// Updates a toggle's ON/OFF caption label. Shared by the value-changed callback below
// (for live edits) and update_grind_mode_toggles() (for the initial load-from-prefs sync,
// which sets switch state directly and so never fires LV_EVENT_VALUE_CHANGED).
static void set_toggle_state_caption(lv_obj_t* state_label, bool checked, lv_color_t accent_color) {
    if (!state_label) return;
    lv_label_set_text(state_label, checked ? "ON" : "OFF");
    lv_obj_set_style_text_color(state_label, checked ? accent_color : lv_color_hex(THEME_COLOR_NEUTRAL), 0);
}

// The accent color is stashed on the state label's own user_data (packed into
// the pointer) so this shared callback can recover it per-row without a heap
// allocation - each row's create_flat_toggle_desc_row() call stores it there.
static void toggle_state_caption_event_cb(lv_event_t* e) {
    lv_obj_t* toggle = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t* state_label = (lv_obj_t*)lv_event_get_user_data(e);
    uint32_t accent = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(state_label)));
    set_toggle_state_caption(state_label, lv_obj_has_state(toggle, LV_STATE_CHECKED), lv_color_hex(accent));
}

lv_obj_t* MenuScreen::create_flat_toggle_desc_row(lv_obj_t* parent, const char* name, const char* description,
                                                   lv_obj_t** out_toggle, lv_obj_t** out_state_label,
                                                   lv_color_t accent_color, const lv_font_t* name_font) {
    // Flat row: title + wrapped description on the left, switch + ON/OFF caption on the right,
    // bottom hairline divider - matches the Display/Bluetooth pages' flat row style.
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_border_opa(row, LV_OPA_30, 0);
    lv_obj_set_style_pad_hor(row, 10, 0);
    lv_obj_set_style_pad_top(row, 14, 0);
    lv_obj_set_style_pad_bottom(row, 18, 0);
    lv_obj_set_style_pad_gap(row, 14, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // Left column: title + wrapped description, grows to fill the row
    lv_obj_t* text_col = lv_obj_create(row);
    lv_obj_set_width(text_col, 0);
    lv_obj_set_height(text_col, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(text_col, 1);
    lv_obj_set_style_bg_opa(text_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(text_col, 0, 0);
    lv_obj_set_style_pad_all(text_col, 0, 0);
    lv_obj_clear_flag(text_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(text_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(text_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(text_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(text_col, 4, 0);

    lv_obj_t* name_label = lv_label_create(text_col);
    lv_label_set_text(name_label, name);
    lv_obj_set_style_text_font(name_label, name_font, 0);
    lv_obj_set_style_text_color(name_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    lv_obj_t* desc_label = lv_label_create(text_col);
    lv_label_set_text(desc_label, description);
    lv_obj_set_style_text_font(desc_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(desc_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_label_set_long_mode(desc_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(desc_label, LV_PCT(100));

    // Right column: switch + ON/OFF caption, sized to content so it never gets squeezed.
    // Right margin nudges it in from the row's edge (a bare switch flush against the edge
    // reads as clipped). ext_click_area is set on BOTH the switch and this wrapping column:
    // LVGL's touch hit-test recurses into a child only if the point already falls inside that
    // child's own (extended) area, so a click landing outside toggle_col's tight content-sized
    // box would never even reach the switch's own ext_click_area check - the column needs the
    // same generous margin as the switch, not just the switch itself.
    constexpr int32_t kToggleClickExtension = 35;
    lv_obj_t* toggle_col = lv_obj_create(row);
    lv_obj_set_size(toggle_col, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(toggle_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(toggle_col, 0, 0);
    lv_obj_set_style_pad_all(toggle_col, 0, 0);
    lv_obj_set_style_margin_right(toggle_col, 14, 0);
    lv_obj_set_ext_click_area(toggle_col, kToggleClickExtension);
    lv_obj_clear_flag(toggle_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(toggle_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(toggle_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(toggle_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(toggle_col, 4, 0);

    *out_toggle = lv_switch_create(toggle_col);
    lv_obj_set_size(*out_toggle, 50, 26);
    lv_obj_set_ext_click_area(*out_toggle, kToggleClickExtension);
    lv_obj_set_style_bg_color(*out_toggle, accent_color, (lv_style_selector_t)LV_PART_INDICATOR | LV_STATE_CHECKED);

    *out_state_label = lv_label_create(toggle_col);
    lv_obj_set_style_text_font(*out_state_label, &lv_font_montserrat_20, 0);
    lv_obj_set_user_data(*out_state_label, reinterpret_cast<void*>(static_cast<uintptr_t>(lv_color_to_u32(accent_color))));
    set_toggle_state_caption(*out_state_label, false, accent_color);

    lv_obj_add_event_cb(*out_toggle, toggle_state_caption_event_cb, LV_EVENT_VALUE_CHANGED, *out_state_label);

    return row;
}

lv_obj_t* MenuScreen::create_description_label(lv_obj_t* parent, const char* text,
                                                const lv_font_t* font, lv_color_t color) {
    // Create container with padding (similar to create_data_label)
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_pad_left(container, 10, 0);
    lv_obj_set_style_pad_right(container, 14, 0);
    lv_obj_set_style_margin_top(container, 12, 0);
    lv_obj_set_style_margin_bottom(container, 12, 0);
    lv_obj_set_size(container, 280, LV_SIZE_CONTENT);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

    // Create label inside container
    lv_obj_t* label = lv_label_create(container);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(100));

    return label;
}

lv_obj_t* MenuScreen::create_profile_style_row(lv_obj_t* parent, const char* title,
                                               const char* category_desc, const char* presets_desc,
                                               lv_obj_t** out_dot) {
    constexpr int32_t kCircleOuter = 32;
    constexpr int32_t kCircleGap = kModeRowIndent - kCircleOuter;

    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_hor(row, 10, 0);
    lv_obj_set_style_pad_ver(row, 14, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(row, 6, 0);

    // Top line: radio circle + title
    lv_obj_t* top = lv_obj_create(row);
    lv_obj_set_size(top, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_pad_all(top, 0, 0);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(top, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(top, kCircleGap, 0);

    lv_obj_t* circle = lv_obj_create(top);
    lv_obj_set_size(circle, kCircleOuter, kCircleOuter);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(circle, 3, 0);
    lv_obj_set_style_border_color(circle, lv_color_hex(THEME_COLOR_MENU_GENERAL), 0);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* dot = lv_obj_create(top);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(dot, kCircleOuter / 2, kCircleOuter / 2);
    lv_obj_align_to(dot, circle, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(THEME_COLOR_MENU_GENERAL), 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* title_label = lv_label_create(top);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    // Description lines, indented to align under the title
    lv_obj_t* desc_col = lv_obj_create(row);
    lv_obj_set_size(desc_col, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(desc_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(desc_col, 0, 0);
    lv_obj_set_style_pad_all(desc_col, 0, 0);
    lv_obj_set_style_pad_left(desc_col, kCircleOuter + kCircleGap, 0);
    lv_obj_clear_flag(desc_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(desc_col, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(desc_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(desc_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(desc_col, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(desc_col, 8, 0);

    lv_obj_t* category_label = lv_label_create(desc_col);
    lv_label_set_text(category_label, category_desc);
    lv_obj_set_style_text_font(category_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(category_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_label_set_long_mode(category_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(category_label, LV_PCT(100));

    if (presets_desc) {
        lv_obj_t* presets_label = lv_label_create(desc_col);
        lv_label_set_text(presets_label, presets_desc);
        lv_obj_set_style_text_font(presets_label, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(presets_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
        lv_label_set_long_mode(presets_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(presets_label, LV_PCT(100));
    }

    if (out_dot) {
        *out_dot = dot;
    }

    return row;
}

void MenuScreen::reset_scale_display() {
    if (scale_weight_label) {
        lv_label_set_text(scale_weight_label, "0.0g");
    }
}

void MenuScreen::update_scale_weight(float weight) {
    if (!scale_weight_label) {
        return;
    }
    char buffer[24];
    snprintf(buffer, sizeof(buffer), SYS_WEIGHT_DISPLAY_FORMAT, weight);
    lv_label_set_text(scale_weight_label, buffer);
}

void MenuScreen::sync_profile_style_buttons(bool espresso_selected) {
    if (profile_style_drip_dot) {
        if (espresso_selected) {
            lv_obj_add_flag(profile_style_drip_dot, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(profile_style_drip_dot, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (profile_style_espresso_dot) {
        if (espresso_selected) {
            lv_obj_clear_flag(profile_style_espresso_dot, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(profile_style_espresso_dot, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void MenuScreen::sync_grind_type_buttons(bool time_only) {
    if (grind_type_weight_dot) {
        if (time_only) {
            lv_obj_add_flag(grind_type_weight_dot, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(grind_type_weight_dot, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (grind_type_time_only_dot) {
        if (time_only) {
            lv_obj_clear_flag(grind_type_time_only_dot, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(grind_type_time_only_dot, LV_OBJ_FLAG_HIDDEN);
        }
    }
    set_swipe_row_visible(!time_only);
}

void MenuScreen::update_grind_mode_toggles() {
    // Read swipe enabled from "swipe" namespace
    Preferences swipe_prefs;
    swipe_prefs.begin("swipe", true); // read-only
    bool swipe_enabled = swipe_prefs.getBool("enabled", false);
    swipe_prefs.end();

    // Read Time Only lock + purge settings + profile style from main grinder preferences using hardware manager
    bool time_only_mode = false;
    int grinder_purge_mode_index = GRIND_PURGE_MODE_DEFAULT;  // Default to Purge
    float grinder_purge_amount_g = GRIND_PURGE_AMOUNT_DEFAULT_G;  // Default to 1.0g
    int profile_style_index = 0;  // Default to Drip Coffee
    if (hardware_manager) {
        Preferences* main_prefs = hardware_manager->get_preferences();
        if (main_prefs) {
            time_only_mode = main_prefs->getBool("time_only_mode", false);
            grinder_purge_mode_index = main_prefs->getInt(GrindController::PREF_KEY_GRINDER_MODE, GRIND_PURGE_MODE_DEFAULT);
            grinder_purge_amount_g = main_prefs->getFloat(GrindController::PREF_KEY_GRINDER_AMOUNT_G, GRIND_PURGE_AMOUNT_DEFAULT_G);
            profile_style_index = main_prefs->getInt("profile_style", 0);
        }
    }
    sync_grind_type_buttons(time_only_mode);
    sync_profile_style_buttons(profile_style_index == 1);

    if (grind_mode_swipe_toggle) {
        if (swipe_enabled) {
            lv_obj_add_state(grind_mode_swipe_toggle, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(grind_mode_swipe_toggle, LV_STATE_CHECKED);
        }
    }
    set_toggle_state_caption(grind_mode_swipe_state_label, swipe_enabled, lv_color_hex(THEME_COLOR_MENU_GENERAL));

    // Auto actions toggles (defaults disabled)
    Preferences auto_prefs;
    auto_prefs.begin("autogrind", true);
    bool auto_start_enabled = auto_prefs.getBool("auto_start", false);
    bool auto_return_enabled = auto_prefs.getBool("auto_return", false);
    auto_prefs.end();

    if (auto_start_toggle) {
        if (auto_start_enabled) {
            lv_obj_add_state(auto_start_toggle, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(auto_start_toggle, LV_STATE_CHECKED);
        }
    }
    set_toggle_state_caption(auto_start_state_label, auto_start_enabled);

    if (auto_return_toggle) {
        if (auto_return_enabled) {
            lv_obj_add_state(auto_return_toggle, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(auto_return_toggle, LV_STATE_CHECKED);
        }
    }
    set_toggle_state_caption(auto_return_state_label, auto_return_enabled);

    // Update grinder purge mode segmented control selection
    if (grinder_purge_mode_radio_group) {
        segmented_control_set_selection(grinder_purge_mode_radio_group, grinder_purge_mode_index);
    }

    grinder_purge_amount_g = std::clamp(grinder_purge_amount_g, GRIND_PURGE_AMOUNT_MIN_G, GRIND_PURGE_AMOUNT_MAX_G);
    const int slider_min_units = static_cast<int>(GRIND_PURGE_AMOUNT_MIN_G * kPurgeSliderScale + 0.5f);
    const int slider_max_units = static_cast<int>(GRIND_PURGE_AMOUNT_MAX_G * kPurgeSliderScale + 0.5f);

    // Update grinder purge amount slider using kPurgeSliderScale (0.1g resolution)
    if (grinder_purge_amount_slider) {
        int slider_value = static_cast<int>(grinder_purge_amount_g * kPurgeSliderScale + 0.5f);
        slider_value = std::clamp(slider_value, slider_min_units, slider_max_units);
        lv_slider_set_value(grinder_purge_amount_slider, slider_value, LV_ANIM_OFF);
    }

    // Update grinder purge amount label
    update_grinder_purge_amount_label(grinder_purge_amount_g);

    // Load and set grind freshness hours
    float freshness_hours = GRIND_FRESHNESS_DEFAULT_HOURS;
    if (hardware_manager) {
        Preferences* main_prefs = hardware_manager->get_preferences();
        if (main_prefs) {
            freshness_hours = main_prefs->getFloat(GrindController::PREF_KEY_GRIND_FRESHNESS_HOURS, GRIND_FRESHNESS_DEFAULT_HOURS);
        }
    }

    // Map hours to slider index (discrete steps: 0.5, 1, 2, 3, 4, 8, 12, 24, 48)
    static const float freshness_steps[] = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 8.0f, 12.0f, 24.0f, 48.0f};
    int slider_index = 5; // Default to 8h
    for (int i = 0; i < 9; i++) {
        if (fabsf(freshness_hours - freshness_steps[i]) < 0.1f) {
            slider_index = i;
            break;
        }
    }

    if (grind_freshness_hours_slider) {
        lv_slider_set_value(grind_freshness_hours_slider, slider_index, LV_ANIM_OFF);
    }

    update_grind_freshness_hours_label(freshness_hours);
}

void MenuScreen::set_swipe_row_visible(bool visible) {
    if (grind_mode_swipe_row) {
        if (visible) {
            lv_obj_clear_flag(grind_mode_swipe_row, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(grind_mode_swipe_row, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
