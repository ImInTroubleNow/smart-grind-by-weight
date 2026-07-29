#pragma once
#include <lvgl.h>
#include "../../config/constants.h"

enum CalibrationStep {
    CAL_STEP_EMPTY,
    CAL_STEP_WEIGHT,
    CAL_STEP_NOISE_CHECK,
    CAL_STEP_COMPLETE
};

class CalibrationScreen {
private:
    lv_obj_t* screen;
    lv_obj_t* close_button;        // Top-left corner icon; hidden on Noise Check / Complete (no cancel there)
    lv_obj_t* eyebrow_label;
    lv_obj_t* title_label;
    lv_obj_t* description_label;

    // Hero block, reused across Tare / Set Weight / Complete (caption, value color and
    // sub-caption visibility change per step; see set_step()).
    lv_obj_t* hero_container;
    lv_obj_t* hero_caption_label;
    lv_obj_t* weight_label;
    lv_obj_t* hero_sub_label;

    // Weight stepper, shown only on the Set Weight step
    lv_obj_t* stepper_row;
    lv_obj_t* plus_btn;
    lv_obj_t* minus_btn;

    // Noise check, shown only on the Noise Check step
    lv_obj_t* noise_container;
    lv_obj_t* noise_status_label;
    lv_obj_t* noise_bar_fill;
    lv_obj_t* noise_metric_label;
    lv_obj_t* noise_bar_caption_label;

    lv_obj_t* ok_button;

    CalibrationStep current_step;
    float calibration_weight;
    bool visible;

public:
    void create();
    void show();
    void hide();
    void set_step(CalibrationStep step);
    void update_current_weight(float weight);
    void update_calibration_weight(float weight);
    void update_noise_status(const char* text, lv_color_t color);
    void update_noise_metric(float std_dev_g);
    void set_ok_button_enabled(bool enabled);

    bool is_visible() const { return visible; }
    CalibrationStep get_step() const { return current_step; }
    lv_obj_t* get_screen() const { return screen; }
    lv_obj_t* get_ok_button() const { return ok_button; }
    lv_obj_t* get_cancel_button() const { return close_button; }
    lv_obj_t* get_plus_btn() const { return plus_btn; }
    lv_obj_t* get_minus_btn() const { return minus_btn; }
    float get_calibration_weight() const { return calibration_weight; }
};
