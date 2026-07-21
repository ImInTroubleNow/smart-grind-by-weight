#pragma once
#include <lvgl.h>
#include "../../config/constants.h"
#include "../../controllers/grind_mode.h"

class ProfileController;

class ReadyScreen {
private:
    lv_obj_t* screen;
    lv_obj_t* tabview;
    lv_obj_t* profile_tabs[USER_PROFILE_COUNT];   // capacity; all created at boot, unused slots hidden
    lv_obj_t* name_labels[USER_PROFILE_COUNT];
    lv_obj_t* water_dividers[USER_PROFILE_COUNT];
    lv_obj_t* water_rows[USER_PROFILE_COUNT];
    lv_obj_t* volume_labels[USER_PROFILE_COUNT];
    lv_obj_t* dose_icon_labels[USER_PROFILE_COUNT];
    lv_obj_t* dose_caption_labels[USER_PROFILE_COUNT];
    lv_obj_t* weight_labels[USER_PROFILE_COUNT];
    lv_obj_t* menu_tab;
    lv_obj_t* menu_icon_button;
    lv_obj_t* page_dot_row;
    lv_obj_t* page_dots[USER_PROFILE_COUNT];
    int active_profile_count;
    bool visible;
public:
    void create(ProfileController* profile_controller);
    void sync_to_profile_style(ProfileController* profile_controller);
    void show();
    void hide();
    void update_profile_values(const float values[], int count, GrindMode mode);
    void set_active_tab(int tab);
    void update_active_dot(int tab);
    void set_profile_long_press_handler(lv_event_cb_t handler);

    bool is_visible() const { return visible; }
    lv_obj_t* get_screen() const { return screen; }
    lv_obj_t* get_tabview() const { return tabview; }
    lv_obj_t* get_menu_tab() const { return menu_tab; }
    lv_obj_t* get_menu_icon_button() const { return menu_icon_button; }
    int get_active_profile_count() const { return active_profile_count; }

private:
    void create_profile_page(lv_obj_t* parent, int profile_index);
    void create_menu_page(lv_obj_t* parent);
    lv_obj_t* create_divider(lv_obj_t* parent);
    lv_obj_t* create_icon_caption_row(lv_obj_t* parent, const char* icon_char, const char* caption_text,
                                      lv_obj_t** out_icon_label, lv_obj_t** out_caption_label);
    void update_dose_labels(int index, GrindMode mode);
};
