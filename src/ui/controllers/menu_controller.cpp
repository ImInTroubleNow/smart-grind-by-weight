#include "menu_controller.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <esp_err.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <algorithm>
#include <cstdint>
#include "../../config/constants.h"
#include "../../controllers/grind_controller.h"
#include "../../controllers/grind_mode_traits.h"
#include "../../logging/grind_logging.h"
#include "../../system/diagnostics_controller.h"
#include "../../system/statistics_manager.h"
#include "../components/blocking_overlay.h"
#include "../components/ui_operations.h"
#include "../event_bridge_lvgl.h"
#include "../ui_helpers.h"
#include "../ui_manager.h"
#include "../screens/menu_screen.h"

MenuUIController::MenuUIController(UIManager* manager)
    : ui_manager_(manager) {}

void MenuUIController::register_events() {
    if (!ui_manager_) {
        return;
    }

    using ET = EventBridgeLVGL::EventType;

    EventBridgeLVGL::register_handler(ET::MENU_CALIBRATE, [this](lv_event_t*) { handle_calibrate(); });
    EventBridgeLVGL::register_handler(ET::MENU_RESET, [this](lv_event_t*) { handle_reset(); });
    EventBridgeLVGL::register_handler(ET::MENU_PURGE, [this](lv_event_t*) { handle_purge(); });
    EventBridgeLVGL::register_handler(ET::MENU_MOTOR_TEST, [this](lv_event_t*) { handle_motor_test(); });
    EventBridgeLVGL::register_handler(ET::MENU_SCALE_OPEN, [this](lv_event_t*) { handle_scale_open(); });
    EventBridgeLVGL::register_handler(ET::MENU_SCALE_TARE, [this](lv_event_t*) { handle_scale_tare(); });
    EventBridgeLVGL::register_handler(ET::MENU_AUTOTUNE, [this](lv_event_t*) { handle_autotune(); });
    EventBridgeLVGL::register_handler(ET::MENU_DIAGNOSTIC_RESET, [this](lv_event_t*) { handle_diagnostics_reset(); });
    EventBridgeLVGL::register_handler(ET::MENU_BACK, [this](lv_event_t*) { handle_back(); });
    EventBridgeLVGL::register_handler(ET::MENU_REFRESH_STATS, [this](lv_event_t*) { handle_refresh_stats(); });

    EventBridgeLVGL::register_handler(ET::BLE_TOGGLE, [this](lv_event_t*) { handle_ble_toggle(); });
    EventBridgeLVGL::register_handler(ET::BLE_STARTUP_TOGGLE, [this](lv_event_t*) { handle_ble_startup_toggle(); });
    EventBridgeLVGL::register_handler(ET::LOGGING_TOGGLE, [this](lv_event_t*) { handle_logging_toggle(); });

    EventBridgeLVGL::register_handler(ET::GRIND_MODE_SWIPE_TOGGLE, [this](lv_event_t*) { handle_grind_mode_swipe_toggle(); });
    EventBridgeLVGL::register_handler(ET::PROFILE_STYLE_SELECT_DRIP, [this](lv_event_t*) { handle_profile_style_select_drip(); });
    EventBridgeLVGL::register_handler(ET::PROFILE_STYLE_SELECT_ESPRESSO, [this](lv_event_t*) { handle_profile_style_select_espresso(); });
    EventBridgeLVGL::register_handler(ET::GRIND_MODE_SELECT_WEIGHT_TIME, [this](lv_event_t*) { handle_grind_mode_select_weight_time(); });
    EventBridgeLVGL::register_handler(ET::GRIND_MODE_SELECT_TIME_ONLY, [this](lv_event_t*) { handle_grind_mode_select_time_only(); });
    EventBridgeLVGL::register_handler(ET::AUTO_START_TOGGLE, [this](lv_event_t*) { handle_auto_start_toggle(); });
    EventBridgeLVGL::register_handler(ET::AUTO_RETURN_TOGGLE, [this](lv_event_t*) { handle_auto_return_toggle(); });
    EventBridgeLVGL::register_handler(ET::GRINDER_PURGE_MODE_RADIO_BUTTON, [this](lv_event_t*) { handle_grinder_purge_mode_radio_button(); });
    EventBridgeLVGL::register_handler(ET::GRINDER_PURGE_AMOUNT_SLIDER, [this](lv_event_t*) { handle_grinder_purge_amount_slider(); });
    EventBridgeLVGL::register_handler(ET::GRINDER_PURGE_AMOUNT_SLIDER_RELEASED, [this](lv_event_t*) { handle_grinder_purge_amount_slider_released(); });
    EventBridgeLVGL::register_handler(ET::GRIND_FRESHNESS_HOURS_SLIDER, [this](lv_event_t*) { handle_grind_freshness_hours_slider(); });
    EventBridgeLVGL::register_handler(ET::GRIND_FRESHNESS_HOURS_SLIDER_RELEASED, [this](lv_event_t*) { handle_grind_freshness_hours_slider_released(); });

    EventBridgeLVGL::register_handler(ET::BRIGHTNESS_NORMAL_SLIDER, [this](lv_event_t*) { handle_brightness_normal_slider(); });
    EventBridgeLVGL::register_handler(ET::BRIGHTNESS_NORMAL_SLIDER_RELEASED, [this](lv_event_t*) { handle_brightness_normal_slider_released(); });
    EventBridgeLVGL::register_handler(ET::BRIGHTNESS_SCREENSAVER_SLIDER, [this](lv_event_t*) { handle_brightness_screensaver_slider(); });
    EventBridgeLVGL::register_handler(ET::BRIGHTNESS_SCREENSAVER_SLIDER_RELEASED, [this](lv_event_t*) { handle_brightness_screensaver_slider_released(); });
    EventBridgeLVGL::register_handler(ET::AUTO_DIM_TIMEOUT_SLIDER, [this](lv_event_t*) { handle_auto_dim_timeout_slider(); });
    EventBridgeLVGL::register_handler(ET::AUTO_DIM_TIMEOUT_SLIDER_RELEASED, [this](lv_event_t*) { handle_auto_dim_timeout_slider_released(); });

    // Note: Event registration for menu widgets is done in the page creation functions
    // (menu_screen.cpp) because the menu is created lazily and destroyed on hide.
    // Attempting to register events here would fail silently since widgets don't exist yet.
}

