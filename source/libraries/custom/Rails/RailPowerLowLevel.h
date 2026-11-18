#pragma once

enum class RailLevel
{
    LOW,
    HIGH
};

class RailPowerLowLevel
{
public:
    void init();
    void allowOutput();
    void lockOutput();
    void writeSignalLevel(RailLevel state);
    bool isOutputAllowed() const;

private:
    bool enabled_ = false;
};