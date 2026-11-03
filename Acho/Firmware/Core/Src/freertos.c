/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_fs.h"
#include "lv_port_indev.h"
#include "bsp_key.h"
#include "sdio.h"
#include "ff.h"
#include "fatfs.h"
#include "pages/page_main.h"
#include "gui_setup.h"
#include "fops.h"
#include "bsp_usart.h"
#include "pages/page_audio.h"
#include "WM8960Apps.h"
#include "mlcd.h"
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
/* USER CODE BEGIN PTD */
extern TIM_HandleTypeDef htim3;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
uint32_t defaultTaskBuffer[ 128 ];
osStaticThreadDef_t defaultTaskControlBlock;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .cb_mem = &defaultTaskControlBlock,
  .cb_size = sizeof(defaultTaskControlBlock),
  .stack_mem = &defaultTaskBuffer[0],
  .stack_size = sizeof(defaultTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for GUITask */
osThreadId_t GUITaskHandle;
uint32_t GUITaskBuffer[ 1024 ];
osStaticThreadDef_t GUITaskControlBlock;
const osThreadAttr_t GUITask_attributes = {
  .name = "GUITask",
  .cb_mem = &GUITaskControlBlock,
  .cb_size = sizeof(GUITaskControlBlock),
  .stack_mem = &GUITaskBuffer[0],
  .stack_size = sizeof(GUITaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for audioTask */
osThreadId_t audioTaskHandle;
uint32_t audioTaskBuffer[ 1024 ];
osStaticThreadDef_t audioTaskControlBlock;
const osThreadAttr_t audioTask_attributes = {
  .name = "audioTask",
  .cb_mem = &audioTaskControlBlock,
  .cb_size = sizeof(audioTaskControlBlock),
  .stack_mem = &audioTaskBuffer[0],
  .stack_size = sizeof(audioTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for transferTask */
osThreadId_t transferTaskHandle;
uint32_t transferTaskBuffer[ 1024 ];
osStaticThreadDef_t transferTaskControlBlock;
const osThreadAttr_t transferTask_attributes = {
  .name = "transferTask",
  .cb_mem = &transferTaskControlBlock,
  .cb_size = sizeof(transferTaskControlBlock),
  .stack_mem = &transferTaskBuffer[0],
  .stack_size = sizeof(transferTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for sysCheckTask */
osThreadId_t sysCheckTaskHandle;
uint32_t sysCheckTaskBuffer[ 512 ];
osStaticThreadDef_t sysCheckTaskControlBlock;
const osThreadAttr_t sysCheckTask_attributes = {
  .name = "sysCheckTask",
  .cb_mem = &sysCheckTaskControlBlock,
  .cb_size = sizeof(sysCheckTaskControlBlock),
  .stack_mem = &sysCheckTaskBuffer[0],
  .stack_size = sizeof(sysCheckTaskBuffer),
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartGUITask(void *argument);
void StartAudioTask(void *argument);
void StartTransferTask(void *argument);
void StartSysCheckTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of GUITask */
  GUITaskHandle = osThreadNew(StartGUITask, NULL, &GUITask_attributes);

  /* creation of audioTask */
  audioTaskHandle = osThreadNew(StartAudioTask, NULL, &audioTask_attributes);

  /* creation of transferTask */
  transferTaskHandle = osThreadNew(StartTransferTask, NULL, &transferTask_attributes);

  /* creation of sysCheckTask */
  sysCheckTaskHandle = osThreadNew(StartSysCheckTask, NULL, &sysCheckTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  vTaskSuspend(GUITaskHandle);
  vTaskSuspend(audioTaskHandle);
  vTaskSuspend(transferTaskHandle);
  vTaskSuspend(sysCheckTaskHandle);

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN StartDefaultTask */
    static portTickType PreviousWakeTime;
    const portTickType TimeIncrement = pdMS_TO_TICKS(1000);
    PreviousWakeTime = xTaskGetTickCount();
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_port_fs_init();
    GUIInit();
    HAL_TIM_Encoder_Start(&htim3,TIM_CHANNEL_ALL);
    // HAL_GPIO_WritePin(USB_ENABLE_GPIO_Port, USB_ENABLE_Pin, GPIO_PIN_SET);
    vTaskResume(GUITaskHandle);
    // vTaskResume(audioTaskHandle);
    // vTaskResume(transferTaskHandle);
    // vTaskResume(sysCheckTaskHandle);
    /* Infinite loop */
    for (;;) {
  
        vTaskDelayUntil(&PreviousWakeTime, TimeIncrement);
    }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartGUITask */
/**
* @brief Function implementing the GUITask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartGUITask */
void StartGUITask(void *argument)
{
  /* USER CODE BEGIN StartGUITask */
  static portTickType PreviousWakeTime;
  const portTickType TimeIncrement = pdMS_TO_TICKS(5);
  FRESULT mount_result;
  PreviousWakeTime = xTaskGetTickCount();

  mount_result = f_mount(&SDFatFS, SDPath, 1);
  Printf("SD mount: %s (%d)\r\n",
         (mount_result == FR_OK) ? "OK" : "failed",
         (int)mount_result);
  exf_showfree((uint8_t*)"0:");
  setup_ui(&ui_main, &ui_ROOT, NULL);
  /* Infinite loop */
  for(;;)
  {
    lv_task_handler();
    mlcd_Service();
    vTaskDelayUntil(&PreviousWakeTime, TimeIncrement);
  }
  /* USER CODE END StartGUITask */
}

/* USER CODE BEGIN Header_StartAudioTask */
/**
* @brief Function implementing the audioTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAudioTask */
void StartAudioTask(void *argument)
{
  /* USER CODE BEGIN StartAudioTask */
  static portTickType PreviousWakeTime;
  const portTickType TimeIncrement = pdMS_TO_TICKS(100);
  PreviousWakeTime = xTaskGetTickCount();
  
  /* Infinite loop */
  for(;;)
  {
    PlayWaveFile(global_BUFF);
    vTaskDelayUntil(&PreviousWakeTime, TimeIncrement);
  }
  /* USER CODE END StartAudioTask */
}

/* USER CODE BEGIN Header_StartTransferTask */
/**
* @brief Function implementing the TransferTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTransferTask */
void StartTransferTask(void *argument)
{
  /* USER CODE BEGIN StartTransferTask */
  static portTickType PreviousWakeTime;
  const portTickType TimeIncrement = pdMS_TO_TICKS(100);
  PreviousWakeTime = xTaskGetTickCount();
  /* Infinite loop */
  for(;;)
  {
    USART1_REC_Handler();
    vTaskDelayUntil(&PreviousWakeTime, TimeIncrement);
  }
  /* USER CODE END StartTransferTask */
}

/* USER CODE BEGIN Header_StartSysCheckTask */
/**
* @brief Function implementing the sysCheckTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSysCheckTask */
void StartSysCheckTask(void *argument)
{
  /* USER CODE BEGIN StartSysCheckTask */
  static portTickType PreviousWakeTime;
  const portTickType TimeIncrement = pdMS_TO_TICKS(100);
  PreviousWakeTime = xTaskGetTickCount();
  /* Infinite loop */
  for(;;)
  {
    vTaskDelayUntil(&PreviousWakeTime, TimeIncrement);
  }
  /* USER CODE END StartSysCheckTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

