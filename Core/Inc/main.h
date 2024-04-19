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
#define POWER_SW_Pin GPIO_PIN_1
#define POWER_SW_GPIO_Port GPIOF
#define START_SW_Pin GPIO_PIN_0
#define START_SW_GPIO_Port GPIOA
#define BOOT_SW_Pin GPIO_PIN_1
#define BOOT_SW_GPIO_Port GPIOA
#define START_LED_Pin GPIO_PIN_2
#define START_LED_GPIO_Port GPIOA
#define KILL_SW_Pin GPIO_PIN_3
#define KILL_SW_GPIO_Port GPIOA
#define BOOTING_LED_Pin GPIO_PIN_4
#define BOOTING_LED_GPIO_Port GPIOA
#define BOOT_ERR_LED_Pin GPIO_PIN_5
#define BOOT_ERR_LED_GPIO_Port GPIOA
#define BOOTED_LED_Pin GPIO_PIN_6
#define BOOTED_LED_GPIO_Port GPIOA
#define RED_ZONE_LED_Pin GPIO_PIN_7
#define RED_ZONE_LED_GPIO_Port GPIOA
#define BLUE_ZONE_LED_Pin GPIO_PIN_0
#define BLUE_ZONE_LED_GPIO_Port GPIOB
#define S_OR_R_SW_Pin GPIO_PIN_8
#define S_OR_R_SW_GPIO_Port GPIOA
#define ZONE_SW_Pin GPIO_PIN_9
#define ZONE_SW_GPIO_Port GPIOA
#define BTN_ID_Pin GPIO_PIN_10
#define BTN_ID_GPIO_Port GPIOA
#define LED_ERR_Pin GPIO_PIN_15
#define LED_ERR_GPIO_Port GPIOA
#define LED_ID_Pin GPIO_PIN_4
#define LED_ID_GPIO_Port GPIOB
#define LED_TX_Pin GPIO_PIN_5
#define LED_TX_GPIO_Port GPIOB
#define LED_RX_Pin GPIO_PIN_6
#define LED_RX_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
