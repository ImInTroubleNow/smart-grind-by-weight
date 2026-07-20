#include "profile_controller.h"
#include <Arduino.h>
#include <string.h>
#include <Preferences.h>

void ProfileController::init(Preferences* prefs) {
    preferences = prefs;

    // Initialize default Drip Coffee profiles
    strcpy(drip_profiles[0].name, "2 CUPS");
    drip_profiles[0].weight = USER_2CUP_WEIGHT_G;
    drip_profiles[0].time_seconds = USER_2CUP_TIME_S;

    strcpy(drip_profiles[1].name, "4 CUPS");
    drip_profiles[1].weight = USER_4CUP_WEIGHT_G;
    drip_profiles[1].time_seconds = USER_4CUP_TIME_S;

    strcpy(drip_profiles[2].name, "6 CUPS");
    drip_profiles[2].weight = USER_6CUP_WEIGHT_G;
    drip_profiles[2].time_seconds = USER_6CUP_TIME_S;

    strcpy(drip_profiles[3].name, "8 CUPS");
    drip_profiles[3].weight = USER_8CUP_WEIGHT_G;
    drip_profiles[3].time_seconds = USER_8CUP_TIME_S;

    strcpy(drip_profiles[4].name, "10 CUPS");
    drip_profiles[4].weight = USER_10CUP_WEIGHT_G;
    drip_profiles[4].time_seconds = USER_10CUP_TIME_S;

    strcpy(drip_profiles[5].name, "CUSTOM");
    drip_profiles[5].weight = USER_CUSTOM_PROFILE_WEIGHT_G;
    drip_profiles[5].time_seconds = USER_CUSTOM_PROFILE_TIME_S;

    // Initialize default Espresso profiles
    strcpy(espresso_profiles[0].name, "SINGLE");
    espresso_profiles[0].weight = USER_SINGLE_ESPRESSO_WEIGHT_G;
    espresso_profiles[0].time_seconds = USER_SINGLE_ESPRESSO_TIME_S;

    strcpy(espresso_profiles[1].name, "DOUBLE");
    espresso_profiles[1].weight = USER_DOUBLE_ESPRESSO_WEIGHT_G;
    espresso_profiles[1].time_seconds = USER_DOUBLE_ESPRESSO_TIME_S;

    strcpy(espresso_profiles[2].name, "CUSTOM");
    espresso_profiles[2].weight = USER_ESPRESSO_CUSTOM_WEIGHT_G;
    espresso_profiles[2].time_seconds = USER_ESPRESSO_CUSTOM_TIME_S;

    // Initialize default grind mode
    current_grind_mode = GrindMode::WEIGHT;
    time_only_mode = false;
    current_style = ProfileStyle::DRIP;
    drip_current_profile = 0;
    espresso_current_profile = 0;

    load_profiles();
}

void ProfileController::load_profiles() {
    drip_current_profile = preferences->getInt("profile", 0);
    espresso_current_profile = preferences->getInt("e_profile", 0);

    static const float drip_default_weights[USER_PROFILE_COUNT] = {
        USER_2CUP_WEIGHT_G, USER_4CUP_WEIGHT_G, USER_6CUP_WEIGHT_G,
        USER_8CUP_WEIGHT_G, USER_10CUP_WEIGHT_G, USER_CUSTOM_PROFILE_WEIGHT_G
    };
    static const float drip_default_times[USER_PROFILE_COUNT] = {
        USER_2CUP_TIME_S, USER_4CUP_TIME_S, USER_6CUP_TIME_S,
        USER_8CUP_TIME_S, USER_10CUP_TIME_S, USER_CUSTOM_PROFILE_TIME_S
    };

    // Reused NVS slots can carry stale values from an older profile scheme
    // (e.g. index 0 used to be a "SINGLE" espresso profile). When the
    // compiled-in defaults change version, reset all saved weights/times
    // back to the new defaults once; normal edits after that persist as usual.
    int drip_stored_version = preferences->getInt("profile_ver", -1);
    bool drip_reset_to_defaults = (drip_stored_version != USER_PROFILE_DEFAULTS_VERSION);
    if (drip_reset_to_defaults) {
        preferences->putInt("profile_ver", USER_PROFILE_DEFAULTS_VERSION);
    }

    for (int i = 0; i < USER_PROFILE_COUNT; i++) {
        char weight_key[8], time_key[8];
        snprintf(weight_key, sizeof(weight_key), "weight%d", i);
        snprintf(time_key, sizeof(time_key), "time%d", i);
        if (drip_reset_to_defaults) {
            drip_profiles[i].weight = drip_default_weights[i];
            drip_profiles[i].time_seconds = drip_default_times[i];
            preferences->putFloat(weight_key, drip_default_weights[i]);
            preferences->putFloat(time_key, drip_default_times[i]);
        } else {
            drip_profiles[i].weight = preferences->getFloat(weight_key, drip_default_weights[i]);
            drip_profiles[i].time_seconds = preferences->getFloat(time_key, drip_default_times[i]);
        }
    }

    // Same version-gated reset, independently, for the Espresso profile set.
    static const float espresso_default_weights[USER_ESPRESSO_PROFILE_COUNT] = {
        USER_SINGLE_ESPRESSO_WEIGHT_G, USER_DOUBLE_ESPRESSO_WEIGHT_G, USER_ESPRESSO_CUSTOM_WEIGHT_G
    };
    static const float espresso_default_times[USER_ESPRESSO_PROFILE_COUNT] = {
        USER_SINGLE_ESPRESSO_TIME_S, USER_DOUBLE_ESPRESSO_TIME_S, USER_ESPRESSO_CUSTOM_TIME_S
    };

    int espresso_stored_version = preferences->getInt("e_profile_ver", -1);
    bool espresso_reset_to_defaults = (espresso_stored_version != USER_ESPRESSO_PROFILE_DEFAULTS_VERSION);
    if (espresso_reset_to_defaults) {
        preferences->putInt("e_profile_ver", USER_ESPRESSO_PROFILE_DEFAULTS_VERSION);
    }

    for (int i = 0; i < USER_ESPRESSO_PROFILE_COUNT; i++) {
        char weight_key[10], time_key[10];
        snprintf(weight_key, sizeof(weight_key), "e_weight%d", i);
        snprintf(time_key, sizeof(time_key), "e_time%d", i);
        if (espresso_reset_to_defaults) {
            espresso_profiles[i].weight = espresso_default_weights[i];
            espresso_profiles[i].time_seconds = espresso_default_times[i];
            preferences->putFloat(weight_key, espresso_default_weights[i]);
            preferences->putFloat(time_key, espresso_default_times[i]);
        } else {
            espresso_profiles[i].weight = preferences->getFloat(weight_key, espresso_default_weights[i]);
            espresso_profiles[i].time_seconds = preferences->getFloat(time_key, espresso_default_times[i]);
        }
    }

    // Load grind mode (default to WEIGHT if not set)
    int stored_mode = preferences->getInt("grind_mode", static_cast<int>(GrindMode::WEIGHT));
    current_grind_mode = static_cast<GrindMode>(stored_mode);

    // Load Time Only lock (default to false/unlocked if not set)
    time_only_mode = preferences->getBool("time_only_mode", false);

    // Load active profile style (default to Drip Coffee if not set)
    int stored_style = preferences->getInt("profile_style", static_cast<int>(ProfileStyle::DRIP));
    current_style = static_cast<ProfileStyle>(stored_style);

    if (drip_current_profile < 0 || drip_current_profile >= USER_PROFILE_COUNT) {
        drip_current_profile = 0;
    }
    if (espresso_current_profile < 0 || espresso_current_profile >= USER_ESPRESSO_PROFILE_COUNT) {
        espresso_current_profile = 0;
    }
    current_profile = (current_style == ProfileStyle::ESPRESSO) ? espresso_current_profile : drip_current_profile;
}

