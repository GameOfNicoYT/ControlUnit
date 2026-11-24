#pragma once

#include <stdint.h>

#include "stm32l0xx_hal.h"

#include "Rails/CurrentSensors.h"
#include "Rails/RailPower.h"
#include "Rails/RailShortCircuitDetector.h"

/** @brief Messparameter und Grenzwerte für die Bahnüberwachung. */
struct ControlUnitConfig
{
    uint32_t trackShunt_mOhm;       ///< Shuntwiderstand des Bahnausgangs in Milliohm.
    uint32_t supplyShunt_mOhm;      ///< Shuntwiderstand der Versorgung in Milliohm.
    uint32_t trackGain;            ///< Verstärkungsfaktor der Bahnstrommessung.
    uint32_t supplyGain;           ///< Verstärkungsfaktor der Versorgungsstrommessung.
    uint32_t adcReference_mV;      ///< ADC-Referenzspannung in Millivolt.
    uint32_t shortCircuitLimit_mA; ///< Kurzschlussgrenze für den Bahnstrom in Milliampere.
    uint8_t requiredSamples;       ///< Anzahl der Grenzwertüberschreitungen bis zur Abschaltung.
};

/**
 * @brief Koordiniert die Bahnversorgung, Stromüberwachung und Befehle der SPS.
 */
class ControlUnit
{
public:
    /**
     * @brief Erstellt die ControlUnit mit den Parametern der Bahnüberwachung.
     * @param[in] config Messparameter und Grenzwerte für Stromsensor und Kurzschlussdetektor.
     */
    explicit ControlUnit(const ControlUnitConfig& config);

    /**
     * @brief Initialisiert die Bahn im Zustand OFF und startet die Stromüberwachung.
     * @pre CubeMX hat GPIO, ADC, DMA und TIM2 initialisiert.
     * @retval HAL_OK Stromsensor und TIM2-Interrupt wurden erfolgreich gestartet.
     * @retval HAL_ERROR Der Stromsensor oder der Start von TIM2 ist fehlgeschlagen.
     */
    HAL_StatusTypeDef init();

    /**
     * @brief Wertet eine verfügbare Bahnstrommessung in der Hauptschleife aus.
     * @details Bei erkanntem Kurzschluss sperrt RailPower den Bahnausgang.
     * @pre init() wurde erfolgreich abgeschlossen.
     */
    void tick();

    /**
     * @brief Erfasst den logischen Pegel des Bahnausgangs zum ADC-Messzeitpunkt.
     * @note Wird aus dem TIM2-Interrupt aufgerufen und ordnet den Pegel der Messung zu.
     */
    void onTrackSampleTrigger();

    /**
     * @brief Quittiert den Bahnfehler und hält den Bahnausgang ausgeschaltet.
     * @details Setzt auch den Zähler des Kurzschlussdetektors zurück.
     */
    void Req_ResetFault();

    /**
     * @brief Verarbeitet ein MOXA-Befehlswort in der Hauptschleife.
     * @param[in] word Empfangenes 16-Bit-Befehlswort.
     * @return Das 16-Bit-Antwortwort für die SPS.
     */
    uint16_t handleCommand(uint16_t word);

private:
    RailPower rail_;
    CurrentSensor currentSensor_;
    RailShortCircuitDetector detector_;
};
