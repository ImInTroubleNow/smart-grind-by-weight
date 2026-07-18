#include "ready_screen.h"
#include <Arduino.h>
#include "../../config/constants.h"
#include "../../controllers/grind_mode_traits.h"
#include "../ui_helpers.h"

void ReadyScreen::create() {
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

    // Default weights and names, driven by USER_PROFILE_COUNT
    float default_weights[USER_PROFILE_COUNT] = {
        USER_2CUP_WEIGHT_G, USER_4CUP_WEIGHT_G, USER_6CUP_WEIGHT_G,
        USER_8CUP_WEIGHT_G, USER_10CUP_WEIGHT_G, USER_CUSTOM_PROFILE_WEIGHT_G
    };
    const char* names[USER_PROFILE_COUNT] = {
        "2 CUPS", "4 CUPS", "6 CUPS", "8 CUPS", "10 CUPS", "CUSTOM"
    };

    // Add profile tabs
    for (int i = 0; i < USER_PROFILE_COUNT; i++) {
        profile_tabs[i] = lv_tabview_add_tab(tabview, names[i]);
        create_profile_page(profile_tabs[i], i, names[i], default_weights[i]);
    }

    update_profile_values(default_weights, GrindMode::WEIGHT);

    visible = false;
}

void ReadyScreen::create_profile_page(lv_obj_t* parent, int profile_index, const char* profile_name, float weight) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 0, 0);

    lv_obj_t* name_label;
    (void)create_profile_label(parent, &name_label, &weight_labels[profile_index]);
    lv_label_set_text(name_label, profile_name);
    lv_obj_add_flag(name_label, LV_OBJ_FLAG_CLICKABLE);
    
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
    visible = true;
}

void ReadyScreen::hide() {
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = false;
}

void ReadyScreen::update_profile_values(const float values[USER_PROFILE_COUNT], GrindMode mode) {
    for (int i = 0; i < USER_PROFILE_COUNT; i++) {
        if (weight_labels[i]) {
            char text[24];
            format_ready_value(text, sizeof(text), mode, values[i]);
            lv_label_set_text(weight_labels[i], text);
        }
    }
}

void ReadyScreen::set_active_tab(int tab) {
    if (tab >= 0 && tab < USER_PROFILE_COUNT) {
        lv_tabview_set_act(tabview, tab, LV_ANIM_OFF);
    }
}

void ReadyScreen::set_profile_long_press_handler(lv_event_cb_t handler) {
    for (int i = 0; i < USER_PROFILE_COUNT; i++) {
        if (weight_labels[i]) {
            lv_obj_add_event_cb(weight_labels[i], handler, LV_EVENT_LONG_PRESSED, NULL);
        }
    }
}
