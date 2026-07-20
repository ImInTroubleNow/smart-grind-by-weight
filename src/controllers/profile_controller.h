#pragma once
#include <Preferences.h>
#include "../config/constants.h"
#include "grind_mode.h"
#include "profile_style.h"

struct Profile {
    char name[USER_PROFILE_NAME_MAX_LENGTH];
    float weight;
    float time_seconds;
};

class ProfileController {
private:
    Profile drip_profiles[USER_PROFILE_COUNT];
    Profile espresso_profiles[USER_ESPRESSO_PROFILE_COUNT];
    ProfileStyle current_style;
    int current_profile;              // index into whichever array is active
    int drip_current_profile;         // remembered tab within Drip across style switches
    int espresso_current_profile;     // remembered tab within Espresso across style switches
    GrindMode current_grind_mode;
    bool time_only_mode;
    Preferences* preferences;

    Profile* active_profiles() { return current_style == ProfileStyle::ESPRESSO ? espresso_profiles : drip_profiles; }
    const Profile* active_profiles() const { return current_style == ProfileStyle::ESPRESSO ? espresso_profiles : drip_profiles; }

public:
    void init(Preferences* prefs);
    void load_profiles();
    void save_profiles();
    void save_current_profile();

    void set_current_profile(int index);
    int get_current_profile() const { return current_profile; }
    float get_current_weight() const { return active_profiles()[current_profile].weight; }
    float get_current_time() const { return active_profiles()[current_profile].time_seconds; }
    const char* get_current_name() const { return active_profiles()[current_profile].name; }

    void set_profile_weight(int index, float weight);
    float get_profile_weight(int index) const;
    const char* get_profile_name(int index) const;
    void set_profile_time(int index, float seconds);
    float get_profile_time(int index) const;

    void update_current_weight(float weight);
    void update_current_time(float seconds);

    // Weight validation methods - single authority for all weight constraints
    bool is_weight_valid(float weight) const;
    float clamp_weight(float weight) const;

    bool is_time_valid(float seconds) const;
    float clamp_time(float seconds) const;

    // Grind mode persistence methods
    void set_grind_mode(GrindMode mode);
    GrindMode get_grind_mode() const { return current_grind_mode; }
    void save_grind_mode();

    // Time Only mode: locks grinding to TIME and skips load-cell calibration entirely
    bool is_time_only_mode() const { return time_only_mode; }
    void set_time_only_mode(bool enabled);

    // Profile style: Drip Coffee (6 profiles) or Espresso (3 profiles)
    int get_profile_count() const { return current_style == ProfileStyle::ESPRESSO ? USER_ESPRESSO_PROFILE_COUNT : USER_PROFILE_COUNT; }
    ProfileStyle get_profile_style() const { return current_style; }
    void set_profile_style(ProfileStyle style);
};
