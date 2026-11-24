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
    if (!currentSensor_.takeTrackSample(current_mA, level) || rail_.state() != RailPowerState::ON)
        return;

    if (detector_.check(current_mA, level == RailLevel::HIGH) == HAL_ERROR)
        rail_.onShortCircuit();
}

void ControlUnit::Req_ResetFault()
{
    rail_.disable();
    rail_.resetFault();
    detector_.reset();
}

uint16_t ControlUnit::handleCommand(uint16_t word)
{
    constexpr uint16_t ack = 0x3000;
    constexpr uint16_t unknownCommand = 0x3001;
    constexpr uint16_t invalidArgument = 0x3002;
    constexpr uint16_t invalidState = 0x3003;

    switch (word >> 12)
    {
    case 0x0: // INIT
        return word == 0x0000 ? ack : invalidArgument;

    case 0x1: // GET_STATUS
    {
        const bool railPowerRequested = word == 0x1000;
        if (!railPowerRequested && word != 0x1100)
            return invalidArgument;

        switch (rail_.state())
        {
        case RailPowerState::OFF:
            return railPowerRequested ? 0x4000 : 0x4100;
        case RailPowerState::ON:
            return railPowerRequested ? 0x4001 : 0x4100;
        case RailPowerState::SHORT_CIRCUIT:
            return railPowerRequested ? 0x4002 : 0x4101;
        case RailPowerState::FAULT:
            return railPowerRequested ? 0x4002 : 0x4102;
        }
        return invalidState;
    }

    case 0x2: // SET_STATUS
        if (word == 0x2000)
        {
            rail_.disable();
            detector_.reset();
            return ack;
        }
        if (word == 0x2001)
        {
            rail_.enable();
            return rail_.state() == RailPowerState::ON ? ack : invalidState;
        }
        return invalidArgument;

    case 0xA: // SEND_CAR_COMMAND
    {
        const uint8_t carID = (word >> 8) & 0xF;
        const uint8_t laneChange = word & 0xF;
        if (carID > 6 || laneChange > 1)
            return invalidArgument;

        // The track signal generator still needs to consume car commands.
        return invalidState;
    }

    case 0xF:
        if (word == 0xF000) // RESET_ERROR
        {
            Req_ResetFault();
            return ack;
        }
        if (word == 0xFFFF) // ENTER_ERROR_MODE
        {
            rail_.setFault();
            return ack;
        }
        return unknownCommand;

    default:
        return unknownCommand;
    }
}
