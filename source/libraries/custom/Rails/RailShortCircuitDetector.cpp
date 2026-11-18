#include "RailShortCircuitDetector.h"

RailShortCircuitDetector::RailShortCircuitDetector(
    uint32_t limit_mA,
    uint8_t requiredSamples) : trackCurrentLimit_mA_(limit_mA), requiredSamples_(requiredSamples) {

                               };

bool RailShortCircuitDetector::check(uint32_t trackCurrent_mA, bool outputHigh)
{
    
}

void RailShortCircuitDetector::reset()
{
}