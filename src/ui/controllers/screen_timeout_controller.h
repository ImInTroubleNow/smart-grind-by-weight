#pragma once

#include <cstdint>

class UIManager;

// Implements automatic screen dimming based on touch/weight activity

class ScreenTimeoutController {
public:
    explicit ScreenTimeoutController(UIManager* manager);

    void register_events();
    void update();

private:
    UIManager* ui_manager_;
    bool screen_dimmed_;

    bool fading_;
    uint32_t fade_start_ms_;
    float fade_from_;
    float fade_to_;

    static constexpr uint32_t kFadeDurationMs = 600;
};
