#pragma once
#include <lvgl.h>

// Abstract base class for grinding screen implementations
class IGrindingScreen {
public:
    virtual ~IGrindingScreen() = default;
    
    virtual void create() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void update_profile_name(const char* name) = 0;
    virtual void update_target_weight(float weight) = 0;
    virtual void update_target_weight_text(const char* text) = 0;
    virtual void update_current_weight(float weight) = 0;
    virtual void update_tare_display() = 0;
    virtual void update_progress(int percent) = 0;
    virtual bool is_visible() const = 0;
    virtual lv_obj_t* get_screen() const = 0;

    // Status caption shown below the progress indicator (e.g. "GRINDING...",
    // "DONE", or an error message), in the given color. Only implemented by
    // the arc screen - the chart screen already shows equivalent text inline
    // with the weight.
    virtual void set_status_text(const char* text, uint32_t color_hex) {}

    // Overrides the progress indicator's color (e.g. green on grind
    // complete). Only implemented by the arc screen; set_time_mode() resets
    // it back to the mode color on the next grind.
    virtual void set_progress_color(uint32_t color_hex) {}

    // Chart-specific method (only implemented by chart screen)
    virtual void add_chart_data_point(float current_weight, float flow_rate, uint32_t current_time_ms) {}
};

enum class GrindScreenLayout {
    MINIMAL_ARC,   // Original arc-based screen
    NERDY_CHART    // Chart-based screen
};