void MenuUIController::update() {
    if (!ui_manager_) {
        return;
    }

    WeightSensor* sensor = ui_manager_->hardware_manager->get_weight_sensor();
    unsigned long uptime_ms = millis();
    size_t free_heap = ESP.getFreeHeap();

    ui_manager_->menu_screen.update_info(sensor, uptime_ms, free_heap);
    ui_manager_->menu_screen.update_diagnostics(sensor);
    ui_manager_->menu_screen.update_ble_status();

    if (ui_manager_->menu_screen.is_scale_page_active()) {
        float display_weight = sensor ? sensor->get_display_weight() : 0.0f;
        ui_manager_->menu_screen.update_scale_weight(display_weight);
    }
}

void MenuUIController::handle_calibrate() {
    if (ui_manager_) {
        ui_manager_->switch_to_state(UIState::CALIBRATION);
    }
}

void MenuUIController::handle_reset() {
    if (!ui_manager_) return;

    ui_manager_->show_confirmation(
        "FACTORY RESET",
        "This will reset all settings to factory defaults:\n\n"
        "• Profile weights\n"
        "• Calibration data\n"
        "• Grind history\n"
        "• Lifetime statistics\n\n"
        "This action cannot be undone.",
        "RESET",
        lv_color_hex(THEME_COLOR_ERROR),
        [this]() { perform_factory_reset(); },
        "CANCEL",
        [this]() { return_to_menu(); }
    );
}

void MenuUIController::handle_purge() {
    if (!ui_manager_) return;

    ui_manager_->show_confirmation(
        "PURGE LOGS",
        "This will remove all saved grind log files from flash.\n"
        "Lifetime statistics will be preserved."
        "\n\n"
        "This action cannot be undone.",
        "PURGE LOGS",
        lv_color_hex(THEME_COLOR_ERROR),
        [this]() { execute_purge_operation(); },
        "CANCEL",
        [this]() { return_to_menu(); }
    );
}

void MenuUIController::handle_motor_test() {
    if (!ui_manager_) return;

    ui_manager_->show_confirmation(
        "MOTOR TEST",
        "Motor will be engaged for 1 second."
        "\n\n"
        "Make sure grinder is safe to run.",
        "RUN",
        lv_color_hex(THEME_COLOR_SUCCESS),
        [this]() { run_motor_test(); },
        "CANCEL",
        [this]() { return_to_menu(); }
    );
}

