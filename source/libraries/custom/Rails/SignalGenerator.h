#pragma once

#include <stdint.h>

#include "RailPowerLowLevel.h"

/// @brief Erzeugt den wiederholten Carrera-Digital-Telegrammzyklus nach OpenSlot.
class SignalGenerator
{
public:
    static constexpr uint8_t carCount = 6;
    static constexpr uint16_t halfBit_us = 50;
    static constexpr uint16_t packetInterval_us = 7500;

    struct TimedLevel
    {
        RailLevel level;
        uint16_t duration_us;
    };

    /// @brief Startet den Zyklus bei PRG und setzt alle Fahrzeuge auf Stillstand.
    void reset();

    /**
     * @brief Übernimmt die Fahrstufe und erkennt eine steigende Spurwechselanforderung.
     * @param address Carrera-Adresse von 0 bis 5.
     * @param speed Fahrstufe von 0 bis 15.
     * @param laneChange SPS-Anforderung für den Spurwechsel.
     * @retval true Der Befehl wurde übernommen.
     * @retval false Die Felder sind ungültig oder ein Spurwechsel wartet bereits.
     */
    bool setCarCommand(uint8_t address, uint8_t speed, bool laneChange);

    /**
     * @brief Liefert den nächsten Manchester-Halbbitpegel oder den HIGH-Pegel in der Paketpause.
     * @return Bahnpegel und Dauer bis zum nächsten Aufruf.
     */
    TimedLevel nextTimedLevel();

private:
    struct CarCommand
    {
        uint8_t speed = 0;
        bool laneChangeRequested = false;
        bool laneChangePending = false;
        bool laneChangeTransmitted = false;
    };

    struct Packet
    {
        uint16_t word = 0;
        uint16_t bitCount = 0;
    };

    Packet nextPacket();
    Packet carPacket(uint8_t address);

    CarCommand cars_[carCount]{};
    Packet packet_{};
    uint16_t activityMask_ = 0;
    uint8_t packetIndex_ = 0;
    uint8_t halfBitIndex_ = 0;
};
