#include "ControlUnit.h"
#include "ControlUnitController.h"
#include "Communication/UartTransport.h"
#include "main.h"
#include "tim.h"
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
}

void ControlUnit_Init(void)
{
    if (controlUnit.init() != HAL_OK)
        Error_Handler();

    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
    if (uartTransport.init() != HAL_OK)
        Error_Handler();
}

void ControlUnit_Tick(void)
{
    controlUnit.tick();
}

extern "C" void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}

extern "C" void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *timer)
{
    if (timer == &htim2 && timer->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
        controlUnit.onTrackSampleTrigger();
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
 * @brief Stoppt den USART2-Empfang nach einem UART-Fehler.
 * @param[in,out] uart HAL-Handle der betroffenen UART.
 * @note Die Kommunikation muss danach an beiden Enden neu synchronisiert werden.
 */
extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if (uart == &huart2)
        HAL_UART_AbortReceive(uart);
}
