/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32g4xx_hal.h"

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
#define SW_START_Pin GPIO_PIN_1
#define SW_START_GPIO_Port GPIOF
#define SW_24V_Pin GPIO_PIN_0
#define SW_24V_GPIO_Port GPIOA
#define SW_BOOT_Pin GPIO_PIN_1
#define SW_BOOT_GPIO_Port GPIOA
#define LED_START_Pin GPIO_PIN_2
#define LED_START_GPIO_Port GPIOA
#define SW_KILL_Pin GPIO_PIN_3
#define SW_KILL_GPIO_Port GPIOA
#define LED_BOOT_ERR_Pin GPIO_PIN_4
#define LED_BOOT_ERR_GPIO_Port GPIOA
#define LED_BOOTING_Pin GPIO_PIN_5
#define LED_BOOTING_GPIO_Port GPIOA
#define LED_RUNNING_Pin GPIO_PIN_6
#define LED_RUNNING_GPIO_Port GPIOA
#define LED_RED_ZONE_Pin GPIO_PIN_7
#define LED_RED_ZONE_GPIO_Port GPIOA
#define LED_BLUE_ZONE_Pin GPIO_PIN_0
#define LED_BLUE_ZONE_GPIO_Port GPIOB
#define SW_RETRY_Pin GPIO_PIN_8
#define SW_RETRY_GPIO_Port GPIOA
#define SW_ZONE_Pin GPIO_PIN_9
#define SW_ZONE_GPIO_Port GPIOA
#define LED_ERR_Pin GPIO_PIN_15
#define LED_ERR_GPIO_Port GPIOA
#define LED_RETRY_Pin GPIO_PIN_4
#define LED_RETRY_GPIO_Port GPIOB
#define LED_TX_Pin GPIO_PIN_5
#define LED_TX_GPIO_Port GPIOB
#define LED_RX_Pin GPIO_PIN_6
#define LED_RX_GPIO_Port GPIOB
#define SW_AREA_Pin GPIO_PIN_7
#define SW_AREA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
