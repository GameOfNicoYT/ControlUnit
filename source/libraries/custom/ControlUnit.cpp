#include "main.h"
#include "tim.h"
#include "ControlUnitController.h"

namespace
{
constexpr uint16_t sampleInterval_us = 400;
constexpr uint16_t sampleOffset_us = 20;
constexpr uint16_t maxCompareLatency_us = 15;
static_assert(sampleInterval_us % SignalGenerator::halfBit_us == 0);
static_assert(maxCompareLatency_us < sampleOffset_us);

uint16_t elapsedSince(uint16_t compare)
{
    return static_cast<uint16_t>(TIM2->CNT - compare);
}
}

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

    // TIM2 zählt mit 1 MHz und springt nach 65535 auf 0.
    if (HAL_RCC_GetPCLK1Freq() != 16000000U ||
        (RCC->CFGR & RCC_CFGR_PPRE1) != 0U ||
        htim2.Init.Prescaler != 15 || htim2.Init.Period != UINT16_MAX)
        return HAL_ERROR;

    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 1, 0);
    if (currentSensor_.init() != HAL_OK)
        return HAL_ERROR;

    TIM2->CCR4 = sampleOffset_us;
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

void ControlUnit::onRailTimerInterrupt()
{
    const uint32_t pending = TIM2->SR & TIM2->DIER;
    if ((pending & TIM_FLAG_CC1) != 0U)
    {
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC1);
        const auto compare = static_cast<uint16_t>(TIM2->CCR1);
        if (elapsedSince(compare) <= maxCompareLatency_us)
        {
            rail_.applySignalLevel(nextSignalStep_.level);
            TIM2->CCR1 = static_cast<uint16_t>(compare + nextSignalStep_.duration_us);
            // Bereite den nächsten Pegel erst nach der zeitkritischen Ausgabe vor.
            nextSignalStep_ = signalGenerator_.nextTimedLevel();
        }
        else
        {
            stopRailSignal();
            rail_.setFault();
        }
    }

    if ((pending & TIM_FLAG_CC4) != 0U)
    {
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC4);
        onTrackSampleTrigger();
    }
}

void ControlUnit::onTrackSampleTrigger()
{
    const auto compare = static_cast<uint16_t>(TIM2->CCR4);
    const uint16_t elapsed = elapsedSince(compare);
    // Bei verspätetem IRQ den nächsten Compare auf demselben Abtastraster setzen.
    uint32_t nextCompare = compare + sampleInterval_us;
    if (elapsed >= sampleInterval_us - maxCompareLatency_us)
        nextCompare += ((elapsed + maxCompareLatency_us) / sampleInterval_us) * sampleInterval_us;
    TIM2->CCR4 = static_cast<uint16_t>(nextCompare);

    if (elapsed > maxCompareLatency_us)
    {
        currentSensor_.onAdcError();
        return;
    }

    const RailLevel level =
        HAL_GPIO_ReadPin(CARERRA_DATA_GPIO_Port, CARERRA_DATA_Pin) == GPIO_PIN_SET
            ? RailLevel::HIGH
            : RailLevel::LOW;
    currentSensor_.onSampleTrigger(level);
}

bool ControlUnit::startRailSignal()
{
    const uint32_t previousMask = __get_PRIMASK();
    __disable_irq();

    if (rail_.state() == RailPowerState::OFF)
    {
        detector_.reset();
        signalGenerator_.reset();
        nextSignalStep_ = signalGenerator_.nextTimedLevel();
        rail_.enable();

        // Alle Signalpegel beginnen auf demselben Halbbitraster. CH4 misst
        // sampleOffset_us nach diesem Rasterpunkt.
        TIM2->CCR1 = static_cast<uint16_t>(TIM2->CCR4 - sampleOffset_us + sampleInterval_us);
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC1);
        __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_CC1);
    }

    const bool enabled = rail_.state() == RailPowerState::ON;
    __set_PRIMASK(previousMask);
    return enabled;
}

void ControlUnit::stopRailSignal()
{
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_CC1);
}

void ControlUnit::tick()
{
    if (heartbeatReceived_ && HAL_GetTick() - lastHeartbeat_ms_ >= heartbeatTimeout_ms_)
    {
        heartbeatReceived_ = false;
        stopRailSignal();
        rail_.onCommunicationTimeout();
    }

    uint32_t current_mA;
    RailLevel level;
    if (!currentSensor_.takeTrackSample(current_mA, level) || rail_.state() != RailPowerState::ON)
        return;

    if (detector_.check(current_mA, level == RailLevel::HIGH) == HAL_ERROR)
    {
        stopRailSignal();
        rail_.onShortCircuit();
    }
}

void ControlUnit::Req_ResetFault()
{
    stopRailSignal();
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
    case 0x0: // Communication
        if (word == 0x0100) // RUN_KEEPALIVE
        {
            lastHeartbeat_ms_ = HAL_GetTick();
            heartbeatReceived_ = true;
            return ack;
        }
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
        case RailPowerState::COMMUNICATION_TIMEOUT:
            return railPowerRequested ? 0x4002 : 0x4103;
        }
        return invalidState;
    }

    case 0x2: // SET_STATUS
        if (word == 0x2000)
        {
            stopRailSignal();
            rail_.disable();
            detector_.reset();
            return ack;
        }
        if (word == 0x2001)
        {
            if (!heartbeatReceived_)
                return invalidState;

            return startRailSignal() ? ack : invalidState;
        }
        return invalidArgument;

    case 0xA: // SEND_CAR_COMMAND
    {
        const uint8_t address = (word >> 8) & 0xF;
        const uint8_t speed = (word >> 4) & 0xF;
        const uint8_t laneChange = word & 0xF;
        if (address >= SignalGenerator::carCount || laneChange > 1)
            return invalidArgument;

        // Der Timer liest die Fahrdaten beim Vorbereiten des jeweiligen Telegramms.
        const uint32_t previousMask = __get_PRIMASK();
        __disable_irq();
        const bool accepted = rail_.state() == RailPowerState::ON &&
                              signalGenerator_.setCarCommand(address, speed, laneChange != 0);
        __set_PRIMASK(previousMask);
        return accepted ? ack : invalidState;
    }

    case 0xF:
        if (word == 0xF000) // RESET_ERROR
        {
            Req_ResetFault();
            return ack;
        }
        if (word == 0xFFFF) // ENTER_ERROR_MODE
        {
            stopRailSignal();
            rail_.setFault();
            return ack;
        }
        return unknownCommand;

    default:
        return unknownCommand;
    }
}
