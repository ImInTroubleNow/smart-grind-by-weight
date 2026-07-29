#pragma once
#include <lvgl.h>
#include "../../config/constants.h"
#include "../../controllers/autotune_controller.h"

enum class AutoTuneScreenState {
    CONSOLE,        // Running with live phase/iteration/testing status
    RESULT          // Final success/failure screen
};

class AutoTuneScreen {
private:
    lv_obj_t* screen;
    lv_obj_t* title_label;
    lv_obj_t* description_label;

    // Running (status) screen elements
    lv_obj_t* status_container;
    lv_obj_t* phase_value_label;
    lv_obj_t* iteration_value_label;
    lv_obj_t* testing_value_label;
    lv_obj_t* testing_bar_fill;
    lv_obj_t* cancel_button;

    // Result screen elements
    lv_obj_t* result_container;
    lv_obj_t* final_latency_label;
    lv_obj_t* previous_value_row;
    lv_obj_t* previous_value_label;
    lv_obj_t* verification_row;
    lv_obj_t* verification_value_label;
    lv_obj_t* reason_row;
    lv_obj_t* reason_value_label;
    lv_obj_t* ok_button;

    bool visible;
    AutoTuneScreenState current_state;

public:
    void create();
    void show();
    void hide();

    void show_console_screen();
    void show_success_screen(float new_latency_ms, float previous_latency_ms, int verification_success_count);
    void show_failure_screen(const char* error_message, float previous_latency_ms);

    void update_progress(const AutoTuneProgress& progress);

    bool is_visible() const { return visible; }
    AutoTuneScreenState get_state() const { return current_state; }
    lv_obj_t* get_screen() const { return screen; }
    lv_obj_t* get_cancel_button() const { return cancel_button; }
    lv_obj_t* get_ok_button() const { return ok_button; }
};
