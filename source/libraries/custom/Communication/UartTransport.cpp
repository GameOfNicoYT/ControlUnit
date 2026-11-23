#include "UartTransport.h"

UartTransport::UartTransport(UART_HandleTypeDef &handle) : uart_(handle)
{
}

HAL_StatusTypeDef UartTransport::init()
{
    return HAL_UART_Receive_IT(&uart_, rxBuffer_, sizeof(rxBuffer_));
}

bool UartTransport::tryRecieveWord(uint16_t *data)
{
    if (data == nullptr)
        return false;

    const uint32_t previousMask = __get_PRIMASK();
    __disable_irq();

    const bool available = rxReady_;
    if (available)
    {
        *data = receivedWord_;
        rxReady_ = false;
    }

    __set_PRIMASK(previousMask);
    return available;
}

bool UartTransport::trySendWord(uint16_t)
{
    return false;
}

HAL_StatusTypeDef UartTransport::onRecieveComplete()
{
    if (!rxReady_)
    {
        receivedWord_ = static_cast<uint16_t>((rxBuffer_[0] << 8) | rxBuffer_[1]);
        rxReady_ = true;
    }

    return HAL_UART_Receive_IT(&uart_, rxBuffer_, sizeof(rxBuffer_));
}

HAL_StatusTypeDef UartTransport::onTransmitComplete()
{
    return HAL_ERROR;
}
