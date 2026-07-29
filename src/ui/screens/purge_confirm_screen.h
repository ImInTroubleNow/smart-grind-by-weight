#pragma once
#include <lvgl.h>
#include "../../config/constants.h"

class PurgeConfirmScreen {
private:
    lv_obj_t* screen;
    lv_obj_t* title_label;
    lv_obj_t* message_label;
    bool visible;

public:
    void create();
    void show();
    void hide();
    void set_message(const char* message);

    bool is_visible() const { return visible; }
    lv_obj_t* get_screen() const { return screen; }
};
