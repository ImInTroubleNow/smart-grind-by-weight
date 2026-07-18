#include "ready_controller.h"

#include <Preferences.h>
#include <lvgl.h>
#include "../../config/constants.h"
#include "../../controllers/grind_mode_traits.h"
#include "../event_bridge_lvgl.h"
#include "../ui_manager.h"

namespace {
void set_translate_x_cb(void* obj, int32_t v) {
    lv_obj_set_style_translate_x(static_cast<lv_obj_t*>(obj), v, 0);
}
}

ReadyUIController::ReadyUIController(UIManager* manager)
    : ui_manager_(manager) {}

void ReadyUIController::update() {}

void ReadyUIController::refresh_profiles() {
    if (!ui_manager_ || !ui_manager_->profile_controller) {
        return;
    }

    float values[USER_PROFILE_COUNT];
    for (int i = 0; i < USER_PROFILE_COUNT; ++i) {
        values[i] = get_profile_target(*ui_manager_->profile_controller, ui_manager_->current_mode, i);
    }
    ui_manager_->ready_screen.update_profile_values(values, ui_manager_->current_mode);
}

void ReadyUIController::handle_tab_change(int tab) {
    if (!ui_manager_) {
        return;
    }

    ui_manager_->current_tab = tab;
    if (ui_manager_->profile_controller && tab < USER_PROFILE_COUNT) {
        ui_manager_->profile_controller->set_current_profile(tab);
        refresh_profiles();
    }

    if (ui_manager_->grinding_controller_) {
        ui_manager_->grinding_controller_->update_grind_button_icon();
    }
}

void ReadyUIController::handle_profile_long_press() {
    if (!ui_manager_ || !ui_manager_->state_machine) {
        return;
    }

    if (!ui_manager_->state_machine->is_state(UIState::READY) || ui_manager_->current_tab >= USER_PROFILE_COUNT) {
        return;
    }

    ui_manager_->original_target = get_current_profile_target(*ui_manager_->profile_controller, ui_manager_->current_mode);
    ui_manager_->edit_target = ui_manager_->original_target;
    ui_manager_->edit_screen.set_mode(ui_manager_->current_mode);
    if (ui_manager_->edit_controller_) {
        ui_manager_->edit_controller_->update_display();
    }
    ui_manager_->switch_to_state(UIState::EDIT);
}

void ReadyUIController::toggle_mode() {
    if (!ui_manager_ || ui_manager_->current_tab >= USER_PROFILE_COUNT) {
        return;
    }

    Preferences prefs;
    prefs.begin("swipe", true); // read-only
    bool swipe_enabled = prefs.getBool("enabled", false);
    prefs.end();

    if (!swipe_enabled) {
        return;
    }

    ui_manager_->current_mode = (ui_manager_->current_mode == GrindMode::WEIGHT)
                                    ? GrindMode::TIME
                                    : GrindMode::WEIGHT;

    if (ui_manager_->profile_controller) {
        ui_manager_->profile_controller->set_grind_mode(ui_manager_->current_mode);
    }

    refresh_profiles();
    ui_manager_->edit_target = get_current_profile_target(*ui_manager_->profile_controller, ui_manager_->current_mode);
    if (ui_manager_->state_machine && ui_manager_->state_machine->is_state(UIState::EDIT)) {
        if (ui_manager_->edit_controller_) {
            ui_manager_->edit_controller_->update_display();
        }
    }

    ui_manager_->grinding_screen.set_mode(ui_manager_->current_mode);
    if (ui_manager_->state_machine &&
        (ui_manager_->state_machine->is_state(UIState::GRINDING) ||
         ui_manager_->state_machine->is_state(UIState::GRIND_COMPLETE))) {
        if (ui_manager_->grinding_controller_) {
            ui_manager_->grinding_controller_->update_grinding_targets();
        }
    }

    if (ui_manager_->grinding_controller_) {
        ui_manager_->grinding_controller_->update_grind_button_icon();
    }
}

