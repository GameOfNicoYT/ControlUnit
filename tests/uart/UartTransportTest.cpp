#include "UartTransport.h"

#include <cstdlib>
#include <iostream>

namespace
{
UART_HandleTypeDef *activeHandle = nullptr;
uint8_t *activeBuffer = nullptr;
uint16_t activeSize = 0;
unsigned receiveStarts = 0;
UART_HandleTypeDef *transmitHandle = nullptr;
const uint8_t *transmitBuffer = nullptr;
uint16_t transmitSize = 0;
unsigned transmitStarts = 0;
HAL_StatusTypeDef transmitStatus = HAL_OK;
uint32_t interruptMask = 0;
HAL_StatusTypeDef receiveStatus = HAL_OK;
unsigned failures = 0;

void expect(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        ++failures;
    }
}

HAL_StatusTypeDef completeReceive(UartTransport &transport, uint8_t highByte, uint8_t lowByte)
{
    if (activeBuffer == nullptr || activeSize != 2)
        return HAL_ERROR;

    activeBuffer[0] = highByte;
    activeBuffer[1] = lowByte;
    activeBuffer = nullptr;
    return transport.onRecieveComplete();
}
}

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *handle, uint8_t *data, uint16_t size)
{
    ++receiveStarts;
    if (receiveStatus == HAL_OK)
    {
        activeHandle = handle;
        activeBuffer = data;
        activeSize = size;
    }
    return receiveStatus;
}

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *handle, const uint8_t *data, uint16_t size)
{
    ++transmitStarts;
    if (transmitStatus == HAL_OK)
    {
        transmitHandle = handle;
        transmitBuffer = data;
        transmitSize = size;
    }
    return transmitStatus;
}

uint32_t __get_PRIMASK()
{
    return interruptMask;
}

void __disable_irq()
{
    interruptMask = 1;
}

void __set_PRIMASK(uint32_t mask)
{
    interruptMask = mask;
}

int main()
{
    UART_HandleTypeDef uart;
    UartTransport transport(uart);
    uint16_t word = 0xFFFF;

    expect(!transport.tryRecieveWord(&word) && word == 0xFFFF, "An empty buffer must leave the output untouched");
    expect(interruptMask == 0, "Reading an empty buffer must restore enabled interrupts");
    expect(transport.init() == HAL_OK, "Initialization must start reception");
    expect(activeHandle == &uart && activeSize == 2 && receiveStarts == 1,
           "Initialization must arm exactly two bytes on the supplied UART");

    expect(completeReceive(transport, 0x12, 0x34) == HAL_OK && receiveStarts == 2,
           "A completed word must rearm reception");
    expect(!transport.tryRecieveWord(nullptr), "A null output pointer must be rejected");
    expect(completeReceive(transport, 0x56, 0x78) == HAL_OK && receiveStarts == 3,
           "Reception must rearm even if an older word is still pending");

    expect(transport.tryRecieveWord(&word) && word == 0x1234,
           "The older pending word must be returned in big-endian order");
    expect(interruptMask == 0, "Reading a word must restore enabled interrupts");
    expect(!transport.tryRecieveWord(&word) && word == 0x1234, "A received word must be returned only once");

    receiveStatus = HAL_BUSY;
    expect(completeReceive(transport, 0, 0) == HAL_BUSY && receiveStarts == 4,
           "The callback must report when the HAL fails to rearm");
    interruptMask = 1;
    expect(transport.tryRecieveWord(&word) && word == 0, "Zero must be returned as a valid word");
    expect(interruptMask == 1, "Reading a word must preserve an existing interrupt mask");

    expect(transport.trySendWord(0x1234), "A word must start the UART transmission");
    expect(transmitHandle == &uart && transmitStarts == 1, "Transmission must use the supplied UART");
    expect(transmitSize == 2 && transmitBuffer != nullptr &&
           transmitBuffer[0] == 0x12 && transmitBuffer[1] == 0x34,
           "Transmission must send the high byte first");
    expect(!transport.trySendWord(0x5678) && transmitStarts == 1,
           "An active transmission must prevent reusing the send buffer");
    expect(transmitBuffer != nullptr && transmitBuffer[0] == 0x12 && transmitBuffer[1] == 0x34,
           "The active transmission must retain its original bytes");

    transport.onTransmitComplete();
    transmitStatus = HAL_BUSY;
    expect(!transport.trySendWord(0x3000) && transmitStarts == 2,
           "A HAL-busy result must report a failed start");
    transmitStatus = HAL_OK;
    expect(transport.trySendWord(0x3000) && transmitStarts == 3,
           "A failed start must leave the buffer available for a retry");
    expect(transmitSize == 2 && transmitBuffer != nullptr &&
           transmitBuffer[0] == 0x30 && transmitBuffer[1] == 0,
           "An ACK must send both bytes starting with 0x30");
    transport.onTransmitComplete();

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