void ProfileController::save_profiles() {
    // Only the active style's slots are written, so switching styles never
    // clobbers the other style's independently-persisted values.
    if (current_style == ProfileStyle::ESPRESSO) {
        for (int i = 0; i < USER_ESPRESSO_PROFILE_COUNT; i++) {
            char weight_key[10], time_key[10];
            snprintf(weight_key, sizeof(weight_key), "e_weight%d", i);
            snprintf(time_key, sizeof(time_key), "e_time%d", i);
            preferences->putFloat(weight_key, espresso_profiles[i].weight);
            preferences->putFloat(time_key, espresso_profiles[i].time_seconds);
        }
    } else {
        for (int i = 0; i < USER_PROFILE_COUNT; i++) {
            char weight_key[8], time_key[8];
            snprintf(weight_key, sizeof(weight_key), "weight%d", i);
            snprintf(time_key, sizeof(time_key), "time%d", i);
            preferences->putFloat(weight_key, drip_profiles[i].weight);
            preferences->putFloat(time_key, drip_profiles[i].time_seconds);
        }
    }
}

void ProfileController::save_current_profile() {
    if (current_style == ProfileStyle::ESPRESSO) {
        preferences->putInt("e_profile", current_profile);
    } else {
        preferences->putInt("profile", current_profile);
    }
    save_profiles();
}

void ProfileController::set_current_profile(int index) {
    if (index >= 0 && index < get_profile_count()) {
        current_profile = index;
        if (current_style == ProfileStyle::ESPRESSO) {
            espresso_current_profile = index;
        } else {
            drip_current_profile = index;
        }
        save_current_profile();
    }
}

void ProfileController::set_profile_weight(int index, float weight) {
    if (index >= 0 && index < get_profile_count() && is_weight_valid(weight)) {
        active_profiles()[index].weight = weight;
        save_profiles();
    }
}

float ProfileController::get_profile_weight(int index) const {
    if (index >= 0 && index < get_profile_count()) {
        return active_profiles()[index].weight;
    }
    return 0.0f;
}

const char* ProfileController::get_profile_name(int index) const {
    if (index >= 0 && index < get_profile_count()) {
        return active_profiles()[index].name;
    }
    return "UNKNOWN";
}

void ProfileController::set_profile_time(int index, float seconds) {
    if (index >= 0 && index < get_profile_count() && is_time_valid(seconds)) {
        active_profiles()[index].time_seconds = seconds;
        save_profiles();
    }
}

float ProfileController::get_profile_time(int index) const {
    if (index >= 0 && index < get_profile_count()) {
        return active_profiles()[index].time_seconds;
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
        active_profiles()[current_profile].weight = weight;
    }
}

void ProfileController::update_current_time(float seconds) {
    if (is_time_valid(seconds)) {
        active_profiles()[current_profile].time_seconds = seconds;
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

void ProfileController::set_time_only_mode(bool enabled) {
    time_only_mode = enabled;
    preferences->putBool("time_only_mode", enabled);

    // Locking in Time Only always forces TIME; unlocking always returns to WEIGHT
    // as the sane default starting mode.
    set_grind_mode(enabled ? GrindMode::TIME : GrindMode::WEIGHT);
}

void ProfileController::set_profile_style(ProfileStyle style) {
    if (style == current_style) return;
    current_style = style;
    preferences->putInt("profile_style", static_cast<int>(style));
    current_profile = (style == ProfileStyle::ESPRESSO) ? espresso_current_profile : drip_current_profile;
}
