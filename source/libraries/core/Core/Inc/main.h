/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SUPPLY_SHUNT_Pin GPIO_PIN_0
#define SUPPLY_SHUNT_GPIO_Port GPIOA
#define TRACK_SHUNT_Pin GPIO_PIN_1
#define TRACK_SHUNT_GPIO_Port GPIOA
#define MOXA_TX_Pin GPIO_PIN_2
#define MOXA_TX_GPIO_Port GPIOA
#define MOXA_RX_Pin GPIO_PIN_3
#define MOXA_RX_GPIO_Port GPIOA
#define WIRELESS_CLK_Pin GPIO_PIN_6
#define WIRELESS_CLK_GPIO_Port GPIOA
#define WIRELESS_DATA_Pin GPIO_PIN_7
#define WIRELESS_DATA_GPIO_Port GPIOA
#define CARERRA_DATA_Pin GPIO_PIN_8
#define CARERRA_DATA_GPIO_Port GPIOA
#define IR1_Pin GPIO_PIN_15
#define IR1_GPIO_Port GPIOA
#define IR2_Pin GPIO_PIN_3
#define IR2_GPIO_Port GPIOB
#define IR3_Pin GPIO_PIN_4
#define IR3_GPIO_Port GPIOB
#define IR4_Pin GPIO_PIN_5
#define IR4_GPIO_Port GPIOB
#define IR5_Pin GPIO_PIN_6
#define IR5_GPIO_Port GPIOB
#define IR6_Pin GPIO_PIN_7
#define IR6_GPIO_Port GPIOB
#define IR7_Pin GPIO_PIN_8
#define IR7_GPIO_Port GPIOB
#define IR8_Pin GPIO_PIN_9
#define IR8_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
