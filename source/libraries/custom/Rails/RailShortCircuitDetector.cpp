#include "RailShortCircuitDetector.h"

RailShortCircuitDetector::RailShortCircuitDetector(
    uint32_t limit_mA,
    uint8_t requiredSamples) : trackCurrentLimit_mA_(limit_mA), requiredSamples_(requiredSamples > 0 ? requiredSamples : 1)
{
    overLimitCount_ = 0;
};

bool RailShortCircuitDetector::check(uint32_t trackCurrent_mA, bool outputHigh)
{
    if (!outputHigh)
        return false;

    if (trackCurrent_mA > trackCurrentLimit_mA_)
    {
        if (overLimitCount_ < requiredSamples_)
        {
            overLimitCount_++;
        }
    }
    else
    {
        overLimitCount_ = 0;
    }

    if (overLimitCount_ >= requiredSamples_)
    {
        return true;
    }
    return false;
}

void RailShortCircuitDetector::reset()
{
    overLimitCount_ = 0;
}