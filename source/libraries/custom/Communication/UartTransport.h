#pragma once

#include "stm32l0xx_hal.h"

/**
 * @brief Sendet und empfängt 16-Bit-Wörter über UART-Interrupts.
 * @details Jedes Wort besteht aus zwei Bytes. Das höherwertige Byte wird
 * zuerst übertragen. Die Hauptschleife holt empfangene Wörter ab und startet
 * neue Übertragungen. Die HAL-Callbacks melden den Abschluss aus dem Interrupt.
 */
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
     * @pre Die UART-Peripherie ist auf 8 Datenbits eingestellt und ihr NVIC-Interrupt ist aktiviert.
     * @retval HAL_OK Der Empfang ist gestartet.
     * @retval HAL_BUSY Der HAL-Empfang läuft bereits.
     * @retval HAL_ERROR Der HAL hat den Start abgelehnt.
     */
    HAL_StatusTypeDef init();

    /**
     * @brief Holt ein vollständiges Wort in der Hauptschleife ab.
     * @param[out] data Zeiger auf das Ziel für das empfangene Wort.
     * @note Der Zielwert wird nur bei erfolgreicher Abholung beschrieben.
     * @retval true Ein Wort wurde kopiert und aus dem Empfangspuffer entnommen.
     * @retval false Es liegt kein Wort vor oder data ist nullptr.
     */
    bool tryRecieveWord(uint16_t *data);

    /**
     * @brief Startet in der Hauptschleife den interruptgesteuerten Versand eines Wortes.
     * @param[in] word Zu sendendes 16-Bit-Wort.
     * @retval true Die Übertragung wurde gestartet.
     * @retval false Der Sendepuffer ist belegt oder HAL hat den Start abgelehnt.
     * @note Der Sendepuffer bleibt bis onTransmitComplete() belegt.
     */
    bool trySendWord(uint16_t word);

    /**
     * @brief Speichert ein vollständiges Wort und startet den nächsten Empfang.
     * @note Im HAL-Empfangs-Callback aufrufen, sobald zwei Bytes angekommen sind.
     * Ein noch ungelesenes Wort bleibt erhalten. Das neu eingetroffene Wort wird dann verworfen.
     * @retval HAL_OK Der nächste Empfang ist gestartet.
     * @retval HAL_BUSY Der HAL-Empfang läuft bereits.
     * @retval HAL_ERROR Der HAL hat den Neustart abgelehnt.
     */
    HAL_StatusTypeDef onRecieveComplete();

    /**
     * @brief Gibt den Sendepuffer für die nächste Übertragung frei.
     * @note Im HAL-Sende-Callback aufrufen, wenn das gesamte Wort übertragen wurde.
     */
    void onTransmitComplete();

private:
    UART_HandleTypeDef &uart_;
    uint8_t rxBuffer_[2];
    volatile uint16_t receivedWord_;
    volatile bool rxReady_ = false;
    uint8_t txBuffer_[2];
    volatile bool txBusy_ = false;
};
