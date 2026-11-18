#pragma once

#include "RailPowerLowLevel.h"

enum class RailPowerState
{
    OFF,
    ON,
    SHORT_CIRCUIT
};

class RailPower
{
public:
    void init();
    void enable();
    void disable();
    void onShortCircuit();
    void resetFault();
    void applySignalLevel(RailLevel level);
    RailPowerState state() const;

private:
    RailPowerLowLevel lowLevel_;
    RailPowerState state_ = RailPowerState::OFF;
};