#include "ready_screen.h"
#include <Arduino.h>
#include "../../config/constants.h"
#include "../../controllers/grind_mode_traits.h"
#include "../../controllers/profile_controller.h"
#include "../../controllers/profile_style.h"
#include "../fonts/custom_icons.h"
#include "../ui_helpers.h"

// Water volume at a 1:16 coffee:water ratio for Drip Coffee tabs; CUSTOM (and
// every Espresso tab) has no fixed volume.
static const char* DRIP_VOLUME_TEXT[USER_PROFILE_COUNT] = {
    "250mL", "500mL", "750mL", "1.0L", "1.25L", ""
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

    // Pre-create every profile tab/dot slot up to capacity, once, at boot.
    // Style switches only relabel and show/hide these existing objects from
    // here on - they are never destroyed. Repeatedly destroying/recreating
    // LVGL object trees at runtime caused real heap-corruption crashes in
    // this project before (see git history), which is why every other
    // screen already follows a create-once-at-boot pattern; the Ready
    // screen's tabview now does too.
    for (int i = 0; i < USER_PROFILE_COUNT; i++) {
        profile_tabs[i] = lv_tabview_add_tab(tabview, "");
        create_profile_page(profile_tabs[i], i);

        page_dots[i] = lv_obj_create(page_dot_row);
        lv_obj_set_size(page_dots[i], 6, 6);
        lv_obj_set_style_radius(page_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(page_dots[i], 0, 0);
        lv_obj_set_style_bg_color(page_dots[i], lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
        lv_obj_set_style_bg_opa(page_dots[i], LV_OPA_30, 0);
        lv_obj_clear_flag(page_dots[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_clear_flag(page_dots[i], LV_OBJ_FLAG_CLICKABLE);
    }

    sync_to_profile_style(profile_controller);

    visible = false;
}

void ReadyScreen::sync_to_profile_style(ProfileController* profile_controller) {
    int count = profile_controller->get_profile_count();
    ProfileStyle style = profile_controller->get_profile_style();
    GrindMode mode = profile_controller->get_grind_mode();
    active_profile_count = count;

    for (int i = 0; i < USER_PROFILE_COUNT; i++) {
        bool active = i < count;

        if (profile_tabs[i]) {
            if (active) lv_obj_clear_flag(profile_tabs[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(profile_tabs[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (page_dots[i]) {
            if (active) lv_obj_clear_flag(page_dots[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(page_dots[i], LV_OBJ_FLAG_HIDDEN);
        }

        if (!active) continue;

        if (name_labels[i]) {
            lv_label_set_text(name_labels[i], profile_controller->get_profile_name(i));
        }

        if (volume_labels[i]) {
            const char* volume_text = (style == ProfileStyle::DRIP) ? DRIP_VOLUME_TEXT[i] : "";
            bool has_volume = volume_text && volume_text[0] != '\0';
            lv_label_set_text(volume_labels[i], has_volume ? volume_text : "");
            if (has_volume) {
                lv_obj_clear_flag(volume_labels[i], LV_OBJ_FLAG_HIDDEN);
                if (water_dividers[i]) lv_obj_clear_flag(water_dividers[i], LV_OBJ_FLAG_HIDDEN);
                if (water_rows[i]) lv_obj_clear_flag(water_rows[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(volume_labels[i], LV_OBJ_FLAG_HIDDEN);
                if (water_dividers[i]) lv_obj_add_flag(water_dividers[i], LV_OBJ_FLAG_HIDDEN);
                if (water_rows[i]) lv_obj_add_flag(water_rows[i], LV_OBJ_FLAG_HIDDEN);
            }
        }

        if (weight_labels[i]) {
            char text[24];
            format_ready_value(text, sizeof(text), mode, get_profile_target(*profile_controller, mode, i));
            lv_label_set_text(weight_labels[i], text);
        }
        update_dose_labels(i, mode);
    }

    // Hidden tabs/dots change the tabview content's flex layout (hidden
    // children are skipped) - force it to resolve now, before computing the
    // scroll target below, so it isn't based on stale (pre-hide) positions.
    lv_obj_update_layout(tabview);

    update_active_dot(profile_controller->get_current_profile());
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

lv_obj_t* ReadyScreen::create_divider(lv_obj_t* parent) {
    lv_obj_t* divider = lv_obj_create(parent);
    lv_obj_set_size(divider, 60, 1);
    lv_obj_set_style_bg_color(divider, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_40, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_set_style_margin_ver(divider, 4, 0);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(divider, LV_OBJ_FLAG_CLICKABLE);
    return divider;
}

lv_obj_t* ReadyScreen::create_icon_caption_row(lv_obj_t* parent, const char* icon_char, const char* caption_text,
                                              lv_obj_t** out_icon_label, lv_obj_t** out_caption_label) {
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row, 6, 0);

    lv_obj_t* icon_label = lv_label_create(row);
    lv_label_set_text(icon_label, icon_char);
    lv_obj_set_style_text_font(icon_label, &lv_font_custom_icons_24, 0);
    lv_obj_set_style_text_color(icon_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);

    lv_obj_t* caption_label = lv_label_create(row);
    lv_label_set_text(caption_label, caption_text);
    lv_obj_set_style_text_font(caption_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(caption_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);

    if (out_icon_label) *out_icon_label = icon_label;
    if (out_caption_label) *out_caption_label = caption_label;
    return row;
}

void ReadyScreen::update_dose_labels(int index, GrindMode mode) {
    if (dose_icon_labels[index]) {
        lv_label_set_text(dose_icon_labels[index], mode == GrindMode::TIME ? ICON_WATCH : ICON_BEAN);
    }
    if (dose_caption_labels[index]) {
        lv_label_set_text(dose_caption_labels[index], mode == GrindMode::TIME ? "TIME" : "DOSE");
    }
}

void ReadyScreen::create_profile_page(lv_obj_t* parent, int profile_index) {
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(parent, 4, 0);

    // Title - the profile name (e.g. "8 CUPS"), styled as a screen title
    // rather than the dimmer subtitle treatment used elsewhere.
    name_labels[profile_index] = lv_label_create(parent);
    lv_label_set_text(name_labels[profile_index], "");
    lv_obj_set_style_text_font(name_labels[profile_index], &lv_font_montserrat_34, 0);
    lv_obj_set_style_text_color(name_labels[profile_index], lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_add_flag(name_labels[profile_index], LV_OBJ_FLAG_CLICKABLE);

    // Water block (divider + icon/caption row + volume value) - hidden as a
    // unit by sync_to_profile_style() for styles/slots with no fixed volume
    // (Espresso, CUSTOM), leaving only the divider below directly under the
    // title.
    water_dividers[profile_index] = create_divider(parent);
    water_rows[profile_index] = create_icon_caption_row(parent, ICON_DROPLET, "WATER", nullptr, nullptr);

    volume_labels[profile_index] = lv_label_create(parent);
    lv_label_set_text(volume_labels[profile_index], "");
    lv_obj_set_style_text_font(volume_labels[profile_index], &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(volume_labels[profile_index], lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_margin_bottom(volume_labels[profile_index], 6, 0);

    // Divider before the dose block - always shown, unlike the water divider
    // above, so there's still a separator when the water block is hidden.
    create_divider(parent);

    // Dose block - icon/caption switch between bean/"DOSE" (Weight mode) and
    // watch/"TIME" (Time mode) via update_dose_labels(), kept in sync with
    // weight_labels' own mode-dependent text.
    create_icon_caption_row(parent, ICON_BEAN, "DOSE", &dose_icon_labels[profile_index], &dose_caption_labels[profile_index]);

    weight_labels[profile_index] = lv_label_create(parent);
    lv_label_set_text(weight_labels[profile_index], "");
    lv_obj_set_style_text_font(weight_labels[profile_index], &lv_font_montserrat_60, 0);
    lv_obj_set_style_text_color(weight_labels[profile_index], lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
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
        update_dose_labels(i, mode);
    }
}

void ReadyScreen::set_active_tab(int tab) {
    if (tab >= 0 && tab < active_profile_count) {
        lv_tabview_set_act(tabview, tab, LV_ANIM_OFF);
        update_active_dot(tab);
    }
}

void ReadyScreen::set_profile_long_press_handler(lv_event_cb_t handler) {
    // All USER_PROFILE_COUNT slots are created once at boot and persist
    // regardless of which style is active, so attach to all of them here
    // (not just the currently-active ones) - otherwise slots that start
    // hidden (e.g. Drip's 4th-6th tabs when booting into Espresso) would
    // never get long-press-to-edit once switched to later.
    for (int i = 0; i < USER_PROFILE_COUNT; i++) {
        if (weight_labels[i]) {
            lv_obj_add_event_cb(weight_labels[i], handler, LV_EVENT_LONG_PRESSED, NULL);
        }
    }
}
