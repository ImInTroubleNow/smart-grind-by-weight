#include "confirm_screen.h"
#include <Arduino.h>
#include "../ui_helpers.h"

void ConfirmScreen::create() {
    // screen's own padding, needed so content clears the close button + the
    // global status icons up top. lv_obj_align offsets on children (like
    // close_button below) land relative to this padded content box, not the
    // screen's true top-left - so any absolute on-screen position has to
    // subtract these back out, same as menu_screen.cpp does for its own
    // back/close button.
    constexpr int32_t kContentPadHor = 16;
    constexpr int32_t kContentPadTop = 50;

    screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
    lv_obj_align(screen, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_hor(screen, kContentPadHor, 0);
    lv_obj_set_style_pad_top(screen, kContentPadTop, 0);
    lv_obj_set_style_pad_bottom(screen, 16, 0);
    lv_obj_set_style_pad_row(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Close button - pinned top-left at the same spot as the menu screens'
    // back/close button (44x44 circle at 8,2). Pulled out of the flex flow
    // so it can't consume layout space or shift when content changes.
    close_button = lv_obj_create(screen);
    lv_obj_add_flag(close_button, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(close_button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(close_button, 44, 44);
    lv_obj_set_style_radius(close_button, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(close_button, lv_color_hex(THEME_COLOR_BACKGROUND), 0);
    lv_obj_set_style_bg_opa(close_button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(close_button, 0, 0);
    lv_obj_set_style_pad_all(close_button, 0, 0);
    lv_obj_clear_flag(close_button, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(close_button, LV_ALIGN_TOP_LEFT, 8 - kContentPadHor, 2 - kContentPadTop);
    lv_obj_set_ext_click_area(close_button, 20);

    lv_obj_t* close_icon = lv_label_create(close_button);
    lv_label_set_text(close_icon, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_font(close_icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(close_icon, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_center(close_icon);

    // Eyebrow (optional) + title
    eyebrow_label = lv_label_create(screen);
    lv_label_set_text(eyebrow_label, "");
    lv_obj_set_style_text_font(eyebrow_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_letter_space(eyebrow_label, 1, 0);
    lv_obj_set_style_text_align(eyebrow_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(eyebrow_label, LV_PCT(100));
    lv_obj_add_flag(eyebrow_label, LV_OBJ_FLAG_HIDDEN);

    title_label = lv_label_create(screen);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title_label, LV_PCT(100));
    lv_obj_set_style_margin_top(title_label, 2, 0);

    // Message body - left-aligned, scrolls internally if it runs long rather
    // than pushing the button off the bottom of the screen.
    lv_obj_t* message_container = lv_obj_create(screen);
    lv_obj_set_width(message_container, LV_PCT(100));
    lv_obj_set_flex_grow(message_container, 1);
    lv_obj_set_style_bg_opa(message_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(message_container, 0, 0);
    lv_obj_set_style_pad_all(message_container, 0, 0);
    lv_obj_set_style_margin_top(message_container, 16, 0);
    lv_obj_set_scroll_dir(message_container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(message_container, LV_SCROLLBAR_MODE_AUTO);

    message_label = lv_label_create(message_container);
    lv_obj_set_style_text_font(message_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(message_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_width(message_label, LV_PCT(100));
    lv_label_set_long_mode(message_label, LV_LABEL_LONG_WRAP);

    confirm_button = create_button(screen, "", lv_color_hex(THEME_COLOR_NEUTRAL), LV_PCT(100), 52, &lv_font_montserrat_24);
    lv_obj_set_style_text_color(confirm_button, lv_color_hex(THEME_COLOR_BACKGROUND), 0);
    confirm_button_label = lv_obj_get_child(confirm_button, -1);

    visible = false;
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
}

void ConfirmScreen::show(const char* title, const char* message,
                        const char* confirm_text, lv_color_t confirm_color,
                        const char* eyebrow) {
    if (eyebrow && eyebrow[0] != '\0') {
        lv_label_set_text(eyebrow_label, eyebrow);
        lv_obj_set_style_text_color(eyebrow_label, confirm_color, 0);
        lv_obj_clear_flag(eyebrow_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(title_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    } else {
        lv_obj_add_flag(eyebrow_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_color(title_label, confirm_color, 0);
    }

    lv_label_set_text(title_label, title);
    lv_label_set_text(message_label, message);

    lv_label_set_text(confirm_button_label, confirm_text);
    lv_obj_set_style_bg_color(confirm_button, confirm_color, 0);

    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = true;
}

void ConfirmScreen::show() {
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = true;
}

void ConfirmScreen::hide() {
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = false;
}
