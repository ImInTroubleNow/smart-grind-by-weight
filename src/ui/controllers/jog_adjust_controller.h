#pragma once
#include <lvgl.h>

class UIManager;

// Provides accelerated increment logic shared by edit and calibration controllers

class JogAdjustController {
public:
    explicit JogAdjustController(UIManager* manager);

    void register_events();
    void start(int direction);
    void stop();
    void handle_timer(lv_timer_t* timer);

private:
    UIManager* ui_manager_;
    static void timer_callback(lv_timer_t* timer);
};
