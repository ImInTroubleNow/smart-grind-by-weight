#pragma once

//==============================================================================
// USER INTERFACE THEME CONFIGURATION
//==============================================================================
// This file contains all user interface theming and visual design constants
// for the LVGL-based touchscreen interface. These values define the visual
// appearance, colors, dimensions, and styling of all UI elements.

//------------------------------------------------------------------------------
// COLOR SCHEME (RGB565 Format)
//------------------------------------------------------------------------------
// Primary brand colors
#define THEME_COLOR_PRIMARY 0xFF3D00                                           // Primary theme color (red)
#define THEME_COLOR_ACCENT 0x3FA9E8                                            // Accent color for highlights (blue)
#define THEME_COLOR_STOP 0xC43A21                                              // Grind stop button (red)
#define THEME_COLOR_ARC_WEIGHT 0xD97A2E                                        // Weight mode color - progress arc fill and grind start button (orange)
#define THEME_COLOR_ARC_TIME 0x2E8FC4                                          // Time mode color - progress arc fill and grind start button (blue)
#define THEME_COLOR_GRIND_COMPLETE 0x2EB45C                                    // Grind complete state - arc, button, and status text (green)
#define THEME_COLOR_SECONDARY 0xAAAAAA                                         // Secondary theme color (light gray)

// Main menu section icon colors
#define THEME_COLOR_MENU_GENERAL 0x00E5FF                                      // General group icon color (cyan)
#define THEME_COLOR_MENU_CALIBRATION 0xFFB300                                  // Calibration group icon color (amber)
#define THEME_COLOR_MENU_SETTINGS 0xA855F7                                     // Settings group icon color (purple)
#define THEME_COLOR_MENU_SYSTEM 0x10B981                                       // System group icon color (emerald)

// Text colors
#define THEME_COLOR_TEXT_PRIMARY 0xFFFFFF                                      // Primary text color (white)
#define THEME_COLOR_TEXT_SECONDARY 0xCCCCCC                                    // Secondary text color (light gray)

// Background colors
#define THEME_COLOR_BACKGROUND 0x000000                                        // Background color (black)
#define THEME_COLOR_NEUTRAL 0x666666                                           // Neutral color (dark gray)
#define THEME_COLOR_BACKGROUND_MOCK 0x035e03                                   // Background color when mock hardware is active (dark green)

// Status indication colors
#define THEME_COLOR_SUCCESS 0x00AA00                                           // Success state color (green)
#define THEME_COLOR_ERROR 0xFF0000                                             // Error state color (red)
#define THEME_COLOR_WARNING 0xCC8800                                           // Warning state color (darker yellow/orange)
#define THEME_COLOR_GRINDER_ACTIVE 0x403800                                    // Grinder active indicator (dark yellow)

//------------------------------------------------------------------------------
// UI ELEMENT DIMENSIONS
//------------------------------------------------------------------------------
// Button specifications
#define THEME_BUTTON_WIDTH_PX 120                                             // Standard button width

// Progress and feedback elements
#define THEME_PROGRESS_ARC_DIAMETER_PX 200                                    // Progress arc diameter (OTA update and grinding screens)

// General layout
#define THEME_CORNER_RADIUS_PX 20                                             // Standard UI element corner radius

//------------------------------------------------------------------------------
// OPACITY VALUES
//------------------------------------------------------------------------------
#define THEME_OPACITY_OVERLAY 204                                             // Overlay background opacity (80% of 255)