void MenuUIController::handle_scale_open() {
    if (!ui_manager_) return;
    auto* hardware = ui_manager_->get_hardware_manager();
    if (!hardware) return;

    ui_manager_->menu_screen.reset_scale_display();

    UIOperations::execute_tare(hardware, [this]() {
        if (!ui_manager_) return;
        ui_manager_->refresh_auto_action_settings();

        auto* sensor = ui_manager_->hardware_manager->get_weight_sensor();
        float weight = sensor ? sensor->get_display_weight() : 0.0f;
        if (ui_manager_->menu_screen.is_scale_page_active()) {
            ui_manager_->menu_screen.update_scale_weight(weight);
        }
    });
}

void MenuUIController::handle_scale_tare() {
    if (!ui_manager_) return;
    auto* hardware = ui_manager_->get_hardware_manager();
    if (!hardware) return;

    UIOperations::execute_tare(hardware, [this]() {
        if (!ui_manager_) return;
        ui_manager_->refresh_auto_action_settings();

        auto* sensor = ui_manager_->hardware_manager->get_weight_sensor();
        float weight = sensor ? sensor->get_display_weight() : 0.0f;
        if (ui_manager_->menu_screen.is_scale_page_active()) {
            ui_manager_->menu_screen.update_scale_weight(weight);
        }
    });
}

void MenuUIController::handle_autotune() {
    if (!ui_manager_) return;

    // Show confirmation screen with setup instructions
    auto autotune_controller = ui_manager_->autotune_controller_.get();
    if (autotune_controller) {
        ui_manager_->show_confirmation(
            "Auto-Tune Setup",
            "Before starting:\n\n"
            "- Beans loaded\n"
            "- Cup on scale\n\n"
            "Process takes ~1 min.",
            "START",
            lv_color_hex(THEME_COLOR_ACCENT),
            [autotune_controller]() { autotune_controller->confirm_and_begin(); },
            "CANCEL",
            [this]() { return_to_menu(); }
        );
    }
}

void MenuUIController::handle_back() {
    if (!ui_manager_) return;
    ui_manager_->switch_to_state(UIState::READY);
}

void MenuUIController::handle_refresh_stats() {
    if (!ui_manager_) return;
    ui_manager_->menu_screen.refresh_statistics();
}

void MenuUIController::handle_diagnostics_reset() {
    if (!ui_manager_) return;

    ui_manager_->show_confirmation(
        "Reset Diagnostics",
        "This will clear all active diagnostic warnings.\n\nContinue?",
        "RESET",
        lv_color_hex(THEME_COLOR_WARNING),
        [this]() { perform_diagnostics_reset(); },
        "CANCEL",
        [this]() { return_to_menu(); }
    );
}

void MenuUIController::perform_diagnostics_reset() {
    if (!ui_manager_) return;

    auto* diagnostics = ui_manager_->diagnostics_controller_.get();
    if (diagnostics) {
        diagnostics->reset_diagnostic(DiagnosticCode::LOAD_CELL_NOISY_SUSTAINED);
        diagnostics->reset_diagnostic(DiagnosticCode::MECHANICAL_INSTABILITY);
        diagnostics->reset_noise_tracking();
    }

    auto* grind_controller = ui_manager_->get_grind_controller();
    if (grind_controller) {
        grind_controller->reset_mechanical_anomaly_count();
    }

    auto* hardware = ui_manager_->get_hardware_manager();
    auto* sensor = hardware ? hardware->get_weight_sensor() : nullptr;
    if (sensor) {
        ui_manager_->menu_screen.update_diagnostics(sensor);
    }
}

void MenuUIController::handle_ble_toggle() {
    if (!ui_manager_ || !ui_manager_->bluetooth_manager) return;

    auto* ble = ui_manager_->bluetooth_manager;
    if (ble->is_enabled()) {
        ble->disable();
        LOG_DEBUG_PRINTLN("Bluetooth disabled by user");
        ui_manager_->menu_screen.update_ble_status();
        return;
    }

    auto completion = [this]() {
        ui_manager_->menu_screen.update_ble_status();
    };

    auto operation = [ble]() {
        ble->enable();
        LOG_DEBUG_PRINTLN("Bluetooth enabled by user (30 minute timeout)");
    };

    auto& overlay = BlockingOperationOverlay::getInstance();
    overlay.show_and_execute(BlockingOperation::BLE_ENABLING, operation, completion);
}

void MenuUIController::handle_ble_startup_toggle() {
    if (!ui_manager_) return;

    auto* toggle = ui_manager_->menu_screen.get_ble_startup_toggle();
    if (!toggle) return;

    bool startup_enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);

    Preferences prefs;
    prefs.begin("bluetooth", false);
    prefs.putBool("startup", startup_enabled);
    prefs.end();

    LOG_DEBUG_PRINTLN(startup_enabled ? "Bluetooth startup enabled" : "Bluetooth startup disabled");
}

