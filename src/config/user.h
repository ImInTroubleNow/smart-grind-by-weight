#pragma once

//==============================================================================
// USER CONFIGURATION PARAMETERS
//==============================================================================
// This file contains user-configurable parameters that affect coffee grinding
// behavior, UI responsiveness, and device operation. These are the primary
// settings that users might want to modify to customize their grinder.

//------------------------------------------------------------------------------
// COFFEE PROFILES
//------------------------------------------------------------------------------
#define USER_PROFILE_COUNT 6                                                    // Number of coffee profiles available
#define USER_PROFILE_NAME_MAX_LENGTH 10                                          // Maximum characters in profile name

// Default target weights for each profile
#define USER_2CUP_WEIGHT_G 14.0f                                                // 2-cup default weight
#define USER_4CUP_WEIGHT_G 29.0f                                                // 4-cup default weight
#define USER_6CUP_WEIGHT_G 43.0f                                                // 6-cup default weight
#define USER_8CUP_WEIGHT_G 58.0f                                                // 8-cup default weight
#define USER_10CUP_WEIGHT_G 72.0f                                               // 10-cup default weight
#define USER_CUSTOM_PROFILE_WEIGHT_G 21.5f                                      // Custom profile default weight

#define USER_2CUP_TIME_S 8.0f                                                   // 2-cup default grind time
#define USER_4CUP_TIME_S 16.0f                                                  // 4-cup default grind time
#define USER_6CUP_TIME_S 24.0f                                                  // 6-cup default grind time
#define USER_8CUP_TIME_S 32.0f                                                  // 8-cup default grind time
#define USER_10CUP_TIME_S 40.0f                                                 // 10-cup default grind time
#define USER_CUSTOM_PROFILE_TIME_S 12.0f                                        // Custom profile default grind time

// Weight limits
#define USER_MIN_TARGET_WEIGHT_G 5.0f                                           // Minimum allowed target weight
#define USER_MAX_TARGET_WEIGHT_G 1000.0f                                        // Maximum allowed target weight

#define USER_MIN_TARGET_TIME_S 0.5f                                             // Minimum allowed target time
#define USER_MAX_TARGET_TIME_S 60.0f                                           // Maximum allowed target time

//------------------------------------------------------------------------------
// WEIGHT/TIME ADJUSTMENTS
//------------------------------------------------------------------------------
#define USER_FINE_WEIGHT_ADJUSTMENT_G 0.1f                                     // Small weight increment for fine tuning
#define USER_FINE_TIME_ADJUSTMENT_S 0.1f                                       // Fine adjustment step for time editing

// USER_JOG parameters moved to system.h to be near SYS_JOG parameters

//------------------------------------------------------------------------------
// SCALE CALIBRATION
//------------------------------------------------------------------------------
#define USER_CALIBRATION_REFERENCE_WEIGHT_G 100.0f                             // Default reference weight for calibration
#define USER_DEFAULT_CALIBRATION_FACTOR -7050.0f                               // Default load cell calibration factor

//------------------------------------------------------------------------------
// SCREEN AUTO-DIMMING
//------------------------------------------------------------------------------
#define USER_SCREEN_AUTO_DIM_TIMEOUT_MS 300000                                 // Time before screen dims due to inactivity
#define USER_SCREEN_BRIGHTNESS_NORMAL 1.0f                                     // Normal screen brightness
#define USER_SCREEN_BRIGHTNESS_DIMMED 0.35f                                    // Dimmed screen brightness
#define USER_WEIGHT_ACTIVITY_THRESHOLD_G 1.0f                                  // Weight change threshold for screen timeout reset (grams)

//------------------------------------------------------------------------------
// AUTO ACTIONS
//------------------------------------------------------------------------------
#define USER_AUTO_GRIND_TRIGGER_DELTA_G 50.0f                                   // Weight change threshold used for auto actions (grams)
#define USER_AUTO_GRIND_TRIGGER_WINDOW_MS 5000                                  // Time window for delta detection (milliseconds)
#define USER_AUTO_GRIND_TRIGGER_SETTLING_MS 1000                                // Settling period after trigger detection before confirmation (milliseconds)
#define USER_AUTO_GRIND_REARM_DELAY_MS 1500                                     // Minimum delay between auto actions (milliseconds)
