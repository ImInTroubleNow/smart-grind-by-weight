#include "grinding_screen_arc.h"
#include <Arduino.h>
#include "../../config/constants.h"

void GrindingScreenArc::create() {
    screen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen, LV_PCT(100), LV_PCT(80));
    lv_obj_align(screen, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_TRANSP, 0); // Keep transparent
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 20, 0);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE); // Make the parent screen container clickable

    // Use flex layout for centering
    lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(screen, 30, 0);

    // Profile name label - not shown in the current design (status_label
    // below the arc covers phase/state text instead), but left in place
    // since update_profile_name() is still called on every state entry.
    profile_label = lv_label_create(screen);
    lv_label_set_text(profile_label, "");
    lv_obj_set_style_text_font(profile_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(profile_label, lv_color_hex(THEME_COLOR_SECONDARY), 0);
    lv_obj_add_flag(profile_label, LV_OBJ_FLAG_HIDDEN);

    // Progress arc - a full circular track. Value fill starts at the 12
    // o'clock position (rotation 270 shifts the 0-angle background start
    // there) and sweeps clockwise (increasing angle), ending back at 12
    // o'clock at 100%.
    progress_arc = lv_arc_create(screen);
    lv_obj_set_size(progress_arc, THEME_PROGRESS_ARC_DIAMETER_PX, THEME_PROGRESS_ARC_DIAMETER_PX);
    lv_arc_set_bg_angles(progress_arc, 0, 360);
    lv_arc_set_rotation(progress_arc, 270);
    lv_arc_set_range(progress_arc, 0, 100);
    lv_arc_set_value(progress_arc, 0);
    lv_obj_set_style_arc_color(progress_arc, lv_color_hex(THEME_COLOR_ARC_WEIGHT), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(progress_arc, 20, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(progress_arc, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_arc_width(progress_arc, 20, LV_PART_MAIN);
    lv_obj_remove_style(progress_arc, nullptr, LV_PART_KNOB);
    lv_obj_set_style_translate_y(progress_arc, -20, 0);

    // Value stack (current value + target subtitle), centered inside the
    // arc via flex so the two labels stay horizontally centered relative to
    // each other as their text (and therefore width) changes - a one-time
    // lv_obj_align_to() here would freeze the offset at whatever width the
    // initial text happened to have.
    lv_obj_t* value_stack = lv_obj_create(progress_arc);
    lv_obj_set_size(value_stack, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(value_stack, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(value_stack, 0, 0);
    lv_obj_set_style_pad_all(value_stack, 0, 0);
    lv_obj_clear_flag(value_stack, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(value_stack, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(value_stack, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(value_stack, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(value_stack, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(value_stack);

    // Current value label (weight or countdown)
    weight_label = lv_label_create(value_stack);
    lv_label_set_text(weight_label, "0.0g");
    lv_obj_set_style_text_font(weight_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(weight_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    // Target subtitle ("of 15.0g" / "of 12.0s"), stacked directly under the
    // current-value label.
    target_label = lv_label_create(value_stack);
    lv_label_set_text(target_label, "");
    lv_obj_set_style_text_font(target_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(target_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_align(target_label, LV_TEXT_ALIGN_CENTER, 0);

    // Status caption (e.g. "GRINDING...", "DONE", or an error message during
    // timeout) - driven by set_status_text(). Lives directly on the root
    // screen (not inside 'screen', and not part of its flex column) so it
    // can be placed at an exact pixel position centered between the arc's
    // bottom edge and the stop button's top edge, both of which are fixed:
    // arc bottom ~262.5 (20px screen padding + centered 200px arc within the
    // 365px-tall/80%-height screen, then shifted up 20px via translate_y
    // above), stop button top 310 (kSingleButtonSize 110, kButtonBottomMargin
    // 36 from the 456px display bottom). Midpoint ~286; montserrat_24 has a
    // 27px line height, so top = 286 - 27/2 ~ 273. Visibility is managed
    // manually in show()/hide() since it isn't a child of 'screen'.
    status_label = lv_label_create(lv_scr_act());
    lv_label_set_text(status_label, "");
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_letter_space(status_label, 1, 0);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 273);
    lv_obj_clear_flag(status_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(status_label, LV_OBJ_FLAG_HIDDEN);

    // MODIFIED: Ensure all child widgets pass click events to the parent screen
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(screen); i++) {
        lv_obj_clear_flag(lv_obj_get_child(screen, i), LV_OBJ_FLAG_CLICKABLE);
    }

    visible = false;
    time_mode = false;
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
}

void GrindingScreenArc::show() {
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_HIDDEN);
    if (status_label) {
        lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
    }
    visible = true;
}

void GrindingScreenArc::hide() {
    lv_obj_add_flag(screen, LV_OBJ_FLAG_HIDDEN);
    if (status_label) {
        lv_obj_add_flag(status_label, LV_OBJ_FLAG_HIDDEN);
    }
    visible = false;
}

void GrindingScreenArc::update_profile_name(const char* name) {
    lv_label_set_text(profile_label, name);
}

void GrindingScreenArc::update_target_weight(float weight) {
    if (time_mode) {
        return;
    }
    char target_text[32];
    snprintf(target_text, sizeof(target_text), "of " SYS_WEIGHT_DISPLAY_FORMAT, weight);
    lv_label_set_text(target_label, target_text);
}

void GrindingScreenArc::update_target_weight_text(const char* text) {
    lv_label_set_text(target_label, text);
}

void GrindingScreenArc::update_target_time(float seconds) {
    char target_text[32];
    snprintf(target_text, sizeof(target_text), "of %.1fs", seconds);
    lv_label_set_text(target_label, target_text);
}

void GrindingScreenArc::update_current_weight(float weight) {
    char weight_text[16];
    snprintf(weight_text, sizeof(weight_text), SYS_WEIGHT_DISPLAY_FORMAT, weight);
    lv_obj_set_style_text_font(weight_label, &lv_font_montserrat_48, 0);
    lv_label_set_text(weight_label, weight_text);
}

void GrindingScreenArc::update_countdown(float seconds_remaining) {
    if (seconds_remaining < 0.0f) {
        seconds_remaining = 0.0f;
    }
    int total_seconds = static_cast<int>(seconds_remaining + 0.5f);
    int minutes = total_seconds / 60;
    int secs = total_seconds % 60;

    char countdown_text[16];
    if (minutes > 0) {
        snprintf(countdown_text, sizeof(countdown_text), "%d:%02d", minutes, secs);
    } else {
        snprintf(countdown_text, sizeof(countdown_text), "%ds", secs);
    }
    lv_obj_set_style_text_font(weight_label, &lv_font_montserrat_48, 0);
    lv_label_set_text(weight_label, countdown_text);
}

void GrindingScreenArc::update_tare_display() {
    // Smaller font than the normal weight/countdown display - "TARE" is
    // wider than a typical value at the 56px size and was colliding with
    // the arc ring.
    lv_obj_set_style_text_font(weight_label, &lv_font_montserrat_36, 0);
    lv_label_set_text(weight_label, "TARE");
    lv_arc_set_value(progress_arc, 0);  // Reset arc to 0 during taring
}

void GrindingScreenArc::update_progress(int percent) {
    lv_arc_set_value(progress_arc, percent);
}

void GrindingScreenArc::set_status_text(const char* text, uint32_t color_hex) {
    lv_label_set_text(status_label, text ? text : "");
    lv_obj_set_style_text_color(status_label, lv_color_hex(color_hex), 0);
}

void GrindingScreenArc::set_progress_color(uint32_t color_hex) {
    lv_obj_set_style_arc_color(progress_arc, lv_color_hex(color_hex), LV_PART_INDICATOR);
}

void GrindingScreenArc::set_time_mode(bool enabled) {
    time_mode = enabled;
    lv_obj_set_style_arc_color(progress_arc,
                               lv_color_hex(enabled ? THEME_COLOR_ARC_TIME : THEME_COLOR_ARC_WEIGHT),
                               LV_PART_INDICATOR);
}