void MenuUIController::handle_logging_toggle() {
    if (!ui_manager_) return;

    auto* toggle = ui_manager_->menu_screen.get_logging_toggle();
    if (!toggle) return;

    bool logging_enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);

    Preferences prefs;
    prefs.begin("logging", false);
    prefs.putBool("enabled", logging_enabled);
    prefs.end();

    LOG_DEBUG_PRINTLN(logging_enabled ? "Logging enabled" : "Logging disabled");
}

void MenuUIController::handle_grind_mode_swipe_toggle() {
    if (!ui_manager_) return;

    auto* toggle = ui_manager_->menu_screen.get_grind_mode_swipe_toggle();
    if (!toggle) return;

    bool swipe_enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);

    Preferences prefs;
    prefs.begin("swipe", false);
    prefs.putBool("enabled", swipe_enabled);
    prefs.end();

    LOG_DEBUG_PRINTLN(swipe_enabled ? "Grind mode swipe gestures enabled" : "Grind mode swipe gestures disabled");

    // Swipe is the only way to reach Time mode within Weight & Time style.
    // If it's being turned off while currently parked in Time mode, there
    // would be no way back to Weight mode - fall back to Weight automatically.
    if (!swipe_enabled && ui_manager_->profile_controller &&
        !ui_manager_->profile_controller->is_time_only_mode() &&
        ui_manager_->current_mode == GrindMode::TIME) {

        ui_manager_->current_mode = GrindMode::WEIGHT;
        ui_manager_->profile_controller->set_grind_mode(ui_manager_->current_mode);

        if (ui_manager_->ready_controller_) {
            ui_manager_->ready_controller_->refresh_profiles();
        }
        ui_manager_->edit_target = get_current_profile_target(*ui_manager_->profile_controller, ui_manager_->current_mode);
        if (ui_manager_->state_machine->is_state(UIState::EDIT)) {
            if (ui_manager_->edit_controller_) {
                ui_manager_->edit_controller_->update_display();
            }
        }

        ui_manager_->grinding_screen.set_mode(ui_manager_->current_mode);
        if (ui_manager_->state_machine->is_state(UIState::GRINDING) ||
            ui_manager_->state_machine->is_state(UIState::GRIND_COMPLETE)) {
            if (ui_manager_->grinding_controller_) {
                ui_manager_->grinding_controller_->update_grinding_targets();
            }
        }

        if (ui_manager_->grinding_controller_) {
            ui_manager_->grinding_controller_->update_grind_button_icon();
        }

        LOG_DEBUG_PRINTLN("Swipe disabled while in Time mode - falling back to Weight mode");
    }
}

void MenuUIController::handle_profile_style_select_drip() { apply_profile_style(ProfileStyle::DRIP); }
void MenuUIController::handle_profile_style_select_espresso() { apply_profile_style(ProfileStyle::ESPRESSO); }
void MenuUIController::handle_grind_mode_select_weight_time() { apply_grind_type(false); }
void MenuUIController::handle_grind_mode_select_time_only() { apply_grind_type(true); }

void MenuUIController::apply_profile_style(ProfileStyle style) {
    if (!ui_manager_ || !ui_manager_->profile_controller) return;
    if (style == ui_manager_->profile_controller->get_profile_style()) return;

    ui_manager_->profile_controller->set_profile_style(style);
    ui_manager_->current_tab = ui_manager_->profile_controller->get_current_profile();

    // Ready screen is hidden while the Menu is open (this handler only fires
    // from a menu button), so rebuilding its tabview here is invisible to
    // the user until they navigate back.
    ui_manager_->ready_screen.rebuild_for_style(ui_manager_->profile_controller);
    if (ui_manager_->ready_controller_) {
        ui_manager_->ready_controller_->register_tabview_events();
        ui_manager_->ready_controller_->refresh_profiles();
    }

    ui_manager_->menu_screen.sync_profile_style_buttons(style == ProfileStyle::ESPRESSO);

    LOG_DEBUG_PRINTLN(style == ProfileStyle::ESPRESSO ? "Profile style switched to Espresso" : "Profile style switched to Drip Coffee");
}

