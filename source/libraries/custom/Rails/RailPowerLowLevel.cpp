#include "RailPowerLowLevel.h"
#include "main.h"

/**
 * @brief  Initialises the RailPowerLowLevel Library.
 *
 * @note   This function sets the default states of the Rail and also sets the Rail Enable to false,
 *         therefore the Rail is turned off.
 *
 * @retval None
 */
void RailPowerLowLevel::init()
{
    // TURN OFF TRACK
    HAL_GPIO_WritePin(CARERRA_DATA_GPIO_Port, CARERRA_DATA_Pin, GPIO_PIN_RESET);
    this->enabled_ = false;
}

/**
 * @brief  Locks the Rail.
 *
 * @note   This function locks the Rail and prevents the change of State from LOW to HIGH.
 *         Also the Rail is set to LOW.
 *
 * @retval None
 */
void RailPowerLowLevel::lockOutput()
{
    this->enabled_ = false;
    HAL_GPIO_WritePin(CARERRA_DATA_GPIO_Port, CARERRA_DATA_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  Allows the Rail to change State.
 *
 * @note   This function allows the Rail to change State from HIGH to LOW or vice versa.
 *
 * @retval None
 */
void RailPowerLowLevel::allowOutput()
{
    this->enabled_ = true;
}

/**
 * @brief  Return the State of the enabled_ flag.
 *
 * @note   This function returns the state of the enabled_ flag.
 *         If enabled_ is false the Rail is locked and set to LOW.
 *         If enabled_ is true the Rail is able to change state from LOW to HIGH or vice versa.
 *
 * @retval bool enabled_
 */
bool RailPowerLowLevel::isOutputAllowed() const
{
    return this->enabled_;
}

/**
 * @brief  Sets the Track Transistor to conductive or locked.
 *
 * @note   This function sets the Transistor of the power stage to locked or conductive.
 *         Before setting the Transistor to the prefered state.
 *         There is a check where it is made sure that the Rail can be set to conductive.
 *         If not the Rail will be set to locked.
 *
 * @param  state specifies the wanted state of the Rail (HIGH = Conductive, LOW = Locked)
 * @retval None
 */
void RailPowerLowLevel::writeSignalLevel(RailLevel state)
{
    // Saves the interrupt enabled state
    const uint32_t prevMask = __get_PRIMASK();

    // Disabled all interrupts
    __disable_irq();

    // Writes the Pin
    HAL_GPIO_WritePin(
        CARERRA_DATA_GPIO_Port,
        CARERRA_DATA_Pin,
        this->enabled_ && (state == RailLevel::HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // Restores the interrupt enabled state
    __set_PRIMASK(prevMask);
}