#include "autotune_screen.h"
#include "../ui_helpers.h"
#include <Arduino.h>
#include <algorithm>
#include <cstring>

namespace {

const char* phase_display_name(AutoTunePhase phase) {
    switch (phase) {
        case AutoTunePhase::PRIMING: return "Priming";
        case AutoTunePhase::BINARY_SEARCH: return "Binary search";
        case AutoTunePhase::VERIFICATION: return "Verifying";
        default: return "Priming";
    }
}

} // namespace

void AutoTuneScreen::create() {
    screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));
    lv_obj_align(screen, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_hor(screen, 16, 0);
    lv_obj_set_style_pad_top(screen, 18, 0);
    lv_obj_set_style_pad_bottom(screen, 14, 0);
    // Every flex-column container below sets its own deliberate spacing via
    // explicit margins/padding on its children - zero out the theme's default
    // row gap so it can't silently stack on top of that and inflate things.
    lv_obj_set_style_pad_row(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // === Header (eyebrow + title + description, shared across states) ===
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

    lv_obj_t* eyebrow_label = lv_label_create(header);
    lv_label_set_text(eyebrow_label, "CALIBRATION");
    lv_obj_set_style_text_font(eyebrow_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(eyebrow_label, lv_color_hex(THEME_COLOR_MENU_CALIBRATION), 0);
    lv_obj_set_style_text_letter_space(eyebrow_label, 1, 0);

    title_label = lv_label_create(header);
    lv_label_set_text(title_label, "Tune Pulses");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_top(title_label, 2, 0);

    description_label = lv_label_create(header);
    lv_label_set_text(description_label, "Finding the shortest reliable pulse");
    lv_obj_set_style_text_font(description_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(description_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_align(description_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_margin_top(description_label, 4, 0);
    lv_obj_set_width(description_label, LV_PCT(100));
    lv_label_set_long_mode(description_label, LV_LABEL_LONG_WRAP);

    lv_obj_t* header_separator = create_separator(screen, nullptr, LV_OPA_30);
    lv_obj_set_style_margin_top(header_separator, 8, 0);
    lv_obj_set_style_margin_bottom(header_separator, 10, 0);

    // === Running (status) screen ===
    status_container = lv_obj_create(screen);
    // Width only - height must come from flex_grow alone. Setting an explicit
    // LV_PCT(100) height here fights the flex algorithm's space distribution
    // and can push this container's content past the actual available area,
    // overlapping the button below it.
    lv_obj_set_width(status_container, LV_PCT(100));
    lv_obj_set_flex_grow(status_container, 1);
    lv_obj_set_style_bg_opa(status_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_container, 0, 0);
    lv_obj_set_style_pad_all(status_container, 0, 0);
    lv_obj_clear_flag(status_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(status_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(status_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // flex_grow + internal scrolling, rather than a fixed/estimated height:
    // if the real rendered content is ever taller than the space actually
    // available, it scrolls instead of overlapping cancel_button below it.
    lv_obj_t* status_top = lv_obj_create(status_container);
    lv_obj_set_width(status_top, LV_PCT(100));
    lv_obj_set_flex_grow(status_top, 1);
    lv_obj_set_style_bg_opa(status_top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_top, 0, 0);
    lv_obj_set_style_pad_all(status_top, 0, 0);
    lv_obj_set_style_pad_row(status_top, 0, 0);
    lv_obj_set_scroll_dir(status_top, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(status_top, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_layout(status_top, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_top, LV_FLEX_FLOW_COLUMN);

    // Lightweight heading (not the shared create_description_label, whose 12px
    // top/bottom margins are sized for the spacious reference pages and are
    // too generous for this screen's tighter vertical budget).
    lv_obj_t* status_heading = lv_label_create(status_top);
    lv_label_set_text(status_heading, "STATUS");
    lv_obj_set_style_text_font(status_heading, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(status_heading, lv_color_hex(THEME_COLOR_MENU_CALIBRATION), 0);
    lv_obj_set_style_margin_bottom(status_heading, 4, 0);

    create_flat_data_row(status_top, "Phase", &phase_value_label, true, &lv_font_montserrat_20);
    create_flat_data_row(status_top, "Iteration", &iteration_value_label, true, &lv_font_montserrat_20);

    lv_obj_t* testing_row = lv_obj_create(status_top);
    lv_obj_set_size(testing_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(testing_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(testing_row, 0, 0);
    lv_obj_set_style_pad_hor(testing_row, 2, 0);
    lv_obj_set_style_pad_top(testing_row, 8, 0);
    lv_obj_set_style_pad_bottom(testing_row, 4, 0);
    lv_obj_clear_flag(testing_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(testing_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(testing_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(testing_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* testing_name_label = lv_label_create(testing_row);
    lv_label_set_text(testing_name_label, "Testing");
    lv_obj_set_style_text_font(testing_name_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(testing_name_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    testing_value_label = lv_label_create(testing_row);
    lv_label_set_text(testing_value_label, "--");
    lv_obj_set_style_text_font(testing_value_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(testing_value_label, lv_color_hex(THEME_COLOR_MENU_CALIBRATION), 0);

    lv_obj_t* bar_track = lv_obj_create(status_top);
    lv_obj_set_size(bar_track, LV_PCT(100), 8);
    lv_obj_set_style_radius(bar_track, 4, 0);
    lv_obj_set_style_bg_color(bar_track, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_bg_opa(bar_track, LV_OPA_20, 0);
    lv_obj_set_style_border_width(bar_track, 0, 0);
    lv_obj_set_style_pad_all(bar_track, 0, 0);
    lv_obj_set_style_clip_corner(bar_track, true, 0);
    lv_obj_clear_flag(bar_track, LV_OBJ_FLAG_SCROLLABLE);

    testing_bar_fill = lv_obj_create(bar_track);
    lv_obj_set_size(testing_bar_fill, 0, LV_PCT(100));
    lv_obj_set_style_radius(testing_bar_fill, 4, 0);
    lv_obj_set_style_bg_color(testing_bar_fill, lv_color_hex(THEME_COLOR_MENU_CALIBRATION), 0);
    lv_obj_set_style_border_width(testing_bar_fill, 0, 0);
    lv_obj_clear_flag(testing_bar_fill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* range_row = lv_obj_create(status_top);
    lv_obj_set_size(range_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(range_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(range_row, 0, 0);
    lv_obj_set_style_pad_hor(range_row, 2, 0);
    lv_obj_set_style_pad_top(range_row, 4, 0);
    lv_obj_clear_flag(range_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(range_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(range_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(range_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    char range_min_text[16];
    snprintf(range_min_text, sizeof(range_min_text), "%.0f ms", (float)GRIND_AUTOTUNE_LATENCY_MIN_MS);
    lv_obj_t* range_min_label = lv_label_create(range_row);
    lv_label_set_text(range_min_label, range_min_text);
    lv_obj_set_style_text_font(range_min_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(range_min_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    char range_max_text[16];
    snprintf(range_max_text, sizeof(range_max_text), "%.0f ms", (float)GRIND_AUTOTUNE_LATENCY_MAX_MS);
    lv_obj_t* range_max_label = lv_label_create(range_row);
    lv_label_set_text(range_max_label, range_max_text);
    lv_obj_set_style_text_font(range_max_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(range_max_label, lv_color_hex(THEME_COLOR_NEUTRAL), 0);

    cancel_button = create_button(status_container, "Cancel", lv_color_hex(THEME_COLOR_NEUTRAL), LV_PCT(100), 52, &lv_font_montserrat_24);
    lv_obj_set_style_bg_opa(cancel_button, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cancel_button, 1, 0);
    lv_obj_set_style_border_color(cancel_button, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_obj_set_style_text_color(cancel_button, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);

    // === Result screen ===
    result_container = lv_obj_create(screen);
    lv_obj_set_width(result_container, LV_PCT(100));
    lv_obj_set_flex_grow(result_container, 1);
    lv_obj_set_style_bg_opa(result_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(result_container, 0, 0);
    lv_obj_set_style_pad_all(result_container, 0, 0);
    lv_obj_clear_flag(result_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(result_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(result_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(result_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* result_top = lv_obj_create(result_container);
    lv_obj_set_width(result_top, LV_PCT(100));
    lv_obj_set_flex_grow(result_top, 1);
    lv_obj_set_style_bg_opa(result_top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(result_top, 0, 0);
    lv_obj_set_style_pad_all(result_top, 0, 0);
    lv_obj_set_style_pad_row(result_top, 0, 0);
    lv_obj_set_scroll_dir(result_top, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(result_top, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_layout(result_top, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(result_top, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* hero = lv_obj_create(result_top);
    lv_obj_set_size(hero, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hero, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hero, 0, 0);
    lv_obj_set_style_pad_all(hero, 2, 0);
    lv_obj_set_style_margin_bottom(hero, 8, 0);
    lv_obj_clear_flag(hero, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(hero, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hero, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(hero, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(hero, 2, 0);

    lv_obj_t* hero_caption = lv_label_create(hero);
    lv_label_set_text(hero_caption, "PULSE LATENCY");
    lv_obj_set_style_text_font(hero_caption, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(hero_caption, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_letter_space(hero_caption, 1, 0);

    final_latency_label = lv_label_create(hero);
    lv_label_set_text(final_latency_label, "0 ms");
    lv_obj_set_style_text_font(final_latency_label, &lv_font_montserrat_56, 0);
    lv_obj_set_style_text_color(final_latency_label, lv_color_hex(THEME_COLOR_SUCCESS), 0);

    previous_value_row = create_flat_data_row(result_top, "Previous value", &previous_value_label, true, &lv_font_montserrat_20);
    verification_row = create_flat_data_row(result_top, "Verification", &verification_value_label, false, &lv_font_montserrat_20);

    // Stacked caption + wrapped message rather than a left/right row - the
    // error text can run long, and a flat_data_row's horizontal layout only
    // works cleanly for short values that never wrap.
    reason_row = lv_obj_create(result_top);
    lv_obj_set_size(reason_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(reason_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(reason_row, 0, 0);
    lv_obj_set_style_pad_hor(reason_row, 2, 0);
    lv_obj_set_style_pad_top(reason_row, 10, 0);
    lv_obj_set_style_pad_row(reason_row, 4, 0);
    lv_obj_clear_flag(reason_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(reason_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(reason_row, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* reason_name_label = lv_label_create(reason_row);
    lv_label_set_text(reason_name_label, "Reason");
    lv_obj_set_style_text_font(reason_name_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(reason_name_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    reason_value_label = lv_label_create(reason_row);
    lv_label_set_text(reason_value_label, "");
    lv_obj_set_style_text_font(reason_value_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(reason_value_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_width(reason_value_label, LV_PCT(100));
    lv_label_set_long_mode(reason_value_label, LV_LABEL_LONG_WRAP);

    ok_button = create_button(result_container, "OK", lv_color_hex(THEME_COLOR_SUCCESS), LV_PCT(100), 52, &lv_font_montserrat_24);
    lv_obj_set_style_text_color(ok_button, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    visible = false;
    current_state = AutoTuneScreenState::CONSOLE;
    lv_obj_add_flag(result_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
}

void AutoTuneScreen::show() {
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = true;
}

void AutoTuneScreen::hide() {
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
    visible = false;
}

void AutoTuneScreen::show_console_screen() {
    current_state = AutoTuneScreenState::CONSOLE;

    lv_obj_clear_flag(status_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(result_container, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(title_label, "Tune Pulses");
    lv_label_set_text(description_label, "Finding the shortest reliable pulse");

    lv_label_set_text(phase_value_label, "Priming");

    char iteration_text[16];
    snprintf(iteration_text, sizeof(iteration_text), "0 / %d", GRIND_AUTOTUNE_MAX_ITERATIONS);
    lv_label_set_text(iteration_value_label, iteration_text);

    lv_label_set_text(testing_value_label, "--");
    lv_obj_set_width(testing_bar_fill, 0);
}

void AutoTuneScreen::show_success_screen(float new_latency_ms, float previous_latency_ms, int verification_success_count) {
    current_state = AutoTuneScreenState::RESULT;

    lv_obj_add_flag(status_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(result_container, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(title_label, "Tune complete");
    lv_label_set_text(description_label, "Motor latency calibrated");

    char latency_text[32];
    snprintf(latency_text, sizeof(latency_text), "%.0f ms", new_latency_ms);
    lv_label_set_text(final_latency_label, latency_text);
    lv_obj_set_style_text_color(final_latency_label, lv_color_hex(THEME_COLOR_SUCCESS), 0);

    char previous_text[16];
    snprintf(previous_text, sizeof(previous_text), "%.0f ms", previous_latency_ms);
    lv_label_set_text(previous_value_label, previous_text);

    char verification_text[24];
    snprintf(verification_text, sizeof(verification_text), "%d / %d pulses",
             verification_success_count, GRIND_AUTOTUNE_VERIFICATION_PULSES);
    lv_label_set_text(verification_value_label, verification_text);

    lv_obj_clear_flag(previous_value_row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(verification_row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(reason_row, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_style_bg_color(ok_button, lv_color_hex(THEME_COLOR_SUCCESS), 0);
}

void AutoTuneScreen::show_failure_screen(const char* error_message, float previous_latency_ms) {
    current_state = AutoTuneScreenState::RESULT;

    lv_obj_add_flag(status_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(result_container, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(title_label, "Tune failed");
    lv_label_set_text(description_label, "Using previous value");

    // Nothing is saved to NVS on failure, so the value that stays active is
    // whatever was already saved before this attempt - not a hardcoded default.
    char latency_text[32];
    snprintf(latency_text, sizeof(latency_text), "%.0f ms", previous_latency_ms);
    lv_label_set_text(final_latency_label, latency_text);
    lv_obj_set_style_text_color(final_latency_label, lv_color_hex(THEME_COLOR_WARNING), 0);

    lv_label_set_text(reason_value_label,
                      (error_message && error_message[0] != '\0') ? error_message : "Unknown error");

    lv_obj_add_flag(previous_value_row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(verification_row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(reason_row, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_style_bg_color(ok_button, lv_color_hex(THEME_COLOR_WARNING), 0);
}

void AutoTuneScreen::update_progress(const AutoTuneProgress& progress) {
    if (current_state != AutoTuneScreenState::CONSOLE) {
        return;
    }

    lv_label_set_text(phase_value_label, phase_display_name(progress.phase));

    char iteration_text[16];
    snprintf(iteration_text, sizeof(iteration_text), "%d / %d", progress.iteration, GRIND_AUTOTUNE_MAX_ITERATIONS);
    lv_label_set_text(iteration_value_label, iteration_text);

    char testing_text[16];
    snprintf(testing_text, sizeof(testing_text), "%.0f ms", progress.current_pulse_ms);
    lv_label_set_text(testing_value_label, testing_text);

    float pct = (progress.current_pulse_ms - GRIND_AUTOTUNE_LATENCY_MIN_MS) /
                (GRIND_AUTOTUNE_LATENCY_MAX_MS - GRIND_AUTOTUNE_LATENCY_MIN_MS) * 100.0f;
    pct = std::clamp(pct, 0.0f, 100.0f);
    lv_obj_set_width(testing_bar_fill, LV_PCT(static_cast<int32_t>(pct)));
}