void MenuUIController::apply_grind_type(bool want_time_only) {
    if (!ui_manager_ || !ui_manager_->profile_controller) return;
    bool currently_time_only = ui_manager_->profile_controller->is_time_only_mode();
    if (want_time_only == currently_time_only) return;

    if (want_time_only) {
        ui_manager_->profile_controller->set_time_only_mode(true);
        ui_manager_->current_mode = GrindMode::TIME;
        LOG_DEBUG_PRINTLN("Time Only mode locked in");
    } else {
        auto* sensor = ui_manager_->get_hardware_manager() ? ui_manager_->get_hardware_manager()->get_weight_sensor() : nullptr;
        bool is_calibrated = sensor && sensor->is_calibrated();

        if (!is_calibrated) {
            // Buttons never get recolored here, so Time Only stays visually
            // active until calibration actually completes.
            if (ui_manager_->calibration_controller_) {
                ui_manager_->calibration_controller_->request_unlock_calibration();
            }
            LOG_DEBUG_PRINTLN("Weight & Time requested but uncalibrated - launching calibration wizard");
            ui_manager_->switch_to_state(UIState::CALIBRATION);
            return;
        }

        ui_manager_->profile_controller->set_time_only_mode(false);
        ui_manager_->current_mode = GrindMode::WEIGHT;
        LOG_DEBUG_PRINTLN("Weight & Time mode unlocked");
    }

    ui_manager_->menu_screen.sync_grind_type_buttons(want_time_only);

    if (ui_manager_->ready_controller_) {
        ui_manager_->ready_controller_->refresh_profiles();
    }
    ui_manager_->edit_target = get_current_profile_target(*ui_manager_->profile_controller, ui_manager_->current_mode);
    if (ui_manager_->state_machine->is_state(UIState::EDIT)) {
        if (ui_manager_->edit_controller_) {
            ui_manager_->edit_controller_->update_display();
        }
    }
}

void MenuUIController::handle_auto_start_toggle() {
    if (!ui_manager_) return;

    auto* toggle = ui_manager_->menu_screen.get_auto_start_toggle();
    if (!toggle) return;

    bool enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);

    Preferences prefs;
    prefs.begin("autogrind", false);
    prefs.putBool("auto_start", enabled);
    prefs.end();

    if (ui_manager_) {
        ui_manager_->refresh_auto_action_settings();
    }

    LOG_DEBUG_PRINTLN(enabled ? "Auto-start on cup enabled" : "Auto-start on cup disabled");
}

void MenuUIController::handle_auto_return_toggle() {
    if (!ui_manager_) return;

    auto* toggle = ui_manager_->menu_screen.get_auto_return_toggle();
    if (!toggle) return;

    bool enabled = lv_obj_has_state(toggle, LV_STATE_CHECKED);

    Preferences prefs;
    prefs.begin("autogrind", false);
    prefs.putBool("auto_return", enabled);
    prefs.end();

    if (ui_manager_) {
        ui_manager_->refresh_auto_action_settings();
    }

    LOG_DEBUG_PRINTLN(enabled ? "Auto return on cup removal enabled" : "Auto return on cup removal disabled");
}

void MenuUIController::handle_grinder_purge_mode_radio_button() {
    if (!ui_manager_) return;

    auto* radio_group = ui_manager_->menu_screen.get_grinder_purge_mode_radio_group();
    if (!radio_group) return;

    int selected_index = radio_button_group_get_selection(radio_group);

    auto* hardware = ui_manager_->get_hardware_manager();
    Preferences* prefs = hardware ? hardware->get_preferences() : nullptr;
    if (prefs) {
        prefs->putInt(GrindController::PREF_KEY_GRINDER_MODE, selected_index);
    }

    LOG_DEBUG_PRINTLN(selected_index == 0 ? "Grinder purge mode: Prime (keep coffee)" : "Grinder purge mode: Purge (discard grinds)");
}

void MenuUIController::handle_grinder_purge_amount_slider() {
    if (!ui_manager_) return;

    auto* slider = ui_manager_->menu_screen.get_grinder_purge_amount_slider();
    if (!slider) return;

    int slider_value = lv_slider_get_value(slider);
    float amount_g = slider_value / MenuScreen::kPurgeSliderScale;
    if (amount_g < GRIND_PURGE_AMOUNT_MIN_G) amount_g = GRIND_PURGE_AMOUNT_MIN_G;
    if (amount_g > GRIND_PURGE_AMOUNT_MAX_G) amount_g = GRIND_PURGE_AMOUNT_MAX_G;

    // Update the label via MenuScreen method
    ui_manager_->menu_screen.update_grinder_purge_amount_label(amount_g);
}

