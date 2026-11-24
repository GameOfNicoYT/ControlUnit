#include "SignalGenerator.h"

#include <cstdlib>
#include <iostream>

namespace
{
void expect(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void expectPacket(SignalGenerator &generator, uint16_t word, uint8_t bitCount)
{
    uint16_t duration_us = 0;
    for (int bit = bitCount - 1; bit >= 0; --bit)
    {
        const auto first = generator.nextTimedLevel();
        const auto second = generator.nextTimedLevel();
        const bool one = (word & (1U << bit)) != 0;
        expect(first.level == (one ? RailLevel::HIGH : RailLevel::LOW), "Wrong first Manchester half-bit");
        expect(second.level == (one ? RailLevel::LOW : RailLevel::HIGH), "Wrong second Manchester half-bit");
        expect(first.duration_us == 50 && second.duration_us == 50, "Half-bit must last 50 us");
        duration_us += first.duration_us + second.duration_us;
    }

    const auto idle = generator.nextTimedLevel();
    expect(idle.level == RailLevel::HIGH, "Track must idle high between packets");
    expect(duration_us + idle.duration_us == 7500, "Packets must start 7.5 ms apart");
}

void expectCycle(SignalGenerator &generator, uint8_t activityWord, uint16_t car0, uint16_t car3)
{
    expectPacket(generator, 0x1826, 13);      // OpenSlot PRG capture
    expectPacket(generator, 0x03E0, 10);      // No pace or ghost car
    expectPacket(generator, activityWord, 8);
    expectPacket(generator, car0, 10);
    expectPacket(generator, 0x0320, 10);      // CAR[4]
    expectPacket(generator, 0x0260, 10);      // CAR[1]
    expectPacket(generator, 0x0360, 10);      // CAR[5]
    expectPacket(generator, 0x02A0, 10);      // CAR[2]
    expectPacket(generator, activityWord, 8);
    expectPacket(generator, car3, 10);
}
}

int main()
{
    SignalGenerator generator;
    expectCycle(generator, 0x80, 0x220, 0x2E0);

    expect(!generator.setCarCommand(6, 1, false), "Reserved address was accepted");
    expect(!generator.setCarCommand(0, 16, false), "Invalid speed was accepted");
    expect(generator.setCarCommand(0, 15, false), "CAR[0] command was rejected");
    expect(generator.setCarCommand(3, 5, true), "CAR[3] command was rejected");
    expectCycle(generator, 0xC9, 0x23E, 0x2CA);

    expect(generator.setCarCommand(3, 5, true), "Repeated request was rejected");
    expectCycle(generator, 0xC9, 0x23E, 0x2EA);

    expect(generator.setCarCommand(3, 5, false), "Lane change reset was rejected");
    expect(generator.setCarCommand(3, 5, true), "New lane change was rejected");
    expect(generator.setCarCommand(3, 5, false), "Early lane change reset was rejected");
    expect(!generator.setCarCommand(3, 2, true), "Pending lane change was silently overwritten");
    expectCycle(generator, 0xC9, 0x23E, 0x2CA);
    expect(generator.setCarCommand(3, 5, true), "Next lane change was rejected");
    expectCycle(generator, 0xC9, 0x23E, 0x2EA);
    expectCycle(generator, 0xC9, 0x23E, 0x2CA);

    expect(generator.setCarCommand(0, 0, false), "Stop command for CAR[0] was rejected");
    expect(generator.setCarCommand(3, 0, false), "Stop command for CAR[3] was rejected");
    expectCycle(generator, 0x80, 0x220, 0x2E0);

    expect(generator.setCarCommand(3, 5, true), "Lane change before reset was rejected");
    generator.nextTimedLevel();  // Reset may happen in the middle of a packet.
    generator.reset();
    expectCycle(generator, 0x80, 0x220, 0x2E0);
}
