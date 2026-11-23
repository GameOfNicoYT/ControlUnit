#pragma once

#include "stm32l0xx_hal.h"

/** @brief Empfängt 16-Bit-Wörter aus jeweils zwei UART-Bytes. */
class UartTransport
{
public:
    /**
     * @brief Bindet den vorhandenen HAL-Handle an den Transport.
     * @param[in,out] handle UART-Handle, der während der Lebensdauer des Transports gültig bleibt.
     */
    UartTransport(UART_HandleTypeDef &handle);

    /**
     * @brief Startet den ersten interruptgesteuerten Empfang von zwei Bytes.
     * @pre Die UART-Peripherie und ihr NVIC-Interrupt sind initialisiert.
     * @retval HAL_OK Der Empfang ist gestartet.
     * @retval HAL_BUSY Der HAL-Empfang läuft bereits.
     * @retval HAL_ERROR Der HAL hat den Start abgelehnt.
     */
    HAL_StatusTypeDef init();

    /**
     * @brief Holt ein vollständiges Wort in der Hauptschleife ab.
     * @param[out] data Ziel für das empfangene Wort. Bei false bleibt der Wert unverändert.
     * @retval true Ein Wort wurde kopiert und aus dem Empfangspuffer entnommen.
     * @retval false Es liegt kein Wort vor oder data ist nullptr.
     */
    bool tryRecieveWord(uint16_t *data);

    /**
     * @brief Platzhalter für die spätere Übertragung eines Wortes.
     * @param[in] word Zu sendendes 16-Bit-Wort.
     * @retval false Der Versand ist noch nicht implementiert.
     */
    bool trySendWord(uint16_t word);

    /**
     * @brief Speichert ein vollständiges Wort und startet den nächsten Empfang.
     * @note Im UART-Interrupt aufrufen. Ein noch ungelesenes Wort bleibt erhalten.
     * @retval HAL_OK Der nächste Empfang ist gestartet.
     * @retval HAL_BUSY Der HAL-Empfang läuft bereits.
     * @retval HAL_ERROR Der HAL hat den Neustart abgelehnt.
     */
    HAL_StatusTypeDef onRecieveComplete();

    /**
     * @brief Platzhalter für die spätere Behandlung des Sendeabschlusses.
     * @retval HAL_ERROR Die Sendefunktion ist noch nicht implementiert.
     */
    HAL_StatusTypeDef onTransmitComplete();

private:
    UART_HandleTypeDef &uart_;
    uint8_t rxBuffer_[2];
    volatile uint16_t receivedWord_;
    volatile bool rxReady_ = false;
    uint8_t txBuffer_;
    bool txBusy_ = false;
};
