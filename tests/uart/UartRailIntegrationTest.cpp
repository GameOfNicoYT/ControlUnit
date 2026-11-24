#include "ControlUnit.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *handle);
extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *handle);
extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *handle);
extern "C" void TIM2_IRQHandler();

namespace
{
GPIO_TypeDef gpio{};
RCC_TypeDef rcc{};
TIM_TypeDef timer{};
uint32_t now_us = 0;
uint32_t interruptMask = 0;
uint8_t *rx = nullptr;
const uint8_t *tx = nullptr;
uint16_t *adcBuffer = nullptr;

void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct PacketCapture
{
    std::vector<uint16_t> cars[6];

    void record(bool high, uint16_t duration_us)
    {
        if (duration_us == 50)
        {
            if (halfBits_ % 2 == 0)
                firstHigh_ = high;
            else
            {
                require(firstHigh_ != high, "Wrong Manchester polarity on PA8");
                word_ = static_cast<uint16_t>((word_ << 1) | (firstHigh_ ? 1U : 0U));
            }
            ++halfBits_;
            return;
        }

        require(high && halfBits_ != 0 && duration_us + halfBits_ * 50 == 7500,
                "Wrong packet timing or idle level on PA8");
        const unsigned address = (word_ >> 6) & 7;
        if (halfBits_ == 20 && address < 6)
            cars[address].push_back(word_);
        halfBits_ = 0;
        word_ = 0;
    }

private:
    bool firstHigh_ = false;
    unsigned halfBits_ = 0;
    uint16_t word_ = 0;
};

void sendWord(uint16_t request, uint16_t expected)
{
    require(rx != nullptr && tx == nullptr, "UART was not ready for a request");
    rx[0] = static_cast<uint8_t>(request >> 8);
    rx[1] = static_cast<uint8_t>(request);
    rx = nullptr;
    HAL_UART_RxCpltCallback(&huart2);
    ControlUnit_Tick();
    require(tx != nullptr, "No UART reply");
    const uint16_t response = static_cast<uint16_t>((tx[0] << 8) | tx[1]);
    if (response != expected)
        std::cerr << std::hex << "Request 0x" << request << ", expected 0x" << expected
                  << ", received 0x" << response << '\n';
    require(response == expected, "Unexpected UART reply");
    tx = nullptr;
    HAL_UART_TxCpltCallback(&huart2);
}

void runUntil(uint32_t time_ms, PacketCapture &capture)
{
    const uint32_t end_us = time_ms * 1000;
    while (now_us < end_us)
    {
        const auto sampleDelay = static_cast<uint16_t>(timer.CCR4 - now_us);
        const uint32_t signalDelay = (timer.DIER & TIM_IT_CC1) != 0
            ? static_cast<uint16_t>(timer.CCR1 - now_us) : 65536;
        const uint32_t delay = std::min<uint32_t>(signalDelay, sampleDelay);
        require(delay != 0, "Timer did not schedule the next compare");
        if (delay > end_us - now_us)
            break;

        now_us += delay;
        timer.CNT = static_cast<uint16_t>(now_us);
        const bool sample = delay == sampleDelay;
        const bool signal = delay == signalDelay;
        timer.SR = (sample ? TIM_FLAG_CC4 : 0) | (signal ? TIM_FLAG_CC1 : 0);
        TIM2_IRQHandler();

        if (signal)
            capture.record((gpio.levels & GPIO_PIN_8) != 0, static_cast<uint16_t>(timer.CCR1 - now_us));
        if (sample)
        {
            adcBuffer[0] = adcBuffer[1] = 0;
            HAL_ADC_ConvCpltCallback(&hadc);
        }
        ControlUnit_Tick();
    }
    now_us = end_us;
    timer.CNT = static_cast<uint16_t>(now_us);
    ControlUnit_Tick();
}
}

GPIO_TypeDef *const GPIOA = &gpio;
RCC_TypeDef *const RCC = &rcc;
TIM_TypeDef *const TIM2 = &timer;
UART_HandleTypeDef huart2;
ADC_HandleTypeDef hadc;
TIM_HandleTypeDef htim2{{15, UINT16_MAX}, &timer};

