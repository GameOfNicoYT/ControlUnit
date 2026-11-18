#pragma once

#include <stdint.h>
#include "RailPowerLowLevel.h"

class CurrentSensor
{
public:
    CurrentSensor(uint32_t trackShunt_mOhm,
                  uint32_t supplyShunt_mOhm,
                  uint32_t trackGain,
                  uint32_t supplyGain,
                  uint32_t adcReference_mV);

    /** @brief Kalibriert den ADC und startet die DMA-Messung. */
    bool init();

    /** @brief Uebergibt den Bahnpegel zum Zeitpunkt eines ADC-Triggers. */
    void onSampleTrigger(RailLevel level);

    /** @brief Rechnet einen ADC-Bahnwert in mA um. */
    void updateTrack(uint16_t raw, RailLevel level);

    /** @brief Rechnet einen ADC-Versorgungswert in mA um. */
    void updateSupply(uint16_t raw);

    /** @brief Liefert den zuletzt berechneten Bahnstrom. */
    uint32_t trackCurrent_mA() const;

    /** @brief Liefert den zuletzt berechneten Versorgungsstrom. */
    uint32_t supplyCurrent_mA() const;

    /** @brief Liefert den Pegel der letzten gueltigen Bahnprobe. */
    RailLevel trackOutput() const;

    /** @brief Meldet eine gueltige Bahnprobe mit zugeordnetem Pegel. */
    bool hasValidTrackSample() const;

    /** @brief Meldet eine gueltige Versorgungsprobe. */
    bool hasValidSupplySample() const;

    /** @brief Entnimmt genau eine neue Bahnprobe atomar. */
    bool takeTrackSample(uint32_t &current_mA, RailLevel &level);

    /** @brief Verarbeitet eine vollstaendige ADC-DMA-Sequenz. */
    void onDmaComplete();

    /** @brief Markiert ADC-Messwerte nach einem Fehler als ungueltig. */
    void onAdcError();

private:
    const uint32_t trackShunt_mOhm_;
    const uint32_t supplyShunt_mOhm_;
    const uint32_t trackGain_;
    const uint32_t supplyGain_;
    const uint32_t adcReference_mV_;
    volatile uint32_t trackCurrent_mA_ = 0;
    volatile uint32_t supplyCurrent_mA_ = 0;
    volatile RailLevel trackOutput_ = RailLevel::LOW;
    volatile bool trackSampleValid_ = false;
    volatile bool supplySampleValid_ = false;
    volatile RailLevel pendingTrackLevel_ = RailLevel::LOW;
    volatile bool pendingTrackLevelValid_ = false;
    uint16_t adcBuffer_[2]{};
};
