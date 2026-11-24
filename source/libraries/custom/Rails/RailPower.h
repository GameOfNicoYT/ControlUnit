#pragma once

#include "RailPowerLowLevel.h"

enum class RailPowerState
{
    OFF = 0x00,
    ON = 0x01,
    FAULT = 0xf0,
    SHORT_CIRCUIT = 0xff
};

class RailPower
{
public:
    void init();
    void enable();
    void disable();
    void onShortCircuit();
    void resetFault();
    void setFault();
    void applySignalLevel(RailLevel level);
    RailPowerState state() const;

private:
    void latchFault(RailPowerState fault);

    RailPowerLowLevel lowLevel_;
    RailPowerState state_ = RailPowerState::OFF;
};