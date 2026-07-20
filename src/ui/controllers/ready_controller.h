#pragma once
#include <lvgl.h>
#include "../event_bridge_lvgl.h"

class UIManager;

// Handles profile tab navigation, long-press editing, and swipe mode switching

class ReadyUIController {
public:
    explicit ReadyUIController(UIManager* manager);

    void register_events();
    void register_tabview_events();
    void update();
    void refresh_profiles();
    void handle_tab_change(int tab);
    void handle_profile_long_press();
    void toggle_mode();
    void handle_tab_wrap();

private:
    UIManager* ui_manager_;
};
