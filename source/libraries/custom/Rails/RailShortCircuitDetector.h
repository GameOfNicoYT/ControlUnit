#pragma once
#include <stdint.h>

class RailShortCircuitDetector
{
public:
    RailShortCircuitDetector(uint32_t limit_mA, uint8_t requiredSamples);
    bool check(uint32_t trackCurrent_mA, bool outputHigh);
    void reset();

private:
    uint32_t trackCurrentLimit_mA_;
    uint8_t requiredSamples_;
    uint8_t overLimitCount_;
};