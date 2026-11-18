#include "CurrentSensors.h"
#include "adc.h"

namespace
{
CurrentSensor *activeSensor = nullptr;
}

CurrentSensor::CurrentSensor(uint32_t trackShunt_mOhm,
                             uint32_t supplyShunt_mOhm,
                             uint32_t trackGain,
                             uint32_t supplyGain,
                             uint32_t adcReference_mV)
    : trackShunt_mOhm_(trackShunt_mOhm),
      supplyShunt_mOhm_(supplyShunt_mOhm),
      trackGain_(trackGain),
      supplyGain_(supplyGain),
      adcReference_mV_(adcReference_mV)
{
}

bool CurrentSensor::init()
{
    if ((activeSensor != nullptr && activeSensor != this) ||
        trackShunt_mOhm_ == 0 || supplyShunt_mOhm_ == 0 ||
        trackGain_ == 0 || supplyGain_ == 0 || adcReference_mV_ == 0)
        return false;

    trackSampleValid_ = false;
    supplySampleValid_ = false;
    pendingTrackLevelValid_ = false;

    // HAL kalibriert den ADC vor dem Start der Messungen.
    if (HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED) != HAL_OK)
        return false;

    activeSensor = this;
    // HAL startet den ADC mit zyklischem DMA fuer zwei 16-Bit-Werte.
    if (HAL_ADC_Start_DMA(&hadc, reinterpret_cast<uint32_t *>(adcBuffer_), 2) != HAL_OK)
    {
        activeSensor = nullptr;
        return false;
    }
    return true;
}

void CurrentSensor::onSampleTrigger(RailLevel level)
{
    const uint32_t previousMask = __get_PRIMASK();
    __disable_irq();
    // Ein weiterer Trigger vor der DMA-Rueckmeldung macht die Zuordnung ungueltig.
    if (pendingTrackLevelValid_)
        pendingTrackLevelValid_ = false;
    else
    {
        pendingTrackLevel_ = level;
        pendingTrackLevelValid_ = true;
    }
    __set_PRIMASK(previousMask);
}

void CurrentSensor::updateTrack(uint16_t raw, RailLevel level)
{
    const uint64_t gainShunt = static_cast<uint64_t>(trackGain_) * trackShunt_mOhm_;
    if (raw > 4095 || gainShunt == 0 || adcReference_mV_ == 0 ||
        gainShunt > UINT64_MAX / 4095)
    {
        trackSampleValid_ = false;
        return;
    }
    const uint64_t current =
        (static_cast<uint64_t>(raw) * adcReference_mV_ * 1000) / (4095 * gainShunt);
    if (current > UINT32_MAX)
    {
        trackSampleValid_ = false;
        return;
    }
    trackCurrent_mA_ = static_cast<uint32_t>(current);
    trackOutput_ = level;
    trackSampleValid_ = true;
}

void CurrentSensor::updateSupply(uint16_t raw)
{
    const uint64_t gainShunt = static_cast<uint64_t>(supplyGain_) * supplyShunt_mOhm_;
    if (raw > 4095 || gainShunt == 0 || adcReference_mV_ == 0 ||
        gainShunt > UINT64_MAX / 4095)
    {
        supplySampleValid_ = false;
        return;
    }
    const uint64_t current =
        (static_cast<uint64_t>(raw) * adcReference_mV_ * 1000) / (4095 * gainShunt);
    if (current > UINT32_MAX)
    {
        supplySampleValid_ = false;
        return;
    }
    supplyCurrent_mA_ = static_cast<uint32_t>(current);
    supplySampleValid_ = true;
}

uint32_t CurrentSensor::trackCurrent_mA() const
{
    return trackCurrent_mA_;
}

uint32_t CurrentSensor::supplyCurrent_mA() const
{
    return supplyCurrent_mA_;
}

RailLevel CurrentSensor::trackOutput() const
{
    return trackOutput_;
}

bool CurrentSensor::hasValidTrackSample() const
{
    return trackSampleValid_;
}

bool CurrentSensor::hasValidSupplySample() const
{
    return supplySampleValid_;
}

bool CurrentSensor::takeTrackSample(uint32_t &current_mA, RailLevel &level)
{
    const uint32_t previousMask = __get_PRIMASK();
    __disable_irq();
    const bool available = trackSampleValid_;
    if (available)
    {
        current_mA = trackCurrent_mA_;
        level = trackOutput_;
        trackSampleValid_ = false;
    }
    __set_PRIMASK(previousMask);
    return available;
}

void CurrentSensor::onDmaComplete()
{
    const bool hasLevel = pendingTrackLevelValid_;
    const RailLevel level = pendingTrackLevel_;
    pendingTrackLevelValid_ = false;

    // DMA-Reihenfolge: ADC_IN0 / PA0 ist Versorgung.
    updateSupply(adcBuffer_[0]);
    // ADC_IN1 / PA1 ist Bahn; ohne passenden Trigger-Pegel keine gueltige Probe.
    if (hasLevel)
        updateTrack(adcBuffer_[1], level);
    else
        trackSampleValid_ = false;
}

void CurrentSensor::onAdcError()
{
    trackSampleValid_ = false;
    supplySampleValid_ = false;
    pendingTrackLevelValid_ = false;
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc)
{
    // HAL meldet eine vollstaendige DMA-Sequenz mit beiden Kanaelen.
    if (adc == &hadc && activeSensor != nullptr)
        activeSensor->onDmaComplete();
}

extern "C" void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *adc)
{
    if (adc == &hadc && activeSensor != nullptr)
        activeSensor->onAdcError();
}
