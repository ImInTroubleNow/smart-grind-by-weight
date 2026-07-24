#pragma once
#include <lvgl.h>
#include "../config/constants.h"

// Function declarations
void style_as_button(lv_obj_t* object, int32_t width = 260, int32_t height = 80, const lv_font_t* font = &lv_font_montserrat_28);

lv_obj_t* create_button(lv_obj_t* parent, const char* text,
                       lv_color_t bg_color = lv_color_hex(THEME_COLOR_NEUTRAL),
                       int32_t width = 260, int32_t height = 80, 
                       const lv_font_t* font = &lv_font_montserrat_28);

void set_label_text_int(lv_obj_t* label, int32_t value, const char* unit = nullptr);

void set_label_text_float(lv_obj_t* label, float value, const char* unit = nullptr);

lv_obj_t* create_profile_label(lv_obj_t* parent, lv_obj_t** profile_label, lv_obj_t** weight_label);

lv_obj_t* create_dual_button_row(lv_obj_t* parent, lv_obj_t** left_button, lv_obj_t** right_button, 
                                const char* left_name, const char* right_name, 
                                lv_color_t left_color = lv_color_hex(THEME_COLOR_NEUTRAL), 
                                lv_color_t right_color = lv_color_hex(THEME_COLOR_NEUTRAL), 
                                int height = 80, const lv_font_t* font = &lv_font_montserrat_28);

lv_obj_t* create_data_label(lv_obj_t* parent, const char* name, lv_obj_t** value_label, bool stacked = false);

// Callback signature for segmented control selection changes
typedef void (*segmented_control_callback_t)(int selected_index, void* user_data);

// Joined-pill segmented control (e.g. "KEEP" / "REMOVE"): a single rounded, bordered
// container with borderless flex-grow segments, the selected one filled with selected_color.
lv_obj_t* create_segmented_control(
    lv_obj_t* parent,
    const char* options[],           // Array of segment labels
    int option_count,                // Number of segments
    int initial_selection,           // Initially selected index (0-based)
    lv_color_t selected_color,       // Fill color for the selected segment
    segmented_control_callback_t callback, // Called when selection changes
    void* user_data                  // Passed to callback
);

void segmented_control_set_selection(lv_obj_t* control, int selected_index);
int segmented_control_get_selection(lv_obj_t* control);
