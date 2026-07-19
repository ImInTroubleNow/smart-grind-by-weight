#pragma once
#include <lvgl.h>
#include "../../config/constants.h"
#include "../../controllers/grind_mode.h"

class ReadyScreen {
private:
    lv_obj_t* screen;
    lv_obj_t* tabview;
    lv_obj_t* profile_tabs[USER_PROFILE_COUNT];
    lv_obj_t* weight_labels[USER_PROFILE_COUNT];
    lv_obj_t* menu_tab;
    lv_obj_t* menu_icon_button;
    lv_obj_t* page_dot_row;
    lv_obj_t* page_dots[USER_PROFILE_COUNT];
    bool visible;
public:
    void create();
    void show();
    void hide();
    void update_profile_values(const float values[USER_PROFILE_COUNT], GrindMode mode);
    void set_active_tab(int tab);
    void update_active_dot(int tab);
    void set_profile_long_press_handler(lv_event_cb_t handler);
    
    bool is_visible() const { return visible; }
    lv_obj_t* get_screen() const { return screen; }
    lv_obj_t* get_tabview() const { return tabview; }
    lv_obj_t* get_menu_tab() const { return menu_tab; }
    lv_obj_t* get_menu_icon_button() const { return menu_icon_button; }
    
private:
    void create_profile_page(lv_obj_t* parent, int profile_index, const char* profile_name, float weight, const char* volume_text);
    void create_menu_page(lv_obj_t* parent);
};