extern "C" void Error_Handler()
{
    require(false, "ControlUnit initialization failed");
}

uint32_t __get_PRIMASK() { return interruptMask; }
void __disable_irq() { interruptMask = 1; }
void __set_PRIMASK(uint32_t mask) { interruptMask = mask; }
uint32_t HAL_RCC_GetPCLK1Freq() { return 16000000; }
uint32_t HAL_GetTick() { return now_us / 1000; }
void HAL_NVIC_SetPriority(IRQn_Type, uint32_t, uint32_t) {}
void HAL_NVIC_ClearPendingIRQ(IRQn_Type) {}
void HAL_NVIC_EnableIRQ(IRQn_Type) {}
void HAL_NVIC_DisableIRQ(IRQn_Type) {}

HAL_StatusTypeDef HAL_TIM_OC_Start_IT(TIM_HandleTypeDef *handle, uint32_t channel)
{
    if (channel != TIM_CHANNEL_4)
        return HAL_ERROR;
    handle->Instance->DIER |= TIM_FLAG_CC4;
    return HAL_OK;
}

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState level)
{
    if (level == GPIO_PIN_SET)
        port->levels |= pin;
    else
        port->levels &= ~pin;
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    return (port->levels & pin) != 0 ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *, uint32_t) { return HAL_OK; }
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *, uint32_t *data, uint32_t size)
{
    if (size != 2)
        return HAL_ERROR;
    adcBuffer = reinterpret_cast<uint16_t *>(data);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *handle, uint8_t *data, uint16_t size)
{
    if (handle != &huart2 || size != 2 || rx != nullptr)
        return HAL_ERROR;
    rx = data;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *handle, const uint8_t *data, uint16_t size)
{
    if (handle != &huart2 || size != 2 || tx != nullptr)
        return HAL_ERROR;
    tx = data;
    return HAL_OK;
}

uint32_t HAL_UART_GetError(UART_HandleTypeDef *) { return HAL_UART_ERROR_NONE; }
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *)
{
    rx = nullptr;
    return HAL_OK;
}

int main()
{
    ControlUnit_Init();

    sendWord(0x0000, 0x3000);
    sendWord(0x1000, 0x4000);
    sendWord(0x2001, 0x3003);
    sendWord(0xA351, 0x3003);
    sendWord(0x0100, 0x3000);
    sendWord(0x2001, 0x3000);
    sendWord(0xA651, 0x3002);
    sendWord(0xA351, 0x3000);
    sendWord(0xA070, 0x3000);

    PacketCapture capture;
    runUntil(250, capture);
    require(capture.cars[3] == std::vector<uint16_t>{0x2CA, 0x2EA, 0x2EA},
                           "A single UART command must repeat speed and pulse lane change once");
    sendWord(0x0100, 0x3000);
    sendWord(0xA390, 0x3000);

    runUntil(500, capture);
    require(capture.cars[3].back() == 0x2F2,
                           "A new command must replace the stored speed");
    require(capture.cars[0].size() > 3 &&
                           std::all_of(capture.cars[0].begin(), capture.cars[0].end(),
                                       [](uint16_t word) { return word == 0x22E; }),
                           "Updating one car must leave the other car's command active");

    runUntil(1249, capture);
    sendWord(0x1000, 0x4001);
    runUntil(1250, capture);
    require(gpio.levels == 0 && (timer.DIER & TIM_IT_CC1) == 0,
                           "Heartbeat timeout must stop the rail output and signal interrupt");
    sendWord(0x1000, 0x4002);
    sendWord(0x1100, 0x4103);
    sendWord(0x0100, 0x3000);
    sendWord(0x2001, 0x3003);
    sendWord(0xF000, 0x3000);
    sendWord(0x2001, 0x3000);

    PacketCapture restarted;
    runUntil(1325, restarted);
    require(restarted.cars[0] == std::vector<uint16_t>{0x220} &&
                           restarted.cars[3] == std::vector<uint16_t>{0x2E0},
                           "Re-enabling must restart all cars at speed zero");
    sendWord(0x2000, 0x3000);
    sendWord(0x1000, 0x4000);
}
