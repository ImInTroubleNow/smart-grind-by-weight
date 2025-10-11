#include "screen_timeout_controller.h"

#include <algorithm>

#include "../../config/constants.h"
#include "../../hardware/display_manager.h"
#include "../../hardware/hardware_manager.h"
#include "../ui_manager.h"

ScreenTimeoutController::ScreenTimeoutController(UIManager* manager)
    : ui_manager_(manager), screen_dimmed_(false) {}

void ScreenTimeoutController::register_events() {}

void ScreenTimeoutController::update() {
    if (!ui_manager_) {
        return;
    }

    auto* hardware = ui_manager_->hardware_manager;
    if (!hardware) {
        return;
    }

    auto* display = hardware->get_display();
    if (!display) {
        return;
    }

    auto* touch_driver = display->get_touch_driver();
    if (!touch_driver) {
        return;
    }

    if (ui_manager_->state_machine && ui_manager_->state_machine->is_state(UIState::GRINDING)) {
        if (screen_dimmed_) {
            float normal = USER_SCREEN_BRIGHTNESS_NORMAL;
            if (ui_manager_->settings_controller_) {
                normal = ui_manager_->settings_controller_->get_normal_brightness();
            }
            display->set_brightness(normal);
            screen_dimmed_ = false;
        }
        return;
    }

    uint32_t ms_since_touch = touch_driver->get_ms_since_last_touch();
    auto* sensor = hardware->get_weight_sensor();
    uint32_t ms_since_weight_activity = sensor ? sensor->get_ms_since_last_weight_activity() : ms_since_touch;

    uint32_t ms_since_last_activity = std::min(ms_since_touch, ms_since_weight_activity);
    bool should_dim = ms_since_last_activity >= USER_SCREEN_AUTO_DIM_TIMEOUT_MS;

    if (should_dim && !screen_dimmed_) {
        float dimmed = USER_SCREEN_BRIGHTNESS_DIMMED;
        if (ui_manager_->settings_controller_) {
            dimmed = ui_manager_->settings_controller_->get_screensaver_brightness();
        }
        display->set_brightness(dimmed);
        screen_dimmed_ = true;
    } else if (!should_dim && screen_dimmed_) {
        float normal = USER_SCREEN_BRIGHTNESS_NORMAL;
        if (ui_manager_->settings_controller_) {
            normal = ui_manager_->settings_controller_->get_normal_brightness();
        }
        display->set_brightness(normal);
        screen_dimmed_ = false;
    }
}
