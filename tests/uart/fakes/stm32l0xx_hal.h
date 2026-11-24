#pragma once

#include <stdint.h>

extern "C"
{

enum HAL_StatusTypeDef
{
    HAL_OK,
    HAL_ERROR,
    HAL_BUSY
};

struct UART_HandleTypeDef {};

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *handle, uint8_t *data, uint16_t size);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *handle, const uint8_t *data, uint16_t size);
uint32_t __get_PRIMASK();
void __disable_irq();
void __set_PRIMASK(uint32_t mask);

struct ADC_HandleTypeDef {};
struct GPIO_TypeDef { uint16_t levels = 0; };
struct RCC_TypeDef { uint32_t CFGR = 0; };
struct TIM_TypeDef
{
    uint32_t CNT = 0;
    uint32_t SR = 0;
    uint32_t DIER = 0;
    uint32_t CCR1 = 0;
    uint32_t CCR4 = 0;
};
struct TIM_HandleTypeDef
{
    struct { uint32_t Prescaler; uint32_t Period; } Init;
    TIM_TypeDef *Instance;
};

enum GPIO_PinState { GPIO_PIN_RESET, GPIO_PIN_SET };
enum IRQn_Type { DMA1_Channel1_IRQn, TIM2_IRQn, USART2_IRQn };

extern GPIO_TypeDef *const GPIOA;
extern RCC_TypeDef *const RCC;
extern TIM_TypeDef *const TIM2;

constexpr uint16_t GPIO_PIN_8 = 1U << 8;
constexpr uint32_t RCC_CFGR_PPRE1 = 7U << 8;
constexpr uint32_t HAL_UART_ERROR_NONE = 0;
constexpr uint32_t ADC_SINGLE_ENDED = 0;
constexpr uint32_t TIM_FLAG_CC1 = 1U << 1;
constexpr uint32_t TIM_FLAG_CC4 = 1U << 4;
constexpr uint32_t TIM_IT_CC1 = TIM_FLAG_CC1;
constexpr uint32_t TIM_CHANNEL_4 = 12;

#define __HAL_TIM_CLEAR_FLAG(handle, flag) ((handle)->Instance->SR &= ~(flag))
#define __HAL_TIM_ENABLE_IT(handle, interrupt) ((handle)->Instance->DIER |= (interrupt))
#define __HAL_TIM_DISABLE_IT(handle, interrupt) ((handle)->Instance->DIER &= ~(interrupt))

uint32_t HAL_RCC_GetPCLK1Freq();
uint32_t HAL_GetTick();
void HAL_NVIC_SetPriority(IRQn_Type irq, uint32_t priority, uint32_t subPriority);
void HAL_NVIC_ClearPendingIRQ(IRQn_Type irq);
void HAL_NVIC_EnableIRQ(IRQn_Type irq);
void HAL_NVIC_DisableIRQ(IRQn_Type irq);
HAL_StatusTypeDef HAL_TIM_OC_Start_IT(TIM_HandleTypeDef *handle, uint32_t channel);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState level);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
HAL_StatusTypeDef HAL_ADCEx_Calibration_Start(ADC_HandleTypeDef *handle, uint32_t mode);
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef *handle, uint32_t *data, uint32_t size);
uint32_t HAL_UART_GetError(UART_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *handle);
}
