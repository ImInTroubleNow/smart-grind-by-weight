#pragma once
#include "blocking_overlay.h"
#include "../../hardware/hardware_manager.h"

class UIOperations {
public:
    // Unified tare operation for any screen
    static void execute_tare(HardwareManager* hw_manager, OperationCallback completion = nullptr);
    
    // Unified calibration operation
    static void execute_calibration(HardwareManager* hw_manager, float cal_weight,
                                   OperationCallback completion = nullptr);
};
