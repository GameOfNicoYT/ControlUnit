#include "RailShortCircuitDetector.h"

RailShortCircuitDetector::RailShortCircuitDetector(
    uint32_t limit_mA,
    uint8_t requiredSamples) : trackCurrentLimit_mA_(limit_mA), requiredSamples_(requiredSamples > 0 ? requiredSamples : 1)
{
    overLimitCount_ = 0;
};

HAL_StatusTypeDef RailShortCircuitDetector::check(uint32_t trackCurrent_mA, bool outputHigh)
{
    if (!outputHigh)
        return HAL_OK;

    if (trackCurrent_mA <= trackCurrentLimit_mA_)
    {
        overLimitCount_ = 0;
        return HAL_OK;
    }

    if (overLimitCount_ < requiredSamples_)
        ++overLimitCount_;

    return overLimitCount_ >= requiredSamples_ ? HAL_ERROR : HAL_OK;
}

void RailShortCircuitDetector::reset()
{
    overLimitCount_ = 0;
}
