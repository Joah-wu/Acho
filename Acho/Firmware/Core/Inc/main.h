/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

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
#define BAT_VOTAGE_ADC_Pin GPIO_PIN_0
#define BAT_VOTAGE_ADC_GPIO_Port GPIOC
#define WM8960_HP_ADC_Pin GPIO_PIN_1
#define WM8960_HP_ADC_GPIO_Port GPIOC
#define TX_HOLD_Pin GPIO_PIN_2
#define TX_HOLD_GPIO_Port GPIOC
#define RX_HOLD_Pin GPIO_PIN_3
#define RX_HOLD_GPIO_Port GPIOC
#define READY_Pin GPIO_PIN_0
#define READY_GPIO_Port GPIOA
#define WIFI_EN_Pin GPIO_PIN_1
#define WIFI_EN_GPIO_Port GPIOA
#define WIFI_UART_RX_Pin GPIO_PIN_2
#define WIFI_UART_RX_GPIO_Port GPIOA
#define WIFI_UART_TX_Pin GPIO_PIN_3
#define WIFI_UART_TX_GPIO_Port GPIOA
#define MLCD_DISP_Pin GPIO_PIN_4
#define MLCD_DISP_GPIO_Port GPIOA
#define MLCD_CLK_Pin GPIO_PIN_5
#define MLCD_CLK_GPIO_Port GPIOA
#define MLCD_MOSI_Pin GPIO_PIN_7
#define MLCD_MOSI_GPIO_Port GPIOA
#define MLCD_CS_Pin GPIO_PIN_4
#define MLCD_CS_GPIO_Port GPIOC
#define MLCD_EXTCOMN_Pin GPIO_PIN_5
#define MLCD_EXTCOMN_GPIO_Port GPIOC
#define MLCD_MODE_Pin GPIO_PIN_0
#define MLCD_MODE_GPIO_Port GPIOB
#define BEEP_Pin GPIO_PIN_2
#define BEEP_GPIO_Port GPIOB
#define STD25_I2C_SCL_Pin GPIO_PIN_10
#define STD25_I2C_SCL_GPIO_Port GPIOB
#define WM8960_I2S_LRCLK_Pin GPIO_PIN_12
#define WM8960_I2S_LRCLK_GPIO_Port GPIOB
#define WM8960_I2S_BCLK_Pin GPIO_PIN_13
#define WM8960_I2S_BCLK_GPIO_Port GPIOB
#define SPK_EN_Pin GPIO_PIN_14
#define SPK_EN_GPIO_Port GPIOB
#define WM8960_I2S_DAT_Pin GPIO_PIN_15
#define WM8960_I2S_DAT_GPIO_Port GPIOB
#define WM8960_I2S_MCLK_Pin GPIO_PIN_6
#define WM8960_I2S_MCLK_GPIO_Port GPIOC
#define USB_DETECT_Pin GPIO_PIN_7
#define USB_DETECT_GPIO_Port GPIOC
#define USB_ENABLE_Pin GPIO_PIN_8
#define USB_ENABLE_GPIO_Port GPIOA
#define DAP_UART_RX_Pin GPIO_PIN_9
#define DAP_UART_RX_GPIO_Port GPIOA
#define DAP_UART_TX_Pin GPIO_PIN_10
#define DAP_UART_TX_GPIO_Port GPIOA
#define USB_N_Pin GPIO_PIN_11
#define USB_N_GPIO_Port GPIOA
#define USB_P_Pin GPIO_PIN_12
#define USB_P_GPIO_Port GPIOA
#define SDIO_DETECT_Pin GPIO_PIN_15
#define SDIO_DETECT_GPIO_Port GPIOA
#define ENCODER_TIM3_CH1_Pin GPIO_PIN_4
#define ENCODER_TIM3_CH1_GPIO_Port GPIOB
#define ENCODER_TIM3_CH2_Pin GPIO_PIN_5
#define ENCODER_TIM3_CH2_GPIO_Port GPIOB
#define ENCODER_ENTER_Pin GPIO_PIN_6
#define ENCODER_ENTER_GPIO_Port GPIOB
#define WM8960_I2C_SDA_Pin GPIO_PIN_7
#define WM8960_I2C_SDA_GPIO_Port GPIOB
#define WM8950_I2C_SCL_Pin GPIO_PIN_8
#define WM8950_I2C_SCL_GPIO_Port GPIOB
#define LED_Pin GPIO_PIN_9
#define LED_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
