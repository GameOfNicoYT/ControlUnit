#include "SignalGenerator.h"

namespace
{
    enum class PacketSlot : uint8_t
    {
        CAR0,
        CAR1,
        CAR2,
        CAR3,
        CAR4,
        CAR5,
        PRG,
        PG,
        ACT
    };

    constexpr PacketSlot packetOrder[] = {
        PacketSlot::PRG, PacketSlot::PG, PacketSlot::ACT,
        PacketSlot::CAR0, PacketSlot::CAR4, PacketSlot::CAR1, PacketSlot::CAR5,
        PacketSlot::CAR2, PacketSlot::ACT, PacketSlot::CAR3};

    // OpenSlot-PRG-Oszillogramm: W=1, P=4 (Normalbetrieb), X=0, R=3.
    constexpr uint16_t normalOperationWord = 0b1'1000'001'00'110;
    // Reservierte Adresse 7, KFR=1, FR=0. Ghostcar und Pacecar bleiben stehen.
    constexpr uint16_t inactivePaceGhostWord = 0b1'111'1'0'0'0'0'0;
}

void SignalGenerator::reset()
{
    for (CarCommand &car : cars_)
        car = {};

    activityMask_ = 0;
    packetIndex_ = 0;
    halfBitIndex_ = 0;
}

bool SignalGenerator::setCarCommand(uint8_t address, uint8_t speed, bool laneChange)
{
    if (address >= carCount || speed > 15)
        return false;

    CarCommand &car = cars_[address];
    if (laneChange && !car.laneChangeRequested)
    {
        if (car.laneChangePending)
            return false;
        car.laneChangePending = true;
    }

    car.speed = speed;
    car.laneChangeRequested = laneChange;

    // R0 folgt direkt auf das Startbit, IE steht zuletzt.
    const uint16_t activityBit = 1U << (carCount - address);
    if (speed != 0)
        activityMask_ |= activityBit;
    else
        activityMask_ &= ~activityBit;
    return true;
}

SignalGenerator::TimedLevel SignalGenerator::nextTimedLevel()
{
    if (halfBitIndex_ == 0)
        packet_ = nextPacket();

    const uint8_t packetHalfBits = 2 * packet_.bitCount;
    if (halfBitIndex_ == packetHalfBits)
    {
        halfBitIndex_ = 0;
        return {RailLevel::HIGH, static_cast<uint16_t>(packetInterval_us - packetHalfBits * halfBit_us)};
    }

    const uint8_t bitIndex = packet_.bitCount - 1 - halfBitIndex_ / 2;
    const bool bit = (packet_.word & (1U << bitIndex)) != 0;
    // Logisch 1 ist HIGH–LOW, logisch 0 ist LOW–HIGH.
    const bool high = bit != (halfBitIndex_ % 2 != 0);
    ++halfBitIndex_;
    return {high ? RailLevel::HIGH : RailLevel::LOW, halfBit_us};
}

SignalGenerator::Packet SignalGenerator::nextPacket()
{
    const PacketSlot slot = packetOrder[packetIndex_++];
    if (packetIndex_ == sizeof(packetOrder) / sizeof(packetOrder[0]))
        packetIndex_ = 0;

    switch (slot)
    {
    case PacketSlot::PRG:
        return {normalOperationWord, 13};
    case PacketSlot::PG:
        return {inactivePaceGhostWord, 10};
    case PacketSlot::ACT:
        return {static_cast<uint16_t>(0x80 | activityMask_ | (activityMask_ != 0)), 8};
    default:
        return carPacket(static_cast<uint8_t>(slot));
    }
}

SignalGenerator::Packet SignalGenerator::carPacket(uint8_t address)
{
    CarCommand &car = cars_[address];
    // Zwischen zwei Tastendrücken muss ein Telegramm die Freigabe übertragen.
    const bool press = !car.laneChangeTransmitted && car.laneChangePending;
    if (press)
        car.laneChangePending = false;
    car.laneChangeTransmitted = press;

    // Start, Adresse (MSB zuerst), SW (aktiv LOW), Fahrstufe, Tankbit (aus).
    return {static_cast<uint16_t>(0x200 | (address << 6) | (!press << 5) | (car.speed << 1)), 10};
}
