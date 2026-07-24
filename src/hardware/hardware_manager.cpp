#include "hardware_manager.h"
#include "../controllers/grind_controller.h"
#include <Arduino.h>
#include "../config/constants.h"

void HardwareManager::init() {
    preferences.begin("grinder", false);
    display_manager.init();
    weight_sensor.init(&preferences);
    grinder.init(HW_MOTOR_RELAY_PIN);

    grind_controller = nullptr; // Will be set later

    initialized = true;
}

void HardwareManager::update() {
    if (!initialized) return;
    
    // All hardware components are updated independently by their own FreeRTOS
    // tasks (WeightSamplingTask, GrindControlTask, UI render task in TaskManager).
    // No need for grinding mode switching - load cell runs at constant high speed
}


