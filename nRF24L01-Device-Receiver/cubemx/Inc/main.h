/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern SPI_HandleTypeDef hspi2;

extern UART_HandleTypeDef huart1;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_SPI2_Init(void);
void MX_USART1_UART_Init(void);
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define nRF24L01_IRQ_Pin GPIO_PIN_5
#define nRF24L01_IRQ_GPIO_Port GPIOC
#define nRF24L01_IRQ_EXTI_IRQn EXTI9_5_IRQn
#define nRF24L01_CSN_Pin GPIO_PIN_0
#define nRF24L01_CSN_GPIO_Port GPIOB
#define nRF24L01_CE_Pin GPIO_PIN_1
#define nRF24L01_CE_GPIO_Port GPIOB
#define nRF24L01_SCK_Pin GPIO_PIN_13
#define nRF24L01_SCK_GPIO_Port GPIOB
#define nRF24L01_MISO_Pin GPIO_PIN_14
#define nRF24L01_MISO_GPIO_Port GPIOB
#define nRF24L01_MOSI_Pin GPIO_PIN_15
#define nRF24L01_MOSI_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
