#include "calibration_screen.h"
#include <Arduino.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include "../../config/constants.h"
#include "../ui_helpers.h"

void CalibrationScreen::create() {
    // Screen's own padding, needed so content clears the close button (same
    // pattern as confirm_screen.cpp): lv_obj_align offsets on close_button
    // below land relative to this padded content box, not the screen's true
    // top-left, so its (8, 2) anchor has to subtract these back out. pad_top
    // is 50 (not the usual ~18) so the icon sits on its own row above the
    // centered eyebrow/title instead of relying on horizontal clearance next
    // to them - safe regardless of how long the title text runs.
    constexpr int32_t kContentPadHor = 16;
    constexpr int32_t kContentPadTop = 50;

    screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
    lv_obj_align(screen, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_hor(screen, kContentPadHor, 0);
    lv_obj_set_style_pad_top(screen, kContentPadTop, 0);
    lv_obj_set_style_pad_bottom(screen, 14, 0);
    lv_obj_set_style_pad_row(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // === Close button (top-left corner, matches confirm_screen.cpp / menu screens) ===
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

    // === Header (eyebrow + title + description) ===
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_pad_row(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    eyebrow_label = lv_label_create(header);
    lv_label_set_text(eyebrow_label, "CALIBRATION");
    lv_obj_set_style_text_font(eyebrow_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(eyebrow_label, lv_color_hex(THEME_COLOR_MENU_CALIBRATION), 0);
    lv_obj_set_style_text_letter_space(eyebrow_label, 1, 0);

    title_label = lv_label_create(header);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_top(title_label, 2, 0);

    description_label = lv_label_create(header);
    lv_obj_set_style_text_font(description_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(description_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_align(description_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_top(description_label, 4, 0);
    lv_obj_set_width(description_label, LV_PCT(100));
    lv_label_set_long_mode(description_label, LV_LABEL_LONG_WRAP);

    lv_obj_t* header_separator = create_separator(screen, nullptr, LV_OPA_30);
    lv_obj_set_style_margin_top(header_separator, 3, 0);
    lv_obj_set_style_margin_bottom(header_separator, 4, 0);

    // === Content (flex-grow; ok_button lives inside it as a SPACE_BETWEEN sibling of
    // content_top, same shape as autotune_screen.cpp's status_container/status_top/
    // cancel_button - ok_button must never sit outside this flex_grow box as a trailing
    // screen-level sibling, or it can end up pushed past the visible screen entirely) ===
    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_set_width(content, LV_PCT(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // content_top hugs its own top edge (FLEX_ALIGN_START) rather than centering in the
    // full flex-grow area, so the hero/stepper/noise blocks sit right under the separator
    // instead of floating in the middle of whatever space happens to be left over.
    lv_obj_t* content_top = lv_obj_create(content);
    lv_obj_set_width(content_top, LV_PCT(100));
    lv_obj_set_flex_grow(content_top, 1);
    lv_obj_set_style_bg_opa(content_top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_top, 0, 0);
    lv_obj_set_style_pad_all(content_top, 0, 0);
    lv_obj_set_style_pad_row(content_top, 16, 0);
    lv_obj_set_scroll_dir(content_top, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content_top, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_layout(content_top, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content_top, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content_top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // --- Hero block (Tare / Set Weight / Complete) ---
    hero_container = lv_obj_create(content_top);
    lv_obj_set_size(hero_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hero_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hero_container, 0, 0);
    lv_obj_set_style_pad_all(hero_container, 2, 0);
    lv_obj_clear_flag(hero_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(hero_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hero_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(hero_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(hero_container, 2, 0);

    hero_caption_label = lv_label_create(hero_container);
    lv_obj_set_style_text_font(hero_caption_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hero_caption_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_letter_space(hero_caption_label, 1, 0);

    weight_label = lv_label_create(hero_container);
    lv_label_set_text(weight_label, "0");
    lv_obj_set_style_text_font(weight_label, &lv_font_montserrat_56, 0);
    lv_obj_set_style_text_color(weight_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    hero_sub_label = lv_label_create(hero_container);
    lv_label_set_text(hero_sub_label, "raw sensor counts");
    lv_obj_set_style_text_font(hero_sub_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hero_sub_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    // --- Weight stepper (Set Weight only) ---
    stepper_row = lv_obj_create(content_top);
    lv_obj_set_size(stepper_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(stepper_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(stepper_row, 0, 0);
    lv_obj_set_style_pad_all(stepper_row, 0, 0);
    lv_obj_clear_flag(stepper_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(stepper_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(stepper_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(stepper_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(stepper_row, 16, 0);

    minus_btn = create_button(stepper_row, LV_SYMBOL_MINUS, lv_color_hex(THEME_COLOR_MENU_CALIBRATION), 52, 52, &lv_font_montserrat_28);
    lv_obj_set_style_radius(minus_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_text_color(minus_btn, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    plus_btn = create_button(stepper_row, LV_SYMBOL_PLUS, lv_color_hex(THEME_COLOR_MENU_CALIBRATION), 52, 52, &lv_font_montserrat_28);
    lv_obj_set_style_radius(plus_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_text_color(plus_btn, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    // --- Noise check (Noise Check only) ---
    noise_container = lv_obj_create(content_top);
    lv_obj_set_size(noise_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(noise_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(noise_container, 0, 0);
    lv_obj_set_style_pad_all(noise_container, 0, 0);
    lv_obj_set_style_pad_row(noise_container, 12, 0);
    lv_obj_clear_flag(noise_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(noise_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(noise_container, LV_FLEX_FLOW_COLUMN);

    create_flat_data_row(noise_container, "Status", &noise_status_label, false, &lv_font_montserrat_20);

    // Bar: current standard deviation against the settling tolerance - same shape as the
    // Monitor page's Noise Floor bar, recolored from emerald to the calibration accent.
    lv_obj_t* bar_block = lv_obj_create(noise_container);
    lv_obj_set_size(bar_block, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bar_block, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar_block, 0, 0);
    lv_obj_set_style_pad_hor(bar_block, 10, 0);
    lv_obj_set_style_pad_ver(bar_block, 4, 0);
    lv_obj_clear_flag(bar_block, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(bar_block, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar_block, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(bar_block, 6, 0);

    lv_obj_t* bar_labels = lv_obj_create(bar_block);
    lv_obj_set_size(bar_labels, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bar_labels, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar_labels, 0, 0);
    lv_obj_set_style_pad_all(bar_labels, 0, 0);
    lv_obj_clear_flag(bar_labels, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(bar_labels, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar_labels, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar_labels, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* bar_signal_caption = lv_label_create(bar_labels);
    lv_label_set_text(bar_signal_caption, "Signal");
    lv_obj_set_style_text_font(bar_signal_caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(bar_signal_caption, lv_color_hex(THEME_COLOR_MENU_CALIBRATION), 0);

    lv_obj_t* bar_threshold_caption = lv_label_create(bar_labels);
    lv_label_set_text(bar_threshold_caption, "Threshold");
    lv_obj_set_style_text_font(bar_threshold_caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(bar_threshold_caption, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    lv_obj_t* bar_track = lv_obj_create(bar_block);
    lv_obj_set_size(bar_track, LV_PCT(100), 10);
    lv_obj_set_style_radius(bar_track, 5, 0);
    lv_obj_set_style_bg_color(bar_track, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_opa(bar_track, LV_OPA_20, 0);
    lv_obj_set_style_border_width(bar_track, 0, 0);
    lv_obj_set_style_pad_all(bar_track, 0, 0);
    lv_obj_set_style_clip_corner(bar_track, true, 0);
    lv_obj_clear_flag(bar_track, LV_OBJ_FLAG_SCROLLABLE);

    noise_bar_fill = lv_obj_create(bar_track);
    lv_obj_set_size(noise_bar_fill, 0, LV_PCT(100));
    lv_obj_set_style_radius(noise_bar_fill, 0, 0);
    lv_obj_set_style_bg_color(noise_bar_fill, lv_color_hex(THEME_COLOR_MENU_CALIBRATION), 0);
    lv_obj_set_style_border_width(noise_bar_fill, 0, 0);
    lv_obj_clear_flag(noise_bar_fill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* bar_counts = lv_obj_create(bar_block);
    lv_obj_set_size(bar_counts, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bar_counts, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar_counts, 0, 0);
    lv_obj_set_style_pad_all(bar_counts, 0, 0);
    lv_obj_clear_flag(bar_counts, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(bar_counts, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bar_counts, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar_counts, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    noise_metric_label = lv_label_create(bar_counts);
    lv_label_set_text(noise_metric_label, "--");
    lv_obj_set_style_text_font(noise_metric_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(noise_metric_label, lv_color_hex(THEME_COLOR_MENU_CALIBRATION), 0);

    char threshold_text[24];
    snprintf(threshold_text, sizeof(threshold_text), "%.4f g", GRIND_SCALE_SETTLING_TOLERANCE_G);
    lv_obj_t* bar_threshold_value = lv_label_create(bar_counts);
    lv_label_set_text(bar_threshold_value, threshold_text);
    lv_obj_set_style_text_font(bar_threshold_value, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(bar_threshold_value, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    noise_bar_caption_label = create_description_label(bar_block, "Waiting for a stable reading.",
                                                        &lv_font_montserrat_20, lv_color_hex(THEME_COLOR_NEUTRAL));

    // === OK button (bottom of content, full width, present on every step) ===
    ok_button = create_button(content, "OK", lv_color_hex(THEME_COLOR_SUCCESS), LV_PCT(100), 52, &lv_font_montserrat_24);
    lv_obj_set_style_text_color(ok_button, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    current_step = CAL_STEP_EMPTY;
    calibration_weight = 20.0f; // Default calibration weight
    visible = false;
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
}

void CalibrationScreen::show() {
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = true;
}

void CalibrationScreen::hide() {
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = false;
}

void CalibrationScreen::set_step(CalibrationStep step) {
    current_step = step;

    // Reset every per-step block, then show only what this step needs.
    lv_obj_add_flag(hero_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(hero_sub_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(stepper_row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(noise_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(close_button, LV_OBJ_FLAG_HIDDEN);
    set_ok_button_enabled(true);

    switch (step) {
        case CAL_STEP_EMPTY:
            lv_label_set_text(title_label, "Tare Scale");
            lv_label_set_text(description_label, "Remove all weight, then confirm when empty.");
            lv_obj_clear_flag(hero_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(hero_sub_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(hero_caption_label, "CURRENT READING");
            lv_obj_set_style_text_color(weight_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
            break;

        case CAL_STEP_WEIGHT:
            lv_label_set_text(title_label, "Set Weight");
            lv_label_set_text(description_label, "Place a known weight on the scale, then set its value.");
            lv_obj_clear_flag(hero_container, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(hero_caption_label, "CALIBRATION WEIGHT");
            lv_obj_set_style_text_color(weight_label, lv_color_hex(THEME_COLOR_MENU_CALIBRATION), 0);
            lv_obj_clear_flag(stepper_row, LV_OBJ_FLAG_HIDDEN);
            set_ok_button_enabled(false);  // Hidden until weight is detected
            update_calibration_weight(calibration_weight);
            break;

        case CAL_STEP_NOISE_CHECK:
            lv_label_set_text(title_label, "Noise Check");
            lv_label_set_text(description_label, "Let vibrations settle. Don't touch the grinder or scale.");
            lv_obj_add_flag(close_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(noise_container, LV_OBJ_FLAG_HIDDEN);
            set_ok_button_enabled(false);
            update_noise_status("Checking...", lv_color_hex(THEME_COLOR_TEXT_SECONDARY));
            update_noise_metric(std::numeric_limits<float>::quiet_NaN());
            break;

        case CAL_STEP_COMPLETE:
            lv_label_set_text(title_label, "Calibration Complete");
            lv_label_set_text(description_label, "Scale is calibrated and ready to grind.");
            lv_obj_add_flag(close_button, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(hero_container, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(hero_caption_label, "CALIBRATED WEIGHT");
            lv_obj_set_style_text_color(weight_label, lv_color_hex(THEME_COLOR_SUCCESS), 0);
            break;
    }
}

void CalibrationScreen::update_current_weight(float weight) {
    // Only update current weight display when not in weight input step
    if (current_step != CAL_STEP_WEIGHT && current_step != CAL_STEP_NOISE_CHECK) {
        char weight_text[16];
        if (current_step == CAL_STEP_COMPLETE) {
            // Complete: show calibrated weight in grams
            snprintf(weight_text, sizeof(weight_text), SYS_WEIGHT_DISPLAY_FORMAT, weight);
        } else {
            // Empty: show raw sensor values
            long raw_display = (long)weight;
            snprintf(weight_text, sizeof(weight_text), SYS_RAW_VALUE_FORMAT, raw_display);
        }
        lv_label_set_text(weight_label, weight_text);
    }
}

void CalibrationScreen::update_calibration_weight(float weight) {
    calibration_weight = weight;

    // Also update the hero value if we're in the weight step
    if (current_step == CAL_STEP_WEIGHT) {
        char weight_display[16];
        snprintf(weight_display, sizeof(weight_display), SYS_WEIGHT_DISPLAY_FORMAT, weight);
        lv_label_set_text(weight_label, weight_display);
    }
}

void CalibrationScreen::update_noise_status(const char* text, lv_color_t color) {
    if (!noise_status_label) {
        return;
    }

    lv_label_set_text(noise_status_label, text);
    lv_obj_set_style_text_color(noise_status_label, color, 0);
}

void CalibrationScreen::update_noise_metric(float std_dev_g) {
    if (!noise_metric_label) {
        return;
    }

    if (std::isnan(std_dev_g) || std_dev_g < 0.0f) {
        lv_label_set_text(noise_metric_label, "--");
        lv_obj_set_width(noise_bar_fill, 0);
        lv_label_set_text(noise_bar_caption_label, "Waiting for a stable reading.");
        return;
    }

    char metric_text[24];
    snprintf(metric_text, sizeof(metric_text), "%.4f g", std_dev_g);
    lv_label_set_text(noise_metric_label, metric_text);

    float pct = std_dev_g / GRIND_SCALE_SETTLING_TOLERANCE_G * 100.0f;
    pct = std::clamp(pct, 0.0f, 100.0f);
    lv_obj_set_width(noise_bar_fill, LV_PCT(static_cast<int32_t>(pct)));

    char caption_text[40];
    snprintf(caption_text, sizeof(caption_text), "%.0f%% of settling tolerance.", pct);
    lv_label_set_text(noise_bar_caption_label, caption_text);
}

void CalibrationScreen::set_ok_button_enabled(bool enabled) {
    if (!ok_button) {
        return;
    }

    if (enabled) {
        lv_obj_clear_flag(ok_button, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ok_button, LV_OBJ_FLAG_HIDDEN);
    }
}