void MenuUIController::handle_grinder_purge_amount_slider_released() {
    if (!ui_manager_) return;

    auto* slider = ui_manager_->menu_screen.get_grinder_purge_amount_slider();
    if (!slider) return;

    int slider_value = lv_slider_get_value(slider);
    float amount_g = slider_value / MenuScreen::kPurgeSliderScale;
    if (amount_g < GRIND_PURGE_AMOUNT_MIN_G) {
        amount_g = GRIND_PURGE_AMOUNT_MIN_G;
        lv_slider_set_value(slider, static_cast<int>(GRIND_PURGE_AMOUNT_MIN_G * MenuScreen::kPurgeSliderScale + 0.5f), LV_ANIM_OFF);
    } else if (amount_g > GRIND_PURGE_AMOUNT_MAX_G) {
        amount_g = GRIND_PURGE_AMOUNT_MAX_G;
        lv_slider_set_value(slider, static_cast<int>(GRIND_PURGE_AMOUNT_MAX_G * MenuScreen::kPurgeSliderScale + 0.5f), LV_ANIM_OFF);
    }

    auto* hardware = ui_manager_->get_hardware_manager();
    Preferences* prefs = hardware ? hardware->get_preferences() : nullptr;
    if (prefs) {
        prefs->putFloat(GrindController::PREF_KEY_GRINDER_AMOUNT_G, amount_g);
    }

    LOG_DEBUG_PRINT("Grinder purge amount set to: ");
    LOG_DEBUG_PRINT(amount_g);
    LOG_DEBUG_PRINTLN("g");

    ui_manager_->menu_screen.update_grinder_purge_amount_label(amount_g);
}

void MenuUIController::handle_grind_freshness_hours_slider() {
    if (!ui_manager_) return;

    auto* slider = ui_manager_->menu_screen.get_grind_freshness_hours_slider();
    if (!slider) return;

    // Map slider value to hours (discrete steps: 0.5, 1, 2, 3, 4, 8, 12, 24, 48)
    static const float freshness_steps[] = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 8.0f, 12.0f, 24.0f, 48.0f};
    int slider_index = lv_slider_get_value(slider);
    if (slider_index < 0) slider_index = 0;
    if (slider_index > 8) slider_index = 8;
    float hours = freshness_steps[slider_index];

    // Update the label via MenuScreen method
    ui_manager_->menu_screen.update_grind_freshness_hours_label(hours);
}

void MenuUIController::handle_grind_freshness_hours_slider_released() {
    if (!ui_manager_) return;

    auto* slider = ui_manager_->menu_screen.get_grind_freshness_hours_slider();
    if (!slider) return;

    // Map slider value to hours (discrete steps: 0.5, 1, 2, 3, 4, 8, 12, 24, 48)
    static const float freshness_steps[] = {0.5f, 1.0f, 2.0f, 3.0f, 4.0f, 8.0f, 12.0f, 24.0f, 48.0f};
    int slider_index = lv_slider_get_value(slider);
    if (slider_index < 0) slider_index = 0;
    if (slider_index > 8) slider_index = 8;
    float hours = freshness_steps[slider_index];

    auto* hardware = ui_manager_->get_hardware_manager();
    Preferences* prefs = hardware ? hardware->get_preferences() : nullptr;
    if (prefs) {
        prefs->putFloat(GrindController::PREF_KEY_GRIND_FRESHNESS_HOURS, hours);
    }

    LOG_DEBUG_PRINT("Grind freshness hours set to: ");
    LOG_DEBUG_PRINT(hours);
    LOG_DEBUG_PRINTLN("h");

    ui_manager_->menu_screen.update_grind_freshness_hours_label(hours);
}

void MenuUIController::handle_brightness_normal_slider() {
    if (!ui_manager_) return;

    auto* slider = ui_manager_->menu_screen.get_brightness_normal_slider();
    if (!slider) return;

    int brightness_percent = lv_slider_get_value(slider);
    if (brightness_percent < HW_DISPLAY_MINIMAL_BRIGHTNESS_PERCENT) {
        brightness_percent = HW_DISPLAY_MINIMAL_BRIGHTNESS_PERCENT;
        lv_slider_set_value(slider, brightness_percent, LV_ANIM_OFF);
    }
    float brightness = brightness_percent / 100.0f;

    ui_manager_->get_hardware_manager()->get_display()->set_brightness(brightness);
    ui_manager_->menu_screen.update_brightness_labels(brightness_percent, -1);
    LOG_DEBUG_PRINTF("Normal brightness set to %d%% (%.2f)\n", brightness_percent, brightness);
}

