#include "screen_timeout_controller.h"

#include "../../config/constants.h"
#include "../../hardware/display_manager.h"
#include "../../hardware/hardware_manager.h"
#include "../ui_manager.h"

ScreenTimeoutController::ScreenTimeoutController(UIManager* manager)
    : ui_manager_(manager), screen_dimmed_(false),
      fading_(false), fade_start_ms_(0), fade_from_(1.0f), fade_to_(1.0f) {}

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
        if (screen_dimmed_ || fading_) {
            float normal = USER_SCREEN_BRIGHTNESS_NORMAL;
            if (ui_manager_->menu_controller_) {
                normal = ui_manager_->menu_controller_->get_normal_brightness();
            }
            display->set_brightness(normal);
            screen_dimmed_ = false;
            fading_ = false;
        }
        return;
    }

    uint32_t timeout_ms = USER_SCREEN_AUTO_DIM_TIMEOUT_MS;
    if (ui_manager_->menu_controller_) {
        timeout_ms = ui_manager_->menu_controller_->get_auto_dim_timeout_ms();
    }

    uint32_t ms_since_touch = touch_driver->get_ms_since_last_touch();
    auto* sensor = hardware->get_weight_sensor();
    bool recent_weight_activity = sensor &&
                                  sensor->weight_range_exceeds(timeout_ms,
                                                               USER_WEIGHT_ACTIVITY_THRESHOLD_G);

    bool should_dim = (ms_since_touch >= timeout_ms) && !recent_weight_activity;

    if (should_dim && !screen_dimmed_ && !fading_) {
        float normal = USER_SCREEN_BRIGHTNESS_NORMAL;
        float dimmed = USER_SCREEN_BRIGHTNESS_DIMMED;
        if (ui_manager_->menu_controller_) {
            normal = ui_manager_->menu_controller_->get_normal_brightness();
            dimmed = ui_manager_->menu_controller_->get_screensaver_brightness();
        }
        fade_from_ = normal;
        fade_to_ = dimmed;
        fade_start_ms_ = millis();
        fading_ = true;
    } else if (!should_dim && (screen_dimmed_ || fading_)) {
        float normal = USER_SCREEN_BRIGHTNESS_NORMAL;
        if (ui_manager_->menu_controller_) {
            normal = ui_manager_->menu_controller_->get_normal_brightness();
        }
        display->set_brightness(normal);
        screen_dimmed_ = false;
        fading_ = false;
    }

    if (fading_) {
        uint32_t elapsed = millis() - fade_start_ms_;
        if (elapsed >= kFadeDurationMs) {
            display->set_brightness(fade_to_);
            fading_ = false;
            screen_dimmed_ = true;
        } else {
            float t = static_cast<float>(elapsed) / static_cast<float>(kFadeDurationMs);
            display->set_brightness(fade_from_ + (fade_to_ - fade_from_) * t);
        }
    }
}
