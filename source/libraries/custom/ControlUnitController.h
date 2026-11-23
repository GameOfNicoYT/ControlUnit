#pragma once

#include <stdint.h>

#include "stm32l0xx_hal.h"

#include "Rails/CurrentSensors.h"
#include "Rails/RailPower.h"
#include "Rails/RailShortCircuitDetector.h"

struct ControlUnitConfig
{
    uint32_t trackShunt_mOhm;
    uint32_t supplyShunt_mOhm;
    uint32_t trackGain;
    uint32_t supplyGain;
    uint32_t adcReference_mV;
    uint32_t shortCircuitLimit_mA;
    uint8_t requiredSamples;
};

class ControlUnit
{
public:
    explicit ControlUnit(const ControlUnitConfig& config);

    HAL_StatusTypeDef init();
    void tick();
    void onTrackSampleTrigger();
    void Req_ResetFault();

private:
    RailPower rail_;
    CurrentSensor currentSensor_;
    RailShortCircuitDetector detector_;
};
