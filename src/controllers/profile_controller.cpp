#include "profile_controller.h"
#include <Arduino.h>
#include <string.h>
#include <Preferences.h>

void ProfileController::init(Preferences* prefs) {
    preferences = prefs;
    
    // Initialize default profiles
    strcpy(profiles[0].name, "2 CUPS");
    profiles[0].weight = USER_2CUP_WEIGHT_G;
    profiles[0].time_seconds = USER_2CUP_TIME_S;
    
    strcpy(profiles[1].name, "4 CUPS");
    profiles[1].weight = USER_4CUP_WEIGHT_G;
    profiles[1].time_seconds = USER_4CUP_TIME_S;
    
    strcpy(profiles[2].name, "6 CUPS");
    profiles[2].weight = USER_6CUP_WEIGHT_G;
    profiles[2].time_seconds = USER_6CUP_TIME_S;
    
    strcpy(profiles[3].name, "8 CUPS");
    profiles[3].weight = USER_8CUP_WEIGHT_G;
    profiles[3].time_seconds = USER_8CUP_TIME_S;
    
    strcpy(profiles[4].name, "10 CUPS");
    profiles[4].weight = USER_10CUP_WEIGHT_G;
    profiles[4].time_seconds = USER_10CUP_TIME_S;
    
    strcpy(profiles[5].name, "CUSTOM");
    profiles[5].weight = USER_CUSTOM_PROFILE_WEIGHT_G;
    profiles[5].time_seconds = USER_CUSTOM_PROFILE_TIME_S;
    
    // Initialize default grind mode
    current_grind_mode = GrindMode::WEIGHT;
    
    load_profiles();
}

void ProfileController::load_profiles() {
    current_profile = preferences->getInt("profile", 0);
    
    static const float default_weights[USER_PROFILE_COUNT] = {
        USER_2CUP_WEIGHT_G, USER_4CUP_WEIGHT_G, USER_6CUP_WEIGHT_G,
        USER_8CUP_WEIGHT_G, USER_10CUP_WEIGHT_G, USER_CUSTOM_PROFILE_WEIGHT_G
    };
    static const float default_times[USER_PROFILE_COUNT] = {
        USER_2CUP_TIME_S, USER_4CUP_TIME_S, USER_6CUP_TIME_S,
        USER_8CUP_TIME_S, USER_10CUP_TIME_S, USER_CUSTOM_PROFILE_TIME_S
    };

    // Reused NVS slots can carry stale values from an older profile scheme
    // (e.g. index 0 used to be a "SINGLE" espresso profile). When the
    // compiled-in defaults change version, reset all saved weights/times
    // back to the new defaults once; normal edits after that persist as usual.
    int stored_version = preferences->getInt("profile_ver", -1);
    bool reset_to_defaults = (stored_version != USER_PROFILE_DEFAULTS_VERSION);
    if (reset_to_defaults) {
        preferences->putInt("profile_ver", USER_PROFILE_DEFAULTS_VERSION);
    }

    for (int i = 0; i < USER_PROFILE_COUNT; i++) {
        char weight_key[8], time_key[8];
        snprintf(weight_key, sizeof(weight_key), "weight%d", i);
        snprintf(time_key, sizeof(time_key), "time%d", i);
        if (reset_to_defaults) {
            profiles[i].weight = default_weights[i];
            profiles[i].time_seconds = default_times[i];
            preferences->putFloat(weight_key, default_weights[i]);
            preferences->putFloat(time_key, default_times[i]);
        } else {
            profiles[i].weight = preferences->getFloat(weight_key, default_weights[i]);
            profiles[i].time_seconds = preferences->getFloat(time_key, default_times[i]);
        }
    }
    
    // Load grind mode (default to WEIGHT if not set)
    int stored_mode = preferences->getInt("grind_mode", static_cast<int>(GrindMode::WEIGHT));
    current_grind_mode = static_cast<GrindMode>(stored_mode);
    
    if (current_profile < 0 || current_profile >= USER_PROFILE_COUNT) {
        current_profile = 0;
    }
}

void ProfileController::save_profiles() {
    for (int i = 0; i < USER_PROFILE_COUNT; i++) {
        char weight_key[8], time_key[8];
        snprintf(weight_key, sizeof(weight_key), "weight%d", i);
        snprintf(time_key, sizeof(time_key), "time%d", i);
        preferences->putFloat(weight_key, profiles[i].weight);
        preferences->putFloat(time_key, profiles[i].time_seconds);
    }
}

void ProfileController::save_current_profile() {
    preferences->putInt("profile", current_profile);
    save_profiles();
}

void ProfileController::set_current_profile(int index) {
    if (index >= 0 && index < USER_PROFILE_COUNT) {
        current_profile = index;
        save_current_profile();
    }
}

void ProfileController::set_profile_weight(int index, float weight) {
    if (index >= 0 && index < USER_PROFILE_COUNT && is_weight_valid(weight)) {
        profiles[index].weight = weight;
        save_profiles();
    }
}

float ProfileController::get_profile_weight(int index) const {
    if (index >= 0 && index < USER_PROFILE_COUNT) {
        return profiles[index].weight;
    }
    return 0.0f;
}

const char* ProfileController::get_profile_name(int index) const {
    if (index >= 0 && index < USER_PROFILE_COUNT) {
        return profiles[index].name;
    }
    return "UNKNOWN";
}

void ProfileController::set_profile_time(int index, float seconds) {
    if (index >= 0 && index < USER_PROFILE_COUNT && is_time_valid(seconds)) {
        profiles[index].time_seconds = seconds;
        save_profiles();
    }
}

float ProfileController::get_profile_time(int index) const {
    if (index >= 0 && index < USER_PROFILE_COUNT) {
        return profiles[index].time_seconds;
    }
    return 0.0f;
}

bool ProfileController::is_weight_valid(float weight) const {
    return weight >= USER_MIN_TARGET_WEIGHT_G && weight <= USER_MAX_TARGET_WEIGHT_G;
}

float ProfileController::clamp_weight(float weight) const {
    if (weight < USER_MIN_TARGET_WEIGHT_G) return USER_MIN_TARGET_WEIGHT_G;
    if (weight > USER_MAX_TARGET_WEIGHT_G) return USER_MAX_TARGET_WEIGHT_G;
    return weight;
}

void ProfileController::update_current_weight(float weight) {
    if (is_weight_valid(weight)) {
        profiles[current_profile].weight = weight;
    }
}

void ProfileController::update_current_time(float seconds) {
    if (is_time_valid(seconds)) {
        profiles[current_profile].time_seconds = seconds;
    }
}

bool ProfileController::is_time_valid(float seconds) const {
    return seconds >= USER_MIN_TARGET_TIME_S && seconds <= USER_MAX_TARGET_TIME_S;
}

float ProfileController::clamp_time(float seconds) const {
    if (seconds < USER_MIN_TARGET_TIME_S) return USER_MIN_TARGET_TIME_S;
    if (seconds > USER_MAX_TARGET_TIME_S) return USER_MAX_TARGET_TIME_S;
    return seconds;
}

void ProfileController::set_grind_mode(GrindMode mode) {
    current_grind_mode = mode;
    save_grind_mode();
}

void ProfileController::save_grind_mode() {
    preferences->putInt("grind_mode", static_cast<int>(current_grind_mode));
}
