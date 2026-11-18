#include "RailPower.h"
#include "main.h"

/**
 * @brief  Initialises the RailPower library.
 *
 * @note   Initialises the low-level output and sets the RailPower state to OFF.
 *         The Rail remains turned off.
 *
 * @retval None
 */
void RailPower::init()
{
    lowLevel_.init();
    state_ = RailPowerState::OFF;
}

/**
 * @brief  Enables the Rail output.
 *
 * @note   Allows the signal level to be written to the Rail and sets the state to ON.
 *         If a short circuit is stored, the Rail remains locked.
 *         This function does not set the output to HIGH by itself.
 *
 * @retval None
 */
void RailPower::enable()
{
    const uint32_t previousMask = __get_PRIMASK();
    __disable_irq();

    if (state_ != RailPowerState::SHORT_CIRCUIT)
    {
        lowLevel_.allowOutput();
        state_ = RailPowerState::ON;
    }

    __set_PRIMASK(previousMask);
}

/**
 * @brief  Disables the Rail output.
 *
 * @note   Locks the output and sets the Rail to LOW.
 *         A stored short-circuit state remains active.
 *
 * @retval None
 */
void RailPower::disable()
{
    const uint32_t previousMask = __get_PRIMASK();
    __disable_irq();

    lowLevel_.lockOutput();

    if (state_ != RailPowerState::SHORT_CIRCUIT)
    {
        state_ = RailPowerState::OFF;
    }

    __set_PRIMASK(previousMask);
}

/**
 * @brief  Resets the stored short-circuit state.
 *
 * @note   Changes the state from SHORT_CIRCUIT to OFF.
 *         The Rail remains locked and must be enabled separately.
 *         This function does not check whether the physical short circuit is gone.
 *
 * @retval None
 */
void RailPower::resetFault()
{
    const uint32_t previousMask = __get_PRIMASK();
    __disable_irq();

    if (state_ == RailPowerState::SHORT_CIRCUIT)
    {
        lowLevel_.lockOutput();
        state_ = RailPowerState::OFF;
    }

    __set_PRIMASK(previousMask);
}

/**
 * @brief  Applies the requested signal level to the Rail.
 *
 * @note   Passes the level to RailPowerLowLevel, which checks whether
 *         the output is allowed before writing to the pin.
 *
 * @param  level specifies the requested signal level.
 * @retval None
 */
void RailPower::applySignalLevel(RailLevel level)
{
    lowLevel_.writeSignalLevel(level);
}

/**
 * @brief  Returns the current RailPower state.
 *
 * @retval RailPowerState current state: OFF, ON or SHORT_CIRCUIT.
 */
RailPowerState RailPower::state() const
{
    return state_;
}

/**
 * @brief  Locks the Rail output after a short circuit.
 *
 * @note   Sets the Rail to LOW and stores the SHORT_CIRCUIT state.
 *         The output remains locked until resetFault() is called.
 *
 * @retval None
 */
void RailPower::onShortCircuit()
{
    lowLevel_.lockOutput();
    state_ = RailPowerState::SHORT_CIRCUIT;
}