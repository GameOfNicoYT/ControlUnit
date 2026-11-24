#pragma once

#include "RailPowerLowLevel.h"

enum class RailPowerState
{
    OFF = 0x00,
    ON = 0x01,
    FAULT = 0xf0,
    COMMUNICATION_TIMEOUT = 0xf1,
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
    void onCommunicationTimeout();
    void applySignalLevel(RailLevel level);
    RailPowerState state() const;

private:
    void latchFault(RailPowerState fault);

    RailPowerLowLevel lowLevel_;
    volatile RailPowerState state_ = RailPowerState::OFF;
};