void MenuUIController::handle_brightness_normal_slider_released() {
    auto* slider = ui_manager_->menu_screen.get_brightness_normal_slider();
    if (!slider) return;

    int brightness_percent = lv_slider_get_value(slider);
    if (brightness_percent < HW_DISPLAY_MINIMAL_BRIGHTNESS_PERCENT) {
        brightness_percent = HW_DISPLAY_MINIMAL_BRIGHTNESS_PERCENT;
        lv_slider_set_value(slider, brightness_percent, LV_ANIM_OFF);
    }
    float brightness = brightness_percent / 100.0f;

    Preferences prefs;
    prefs.begin("brightness", false);
    prefs.putFloat("normal", brightness);
    prefs.end();
}

void MenuUIController::handle_brightness_screensaver_slider() {
    if (!ui_manager_) return;

    auto* slider = ui_manager_->menu_screen.get_brightness_screensaver_slider();
    if (!slider) return;

    int brightness_percent = lv_slider_get_value(slider);
    if (brightness_percent < HW_DISPLAY_MINIMAL_DIMMED_BRIGHTNESS_PERCENT) {
        brightness_percent = HW_DISPLAY_MINIMAL_DIMMED_BRIGHTNESS_PERCENT;
        lv_slider_set_value(slider, brightness_percent, LV_ANIM_OFF);
    }
    float brightness = brightness_percent / 100.0f;

    ui_manager_->get_hardware_manager()->get_display()->set_brightness(brightness);
    ui_manager_->menu_screen.update_brightness_labels(-1, brightness_percent);
    LOG_DEBUG_PRINTF("Screensaver brightness set to %d%% (%.2f)\n", brightness_percent, brightness);
}

void MenuUIController::handle_brightness_screensaver_slider_released() {
    auto* slider = ui_manager_->menu_screen.get_brightness_screensaver_slider();
    if (!slider) return;

    int brightness_percent = lv_slider_get_value(slider);
    if (brightness_percent < HW_DISPLAY_MINIMAL_DIMMED_BRIGHTNESS_PERCENT) {
        brightness_percent = HW_DISPLAY_MINIMAL_DIMMED_BRIGHTNESS_PERCENT;
        lv_slider_set_value(slider, brightness_percent, LV_ANIM_OFF);
    }
    float brightness = brightness_percent / 100.0f;

    Preferences prefs;
    prefs.begin("brightness", false);
    prefs.putFloat("screensaver", brightness);
    prefs.end();

    float normal = get_normal_brightness();
    ui_manager_->get_hardware_manager()->get_display()->set_brightness(normal);
    LOG_DEBUG_PRINTF("Touch released - restored normal brightness to %.2f\n", normal);
}

void MenuUIController::handle_auto_dim_timeout_slider() {
    if (!ui_manager_) return;

    auto* slider = ui_manager_->menu_screen.get_auto_dim_timeout_slider();
    if (!slider) return;

    // Raw index 0-17 maps to 5-90 seconds in 5-second steps.
    int slider_index = std::clamp(static_cast<int>(lv_slider_get_value(slider)), 0, 17);
    int seconds = (slider_index + 1) * 5;

    ui_manager_->menu_screen.update_auto_dim_timeout_label(seconds);
}

void MenuUIController::handle_auto_dim_timeout_slider_released() {
    if (!ui_manager_) return;

    auto* slider = ui_manager_->menu_screen.get_auto_dim_timeout_slider();
    if (!slider) return;

    int slider_index = std::clamp(static_cast<int>(lv_slider_get_value(slider)), 0, 17);
    int seconds = (slider_index + 1) * 5;

    Preferences prefs;
    prefs.begin("brightness", false);
    prefs.putInt("autodim_sec", seconds);
    prefs.end();

    LOG_DEBUG_PRINTF("Auto-dim timeout set to %ds\n", seconds);

    ui_manager_->menu_screen.update_auto_dim_timeout_label(seconds);
}

void MenuUIController::perform_factory_reset() {
    if (!ui_manager_) return;

    LOG_DEBUG_PRINTLN("Factory reset: clearing NVS preferences and rebooting...");

    nvs_flash_deinit();
    esp_err_t erase_result = nvs_flash_erase();

    if (erase_result == ESP_OK) {
        LOG_DEBUG_PRINTLN("Factory reset: NVS erase successful. Restarting device...");
    } else {
        LOG_DEBUG_PRINTF("Factory reset: NVS erase failed (code %d). Forcing restart...\n",
                         static_cast<int>(erase_result));
    }

    delay(100);
    esp_restart();
}

