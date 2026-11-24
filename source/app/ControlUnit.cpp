#include "ControlUnit.h"
#include "ControlUnitController.h"
#include "Communication/UartTransport.h"
#include "main.h"
#include "usart.h"

namespace
{
    constexpr ControlUnitConfig config{
        20,   // Bahn-Shunt in mΩ
        20,   // Versorgungs-Shunt in mΩ
        50,   // Verstärkung Bahnstrom
        50,   // Verstärkung Versorgungsstrom
        3300, // ADC-Referenzspannung in mV
        1500, // Kurzschlussgrenze in mA
        20    // Anzahl nötiger Überschreitungen
    };

    ControlUnit controlUnit{config};
    UartTransport uartTransport{huart2};
    uint16_t pendingResponse = 0;
    bool responsePending = false;
}

void ControlUnit_Init(void)
{
    if (controlUnit.init() != HAL_OK)
        Error_Handler();

    HAL_NVIC_SetPriority(USART2_IRQn, 2, 0);
    if (uartTransport.init() != HAL_OK)
        Error_Handler();
}

void ControlUnit_Tick(void)
{
    controlUnit.tick();

    if (!responsePending)
    {
        uint16_t request;
        if (!uartTransport.tryRecieveWord(&request))
            return;

        pendingResponse = controlUnit.handleCommand(request);
        responsePending = true;
    }

    if (uartTransport.trySendWord(pendingResponse))
        responsePending = false;
}

extern "C" void TIM2_IRQHandler(void)
{
    controlUnit.onRailTimerInterrupt();
}

/**
 * @brief Übergibt ein vollständiges USART2-Wort an den UART-Transport.
 * @param[in,out] uart HAL-Handle der UART, die den Interrupt ausgelöst hat.
 */
extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart == &huart2 && HAL_UART_GetError(uart) == HAL_UART_ERROR_NONE)
    {
        if (uartTransport.onRecieveComplete() != HAL_OK)
            HAL_UART_AbortReceive(uart);
    }
}

/**
 * @brief Gibt den Sendepuffer frei, wenn USART2 die Antwort gesendet hat.
 * @param[in] uart HAL-Handle der UART, die den Interrupt ausgelöst hat.
 */
extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart == &huart2)
        uartTransport.onTransmitComplete();
}

/**
 * @brief Stoppt den USART2-Empfang nach einem UART-Fehler.
 * @param[in,out] uart HAL-Handle der betroffenen UART.
 * @note Die Kommunikation muss danach an beiden Enden neu synchronisiert werden.
 */
extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if (uart == &huart2)
        HAL_UART_AbortReceive(uart);
}
