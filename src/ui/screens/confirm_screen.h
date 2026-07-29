#pragma once
#include <lvgl.h>
#include "../../config/constants.h"

class ConfirmScreen {
private:
    lv_obj_t* screen;
    lv_obj_t* close_button;
    lv_obj_t* eyebrow_label;
    lv_obj_t* title_label;
    lv_obj_t* message_label;
    lv_obj_t* confirm_button;
    lv_obj_t* confirm_button_label;
    bool visible;

public:
    void create();
    // eyebrow is optional (nullptr = hidden): when present, it carries confirm_color
    // and the title falls back to plain white, matching the Tune Pulses screen's
    // eyebrow+title header. When absent, the title itself carries confirm_color,
    // matching every other confirmation dialog.
    void show(const char* title, const char* message,
              const char* confirm_text, lv_color_t confirm_color,
              const char* eyebrow = nullptr);
    void show(); // Show with current content (no parameters)
    void hide();

    bool is_visible() const { return visible; }
    lv_obj_t* get_screen() const { return screen; }
    lv_obj_t* get_confirm_button() const { return confirm_button; }
    lv_obj_t* get_cancel_button() const { return close_button; }
};