void MenuUIController::execute_purge_operation() {
    if (!ui_manager_) return;

    auto completion = [this]() {
        return_to_menu();
        ui_manager_->menu_screen.refresh_statistics(false);
    };

    auto purge_task = []() {
        LOG_DEBUG_PRINTLN("\n=== PURGE GRIND LOGS INITIATED ===");
        extern GrindLogger grind_logger;
        bool success = grind_logger.clear_all_sessions_from_flash();
        if (success) {
            LOG_DEBUG_PRINTLN("Grind logs purged successfully - reinitializing logger...");
        } else {
            LOG_DEBUG_PRINTLN("ERROR: Failed to purge all grind log data!");
        }
    };

    auto& overlay = BlockingOperationOverlay::getInstance();
    overlay.show_and_execute(BlockingOperation::CUSTOM, purge_task, completion,
                             "PURGING LOGS...\nPlease wait");
}

void MenuUIController::run_motor_test() {
    if (!ui_manager_) return;

    auto* grinder = ui_manager_->get_hardware_manager()->get_grinder();
    if (!grinder) return;

    ui_manager_->set_background_active(true);
    grinder->start_pulse_rmt(1000);

    // Update statistics for motor test (1000ms = 1 second)
    statistics_manager.update_motor_test(1000);

    stop_motor_timer();
    motor_timer_ = lv_timer_create(static_motor_timer_cb, 2000, this);
    if (motor_timer_) {
        lv_timer_set_user_data(motor_timer_, this);
    }
}

void MenuUIController::return_to_menu() {
    if (!ui_manager_) return;
    ui_manager_->set_current_tab(3);
    ui_manager_->switch_to_state(UIState::MENU);
}

float MenuUIController::get_normal_brightness() const {
    if (!ui_manager_ || !ui_manager_->hardware_manager) {
        return USER_SCREEN_BRIGHTNESS_NORMAL;
    }

    Preferences prefs;
    prefs.begin("brightness", true);
    float brightness = prefs.getFloat("normal", USER_SCREEN_BRIGHTNESS_NORMAL);
    prefs.end();

    const float min_brightness = HW_DISPLAY_MINIMAL_BRIGHTNESS_PERCENT / 100.0f;
    if (brightness < min_brightness) {
        brightness = min_brightness;
    }
    return brightness;
}

float MenuUIController::get_screensaver_brightness() const {
    if (!ui_manager_ || !ui_manager_->hardware_manager) {
        return USER_SCREEN_BRIGHTNESS_DIMMED;
    }

    Preferences prefs;
    prefs.begin("brightness", true);
    float brightness = prefs.getFloat("screensaver", USER_SCREEN_BRIGHTNESS_DIMMED);
    prefs.end();

    const float min_brightness = HW_DISPLAY_MINIMAL_DIMMED_BRIGHTNESS_PERCENT / 100.0f;
    if (brightness < min_brightness) {
        brightness = min_brightness;
    }
    return brightness;
}

uint32_t MenuUIController::get_auto_dim_timeout_ms() const {
    if (!ui_manager_ || !ui_manager_->hardware_manager) {
        return USER_SCREEN_AUTO_DIM_TIMEOUT_MS;
    }

    Preferences prefs;
    prefs.begin("brightness", true);
    int seconds = prefs.getInt("autodim_sec", USER_SCREEN_AUTO_DIM_TIMEOUT_MS / 1000);
    prefs.end();

    seconds = std::clamp(seconds, 5, 90);
    return static_cast<uint32_t>(seconds) * 1000;
}

void MenuUIController::stop_motor_timer() {
    if (motor_timer_) {
        lv_timer_del(motor_timer_);
        motor_timer_ = nullptr;
    }
}

void MenuUIController::motor_timer_cb(lv_timer_t* timer) {
    if (!ui_manager_) {
        return;
    }

    auto* grinder = ui_manager_->get_hardware_manager()->get_grinder();
    if (grinder && !grinder->is_pulse_complete()) {
        grinder->stop();
    }

    stop_motor_timer();
    ui_manager_->set_background_active(false);
    return_to_menu();
}

void MenuUIController::static_motor_timer_cb(lv_timer_t* timer) {
    if (!timer) {
        return;
    }
    auto* controller = static_cast<MenuUIController*>(lv_timer_get_user_data(timer));
    if (controller) {
        controller->motor_timer_cb(timer);
    }
}
