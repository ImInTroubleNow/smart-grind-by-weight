#include "ui_helpers.h"
#include <cstdio>
#include <cstdlib>

void style_as_button(lv_obj_t* object, int32_t width, int32_t height, const lv_font_t* font) {
    lv_obj_set_style_radius(object, THEME_CORNER_RADIUS_PX, 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(object, lv_color_hex(THEME_COLOR_NEUTRAL), 0);
    lv_obj_set_style_text_color(object, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_text_font(object, font, 0);
    lv_obj_set_style_border_width(object, 0, 0);
    lv_obj_set_style_pad_hor(object, 20, 0);
    if (width >= 0){
        lv_obj_set_style_width(object, width, 0);
    }
    if (height >= 0){
        lv_obj_set_style_height(object, height, 0);
    }
    
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* create_button(lv_obj_t* parent, const char* text, lv_color_t bg_color, int32_t width, int32_t height, const lv_font_t* font){ 
    lv_obj_t* button = lv_btn_create(parent);
    style_as_button(button, width, height, font);
    lv_obj_set_style_bg_color(button, bg_color, 0);
    
    lv_obj_t* label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    
    return button;  
}

void set_label_text_int(lv_obj_t* label, int32_t value, const char* unit) {
    if (!label) return;
    char buf[24];

    if (unit) {
        snprintf(buf, sizeof(buf), "%ld %s", value, unit);
    } else {
        snprintf(buf, sizeof(buf), "%ld", value);
    }

    lv_label_set_text(label, buf);
}

void set_label_text_float(lv_obj_t* label, float value, const char* unit) {
    if (!label) return;
    char buf[24];
    
    if (unit) {
        snprintf(buf, sizeof(buf), "%.2f%s", value, unit);
    } else {
        snprintf(buf, sizeof(buf), "%.2f", value);
    }

    lv_label_set_text(label, buf);
}

lv_obj_t* create_profile_label(lv_obj_t* parent, lv_obj_t** profile_label, lv_obj_t** weight_label){
    lv_obj_t* label_container = lv_obj_create(parent);
    lv_obj_set_size(label_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(label_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(label_container, 0, 0);
    lv_obj_set_style_pad_all(label_container, 0, 0);
    
    // Set up button container as horizontal flex
    lv_obj_set_layout(label_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(label_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(label_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(label_container, 0, 0);

    *profile_label = lv_label_create(label_container);
    lv_label_set_text(*profile_label, "");
    lv_obj_set_style_text_font(*profile_label, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(*profile_label, lv_color_hex(THEME_COLOR_SECONDARY), 0);
    
    *weight_label = lv_label_create(label_container);
    lv_label_set_text(*weight_label, "");
    lv_obj_set_style_text_font(*weight_label, &lv_font_montserrat_60, 0);
    lv_obj_set_style_text_color(*weight_label, lv_color_hex(THEME_COLOR_TEXT_PRIMARY), 0);

    return label_container;
}

lv_obj_t* create_dual_button_row(lv_obj_t* parent, lv_obj_t** left_button, lv_obj_t** right_button, const char* left_name, const char* right_name, lv_color_t left_color, lv_color_t right_color, int height, const lv_font_t* font){
    lv_obj_t *row_container = lv_obj_create(parent);
    lv_obj_set_size(row_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row_container, 0, 0);
    lv_obj_set_style_pad_all(row_container, 0, 0);
    
    lv_obj_set_layout(row_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(row_container, 10, 0);

    *left_button = create_button(row_container, left_name, left_color, -1, height, font);
    lv_obj_set_flex_grow(*left_button, 1);

    *right_button = create_button(row_container, right_name, right_color, -1, height, font);
    lv_obj_set_flex_grow(*right_button, 1);

    return row_container;
}

lv_obj_t* create_separator(lv_obj_t* parent, const char* text, lv_opa_t line_opa) {
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

lv_obj_t* create_description_label(lv_obj_t* parent, const char* text,
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

lv_obj_t* create_flat_data_row(lv_obj_t* parent, const char* name, lv_obj_t** value_label, bool with_divider,
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

// Segmented control data structure
struct SegmentedControlData {
    lv_obj_t** buttons;
    lv_obj_t** labels;
    int button_count;
    int selected_index;
    uint32_t selected_color;
    segmented_control_callback_t callback;
    void* user_data;
};

static void segmented_control_apply_visuals(SegmentedControlData* data) {
    for (int i = 0; i < data->button_count; i++) {
        if (!data->buttons[i] || !lv_obj_is_valid(data->buttons[i])) continue;
        bool selected = (i == data->selected_index);
        lv_obj_set_style_bg_opa(data->buttons[i], selected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(data->buttons[i], lv_color_hex(data->selected_color), 0);
        // Selected segment matches every other filled-color button in the app
        // (Tare, Run, Purge Logs, Factory Reset, Reset Diagnostics): black text
        // on the colored fill, not white.
        lv_obj_set_style_text_color(data->labels[i],
                                   selected ? lv_color_hex(THEME_COLOR_BACKGROUND) : lv_color_hex(THEME_COLOR_TEXT_SECONDARY), 0);
    }
}

static void segmented_control_event_handler(lv_event_t* e) {
    lv_obj_t* clicked_button = (lv_obj_t*)lv_event_get_target(e);
    if (!clicked_button || !lv_obj_is_valid(clicked_button)) return;

    lv_obj_t* control = lv_obj_get_parent(clicked_button);
    if (!control || !lv_obj_is_valid(control)) return;

    SegmentedControlData* data = (SegmentedControlData*)lv_obj_get_user_data(control);
    if (!data || !data->buttons) return;

    int clicked_index = -1;
    for (int i = 0; i < data->button_count; i++) {
        if (data->buttons[i] == clicked_button) {
            clicked_index = i;
            break;
        }
    }

    if (clicked_index == -1 || clicked_index == data->selected_index) return;

    data->selected_index = clicked_index;
    segmented_control_apply_visuals(data);

    if (data->callback) {
        data->callback(clicked_index, data->user_data);
    }
}

static void segmented_control_delete_handler(lv_event_t* e) {
    lv_obj_t* control = (lv_obj_t*)lv_event_get_target(e);
    if (!control) return;

    SegmentedControlData* data = (SegmentedControlData*)lv_obj_get_user_data(control);
    if (!data) return;

    lv_obj_set_user_data(control, nullptr);

    if (data->buttons) {
        free(data->buttons);
        data->buttons = nullptr;
    }
    if (data->labels) {
        free(data->labels);
        data->labels = nullptr;
    }
    free(data);
}

lv_obj_t* create_segmented_control(
    lv_obj_t* parent,
    const char* options[],
    int option_count,
    int initial_selection,
    lv_color_t selected_color,
    segmented_control_callback_t callback,
    void* user_data) {

    lv_obj_t* control = lv_obj_create(parent);
    lv_obj_set_size(control, LV_PCT(100), 52);
    lv_obj_set_style_bg_opa(control, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(control, 16, 0);
    lv_obj_set_style_border_width(control, 2, 0);
    lv_obj_set_style_border_color(control, selected_color, 0);
    lv_obj_set_style_pad_all(control, 0, 0);
    lv_obj_set_style_margin_bottom(control, 10, 0);
    lv_obj_set_style_clip_corner(control, true, 0);
    lv_obj_clear_flag(control, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_layout(control, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(control, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(control, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(control, 0, 0);

    SegmentedControlData* data = (SegmentedControlData*)malloc(sizeof(SegmentedControlData));
    data->buttons = (lv_obj_t**)malloc(sizeof(lv_obj_t*) * option_count);
    data->labels = (lv_obj_t**)malloc(sizeof(lv_obj_t*) * option_count);
    data->button_count = option_count;
    data->selected_index = initial_selection;
    data->selected_color = lv_color_to_u32(selected_color);
    data->callback = callback;
    data->user_data = user_data;

    for (int i = 0; i < option_count; i++) {
        lv_obj_t* segment = lv_obj_create(control);
        lv_obj_set_size(segment, LV_SIZE_CONTENT, LV_PCT(100));
        lv_obj_set_flex_grow(segment, 1);
        lv_obj_set_style_radius(segment, 0, 0);
        lv_obj_set_style_border_width(segment, 0, 0);
        lv_obj_set_style_bg_opa(segment, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_all(segment, 0, 0);
        lv_obj_clear_flag(segment, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(segment, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t* label = lv_label_create(segment);
        lv_label_set_text(label, options[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
        lv_obj_center(label);

        data->buttons[i] = segment;
        data->labels[i] = label;

        lv_obj_add_event_cb(segment, segmented_control_event_handler, LV_EVENT_CLICKED, nullptr);
    }

    segmented_control_apply_visuals(data);

    lv_obj_set_user_data(control, data);
    lv_obj_add_event_cb(control, segmented_control_delete_handler, LV_EVENT_DELETE, nullptr);

    return control;
}

void segmented_control_set_selection(lv_obj_t* control, int selected_index) {
    if (!control) return;
    SegmentedControlData* data = (SegmentedControlData*)lv_obj_get_user_data(control);
    if (!data || !data->buttons || selected_index < 0 || selected_index >= data->button_count) return;

    data->selected_index = selected_index;
    segmented_control_apply_visuals(data);
}

int segmented_control_get_selection(lv_obj_t* control) {
    if (!control) return -1;
    SegmentedControlData* data = (SegmentedControlData*)lv_obj_get_user_data(control);
    return (data && data->buttons) ? data->selected_index : -1;
}
