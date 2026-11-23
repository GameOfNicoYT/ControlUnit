#include "main.h"
#include "tim.h"
#include "ControlUnitController.h"

ControlUnit::ControlUnit(const ControlUnitConfig &config)
    : currentSensor_(config.trackShunt_mOhm,
                     config.supplyShunt_mOhm,
                     config.trackGain,
                     config.supplyGain,
                     config.adcReference_mV),
      detector_(config.shortCircuitLimit_mA,
                config.requiredSamples)
{
}

HAL_StatusTypeDef ControlUnit::init()
{
    rail_.init();

    if (currentSensor_.init() != HAL_OK)
        return HAL_ERROR;

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC4);
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);
    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    if (HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_4) != HAL_OK)
    {
        HAL_NVIC_DisableIRQ(TIM2_IRQn);
        return HAL_ERROR;
    }

    return HAL_OK;
}

void ControlUnit::onTrackSampleTrigger()
{
    const RailLevel level =
        HAL_GPIO_ReadPin(CARERRA_DATA_GPIO_Port, CARERRA_DATA_Pin) == GPIO_PIN_SET
            ? RailLevel::HIGH
            : RailLevel::LOW;
    currentSensor_.onSampleTrigger(level);
}

void ControlUnit::tick()
{
    uint32_t current_mA;
    RailLevel level;
    if (!currentSensor_.takeTrackSample(current_mA, level))
        return;

    if (detector_.check(current_mA, level == RailLevel::HIGH) == HAL_ERROR)
        rail_.onShortCircuit();
}
