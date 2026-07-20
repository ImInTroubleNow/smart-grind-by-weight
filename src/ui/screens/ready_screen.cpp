#include "ready_screen.h"
#include <Arduino.h>
#include "../../config/constants.h"
#include "../../controllers/grind_mode_traits.h"
#include "../../controllers/profile_controller.h"
#include "../../controllers/profile_style.h"
#include "../ui_helpers.h"

// Water volume at a 1:16 coffee:water ratio for Drip Coffee tabs; CUSTOM (and
// every Espresso tab) has no fixed volume.
static const char* DRIP_VOLUME_TEXT[USER_PROFILE_COUNT] = {
    "250mL", "500mL", "750mL", "1L", "1.25L", ""
};

void ReadyScreen::create(ProfileController* profile_controller) {
    screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(80));
    lv_obj_align(screen, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_GESTURE_BUBBLE);

    // Create tabview
    tabview = lv_tabview_create(screen);
    lv_obj_set_size(tabview, LV_PCT(100), LV_PCT(100));
    lv_obj_align(tabview, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(tabview, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_add_flag(tabview, LV_OBJ_FLAG_GESTURE_BUBBLE);

    // Hide tab buttons for swipe-only interface
    lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_add_flag(tab_btns, LV_OBJ_FLAG_HIDDEN);

    // Transparent background
    lv_obj_set_style_bg_opa(tabview, LV_OPA_TRANSP, 0);

    // Persistent menu icon - lives on 'screen' (not inside tabview) so it stays
    // visible above every profile tab, giving 1-tap access to Menu from anywhere.
    menu_icon_button = lv_btn_create(screen);
    lv_obj_set_size(menu_icon_button, 44, 44);
    lv_obj_align(menu_icon_button, LV_ALIGN_TOP_LEFT, 8, 8);
    lv_obj_set_style_radius(menu_icon_button, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(menu_icon_button, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_obj_set_style_bg_opa(menu_icon_button, LV_OPA_60, 0);
    lv_obj_set_style_border_width(menu_icon_button, 0, 0);
    lv_obj_set_style_shadow_width(menu_icon_button, 0, 0);
    lv_obj_move_foreground(menu_icon_button);

    lv_obj_t* menu_icon = lv_label_create(menu_icon_button);
    lv_label_set_text(menu_icon, LV_SYMBOL_LIST);
    lv_obj_set_style_text_font(menu_icon, &lv_font_montserrat_24, 0);
    lv_obj_center(menu_icon);

    // Page-dot indicator - lives directly on the root screen (not inside
    // 'screen', which only covers the top 80%) so it can sit in the strip
    // below the grind button. Visibility is managed manually in show()/hide()
    // since it isn't a child of 'screen'.
    page_dot_row = lv_obj_create(lv_scr_act());
    lv_obj_set_size(page_dot_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(page_dot_row, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(page_dot_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page_dot_row, 0, 0);
    lv_obj_set_style_pad_all(page_dot_row, 0, 0);
    lv_obj_clear_flag(page_dot_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(page_dot_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(page_dot_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(page_dot_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(page_dot_row, 8, 0);
    lv_obj_add_flag(page_dot_row, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < USER_PROFILE_COUNT; i++) {
        profile_tabs[i] = nullptr;
        weight_labels[i] = nullptr;
        page_dots[i] = nullptr;
    }

    populate_tabs_and_dots(profile_controller);

    visible = false;
}

void ReadyScreen::populate_tabs_and_dots(ProfileController* profile_controller) {
    int count = profile_controller->get_profile_count();
    ProfileStyle style = profile_controller->get_profile_style();

    float weights[USER_PROFILE_COUNT];
    for (int i = 0; i < count; i++) {
        weights[i] = profile_controller->get_profile_weight(i);
    }

    // Add profile tabs
    for (int i = 0; i < count; i++) {
        const char* name = profile_controller->get_profile_name(i);
        const char* volume_text = (style == ProfileStyle::DRIP) ? DRIP_VOLUME_TEXT[i] : "";
        profile_tabs[i] = lv_tabview_add_tab(tabview, name);
        create_profile_page(profile_tabs[i], i, name, weights[i], volume_text);
    }

    active_profile_count = count;

    // Rebuild the page-dot row to match the active profile count
    lv_obj_clean(page_dot_row);
    for (int i = 0; i < count; i++) {
        page_dots[i] = lv_obj_create(page_dot_row);
        lv_obj_set_size(page_dots[i], 6, 6);
        lv_obj_set_style_radius(page_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(page_dots[i], 0, 0);
        lv_obj_set_style_bg_color(page_dots[i], lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
        lv_obj_set_style_bg_opa(page_dots[i], LV_OPA_30, 0);
        lv_obj_clear_flag(page_dots[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(page_dots[i], LV_OBJ_FLAG_CLICKABLE);
    }

    update_active_dot(profile_controller->get_current_profile());
    update_profile_values(weights, count, profile_controller->get_grind_mode());
}

void ReadyScreen::rebuild_for_style(ProfileController* profile_controller) {
    // LVGL's tabview has no tab-removal API, so switching profile counts
    // requires tearing down and recreating the whole tabview object.
    lv_obj_del(tabview);

    // Every pointer previously tracked in these capacity-sized arrays is now
    // dangling (lv_obj_del invalidates all of the tabview's children,
    // including any beyond the new, possibly-smaller count) - null them all
    // out before rebuilding so stale pointers can't be mistaken for live ones.
    for (int i = 0; i < USER_PROFILE_COUNT; i++) {
        profile_tabs[i] = nullptr;
        weight_labels[i] = nullptr;
        page_dots[i] = nullptr;
    }

    tabview = lv_tabview_create(screen);
    lv_obj_set_size(tabview, LV_PCT(100), LV_PCT(100));
    lv_obj_align(tabview, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(tabview, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_add_flag(tabview, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_add_flag(tab_btns, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_style_bg_opa(tabview, LV_OPA_TRANSP, 0);

    populate_tabs_and_dots(profile_controller);

    // The fresh tabview is appended as 'screen's last child, in front of the
    // menu icon in z-order - restore the icon to the foreground.
    lv_obj_move_foreground(menu_icon_button);

    lv_tabview_set_act(tabview, profile_controller->get_current_profile(), LV_ANIM_OFF);
}

void ReadyScreen::update_active_dot(int tab) {
    for (int i = 0; i < active_profile_count; i++) {
        if (!page_dots[i]) continue;
        bool active = (i == tab);
        lv_obj_set_style_bg_color(page_dots[i],
                                  lv_color_hex(active ? THEME_COLOR_TEXT_PRIMARY : THEME_COLOR_NEUTRAL), 0);
        lv_obj_set_style_bg_opa(page_dots[i], LV_OPA_COVER, 0);
    }
}

void ReadyScreen::create_profile_page(lv_obj_t* parent, int profile_index, const char* profile_name, float weight, const char* volume_text) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 0, 0);

    lv_obj_t* name_label;
    lv_obj_t* label_container = create_profile_label(parent, &name_label, &weight_labels[profile_index]);
    lv_label_set_text(name_label, profile_name);
    lv_obj_add_flag(name_label, LV_OBJ_FLAG_CLICKABLE);

    // Volume subtitle - smaller than the name, same color, placed between
    // the name and the big weight value. Skipped when there's no fixed volume.
    if (volume_text && volume_text[0] != '\0') {
        lv_obj_t* volume_label = lv_label_create(label_container);
        lv_label_set_text(volume_label, volume_text);
        lv_obj_set_style_text_font(volume_label, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(volume_label, lv_color_hex(THEME_COLOR_SECONDARY), 0);
        lv_obj_move_to_index(volume_label, 1);
    }

    char weight_text[16];
    snprintf(weight_text, sizeof(weight_text), SYS_WEIGHT_DISPLAY_FORMAT, weight);
    lv_label_set_text(weight_labels[profile_index], weight_text);
    lv_obj_add_flag(weight_labels[profile_index], LV_OBJ_FLAG_CLICKABLE);
}

void ReadyScreen::create_menu_page(lv_obj_t* parent) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 20, 0);

    // Info label
    lv_obj_t* info_label = lv_label_create(parent);
    lv_label_set_text(info_label, "MAIN\nMENU");
    lv_obj_set_style_text_font(info_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(info_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, 0);
}

void ReadyScreen::show() {
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    if (page_dot_row) {
        lv_obj_clear_flag(page_dot_row, LV_OBJ_FLAG_HIDDEN);
    }
    visible = true;
}

void ReadyScreen::hide() {
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
    if (page_dot_row) {
        lv_obj_add_flag(page_dot_row, LV_OBJ_FLAG_HIDDEN);
    }
    visible = false;
}

void ReadyScreen::update_profile_values(const float values[], int count, GrindMode mode) {
    for (int i = 0; i < count; i++) {
        if (weight_labels[i]) {
            char text[24];
            format_ready_value(text, sizeof(text), mode, values[i]);
            lv_label_set_text(weight_labels[i], text);
        }
    }
}

void ReadyScreen::set_active_tab(int tab) {
    if (tab >= 0 && tab < active_profile_count) {
        lv_tabview_set_act(tabview, tab, LV_ANIM_OFF);
        update_active_dot(tab);
    }
}

void ReadyScreen::set_profile_long_press_handler(lv_event_cb_t handler) {
    for (int i = 0; i < active_profile_count; i++) {
        if (weight_labels[i]) {
            lv_obj_add_event_cb(weight_labels[i], handler, LV_EVENT_LONG_PRESSED, NULL);
        }
    }
}
