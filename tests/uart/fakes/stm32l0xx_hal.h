#pragma once

#include <stdint.h>

enum HAL_StatusTypeDef
{
    HAL_OK,
    HAL_ERROR,
    HAL_BUSY
};

struct UART_HandleTypeDef {};

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *handle, uint8_t *data, uint16_t size);
uint32_t __get_PRIMASK();
void __disable_irq();
void __set_PRIMASK(uint32_t mask);