void ReadyUIController::handle_tab_wrap() {
    if (!ui_manager_ || !ui_manager_->state_machine) {
        return;
    }

    if (!ui_manager_->state_machine->is_state(UIState::READY) || ui_manager_->current_tab >= USER_PROFILE_COUNT) {
        return;
    }

    // Only the two boundary tabs can wrap.
    int tab = ui_manager_->current_tab;
    if (tab != 0 && tab != USER_PROFILE_COUNT - 1) {
        return;
    }

    lv_obj_t* tabview = ui_manager_->ready_screen.get_tabview();
    lv_obj_t* content = lv_tabview_get_content(tabview);

    int32_t gap = lv_obj_get_style_pad_column(content, LV_PART_MAIN);
    int32_t page_w = lv_obj_get_content_width(content);
    int32_t expected_x = tab * (gap + page_w);
    int32_t actual_x = lv_obj_get_scroll_x(content);
    int32_t overscroll = actual_x - expected_x;
    int32_t threshold = page_w / 16; // ~6% of a page width of elastic pull

    int target_tab = -1;
    if (tab == USER_PROFILE_COUNT - 1 && overscroll > threshold) {
        target_tab = 0;
    } else if (tab == 0 && overscroll < -threshold) {
        target_tab = USER_PROFILE_COUNT - 1;
    }

    if (target_tab >= 0) {
        // Cancel LVGL's own elastic snap-back first - it's still in flight
        // from the release and would otherwise animate back over our jump.
        lv_indev_reset(lv_indev_get_act(), nullptr);
        // The content is still sitting at the overscrolled (out-of-bounds)
        // position. Snap it back to a valid resting spot before jumping,
        // otherwise lv_tabview_set_act animates from an invalid starting
        // point and LVGL's scroll clamping can swallow the jump.
        lv_obj_scroll_to_x(content, expected_x, LV_ANIM_OFF);
        lv_tabview_set_act(tabview, target_tab, LV_ANIM_OFF);

        // lv_tabview_set_act() moves the screen but doesn't fire
        // VALUE_CHANGED (that only happens on user-driven scroll), so the
        // normal TAB_CHANGE path never runs. Sync state manually or
        // current_tab silently goes stale after a wrap.
        handle_tab_change(target_tab);

        // Mimic a normal tab swipe (slide in from the edge) without
        // actually scrolling through every tab in between: offset the
        // landing page with a transform and animate it back to place.
        // Wrapping forward (to tab 0) enters from the right, like a normal
        // "next tab" swipe; wrapping backward (to the last tab) enters
        // from the left, like a normal "previous tab" swipe.
        if (lv_obj_t* target_page = lv_obj_get_child(content, target_tab)) {
            int32_t start_offset = (target_tab == 0) ? page_w : -page_w;
            lv_obj_set_style_translate_x(target_page, start_offset, 0);

            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, target_page);
            lv_anim_set_values(&a, start_offset, 0);
            lv_anim_set_duration(&a, 220);
            lv_anim_set_exec_cb(&a, set_translate_x_cb);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
            lv_anim_start(&a);
        }
    }
}

void ReadyUIController::register_events() {
    if (!ui_manager_) {
        return;
    }

    lv_obj_t* ready_screen_obj = ui_manager_->ready_screen.get_screen();
    lv_obj_t* tabview = ui_manager_->ready_screen.get_tabview();

    if (tabview) {
        lv_obj_add_event_cb(tabview, EventBridgeLVGL::dispatch_event, LV_EVENT_VALUE_CHANGED,
                            reinterpret_cast<void*>(static_cast<intptr_t>(EventBridgeLVGL::EventType::TAB_CHANGE)));
    }

    // Persistent menu icon - jumps straight to Menu from any profile tab
    if (lv_obj_t* menu_icon_button = ui_manager_->ready_screen.get_menu_icon_button()) {
        lv_obj_add_event_cb(menu_icon_button, [](lv_event_t* e) {
            if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
                return;
            }
            UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
            if (ui) {
                ui->switch_to_state(UIState::MENU);
            }
        }, LV_EVENT_CLICKED, ui_manager_);
    }

    auto gesture_handler = [](lv_event_t* e) {
        if (lv_event_get_code(e) != LV_EVENT_GESTURE) {
            return;
        }
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir != LV_DIR_TOP && dir != LV_DIR_BOTTOM) {
            return;
        }
        UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
        if (ui && ui->state_machine->is_state(UIState::READY) && ui->ready_controller_) {
            ui->ready_controller_->toggle_mode();
        }
    };

    if (tabview) {
        lv_obj_add_event_cb(tabview, gesture_handler, LV_EVENT_GESTURE, ui_manager_);
    }
    if (ready_screen_obj) {
        lv_obj_add_event_cb(ready_screen_obj, gesture_handler, LV_EVENT_GESTURE, ui_manager_);
    }
    lv_obj_add_event_cb(lv_scr_act(), gesture_handler, LV_EVENT_GESTURE, ui_manager_);

    // Wrap-around: on the boundary tabs, a hard elastic pull past the edge
    // (detected on release, since LVGL's snap scrolling absorbs the drag
    // itself and never emits a gesture event at the edge) jumps to the tab
    // on the opposite end. Registered on the input device directly rather
    // than a specific widget, since RELEASED targets whichever child was
    // actually touched (e.g. a profile label) and doesn't bubble like
    // gesture events do.
    if (lv_indev_t* indev = lv_indev_get_next(nullptr)) {
        lv_indev_add_event_cb(indev, [](lv_event_t* e) {
            UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
            if (ui && ui->ready_controller_ && ui->state_machine->is_state(UIState::READY)) {
                ui->ready_controller_->handle_tab_wrap();
            }
        }, LV_EVENT_RELEASED, ui_manager_);
    }

    EventBridgeLVGL::register_handler(EventBridgeLVGL::EventType::TAB_CHANGE,
                                      [this](lv_event_t* event) {
                                          lv_obj_t* tabview_obj = static_cast<lv_obj_t*>(lv_event_get_target(event));
                                          uint32_t tab_id = lv_tabview_get_tab_act(tabview_obj);
                                          handle_tab_change(static_cast<int>(tab_id));
                                      });

    EventBridgeLVGL::register_handler(EventBridgeLVGL::EventType::PROFILE_LONG_PRESS,
                                      [this](lv_event_t*) { handle_profile_long_press(); });

    ui_manager_->ready_screen.set_profile_long_press_handler(EventBridgeLVGL::profile_long_press_handler);

}
