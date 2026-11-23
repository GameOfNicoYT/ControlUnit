#include "ControlUnit.h"
#include "ControlUnitController.h"
#include "main.h"
#include "tim.h"

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
}

void ControlUnit_Init(void)
{
    if (controlUnit.init() != HAL_OK)
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
