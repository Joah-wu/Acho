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
#include "usbd_def.h"
#include "usbd_core.h"
#include "sdio.h"
#include "ff.h"
#include "fatfs.h"
#include "pages/page_main.h"
#include "gui_setup.h"
#include "fops.h"
#include "bsp_usart.h"
#include "pages/page_audio.h"
#include "WM8960Apps.h"
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
extern TIM_HandleTypeDef htim4;
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
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
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
/* Definitions for dataTransferTas */
osThreadId_t dataTransferTasHandle;
uint32_t dataTransferTasBuffer[ 1024 ];
osStaticThreadDef_t dataTransferTasControlBlock;
const osThreadAttr_t dataTransferTas_attributes = {
  .name = "dataTransferTas",
  .cb_mem = &dataTransferTasControlBlock,
  .cb_size = sizeof(dataTransferTasControlBlock),
  .stack_mem = &dataTransferTasBuffer[0],
  .stack_size = sizeof(dataTransferTasBuffer),
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for syscheckTask */
osThreadId_t syscheckTaskHandle;
uint32_t syscheckTaskBuffer[ 512 ];
osStaticThreadDef_t syscheckTaskControlBlock;
const osThreadAttr_t syscheckTask_attributes = {
  .name = "syscheckTask",
  .cb_mem = &syscheckTaskControlBlock,
  .cb_size = sizeof(syscheckTaskControlBlock),
  .stack_mem = &syscheckTaskBuffer[0],
  .stack_size = sizeof(syscheckTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal3,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartGUITask(void *argument);
void StartAudioTask(void *argument);
void StartDataTransferTask(void *argument);
void StartSyscheckTask(void *argument);

extern void MX_USB_HOST_Init(void);
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

  /* creation of dataTransferTas */
  dataTransferTasHandle = osThreadNew(StartDataTransferTask, NULL, &dataTransferTas_attributes);

  /* creation of syscheckTask */
  syscheckTaskHandle = osThreadNew(StartSyscheckTask, NULL, &syscheckTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
    vTaskSuspend(mainTaskHandle);
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
  /* init code for USB_HOST */
  MX_USB_HOST_Init();
  /* USER CODE BEGIN StartDefaultTask */
    static portTickType PreviousWakeTime;
    const portTickType TimeIncrement = pdMS_TO_TICKS(5);
    PreviousWakeTime = xTaskGetTickCount();
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_port_fs_init();
    GUIInit();
    HAL_TIM_Encoder_Start(&htim4,TIM_CHANNEL_ALL);
    vTaskResume(mainTaskHandle);
    HAL_GPIO_WritePin(USB_EN_GPIO_Port, USB_EN_Pin, GPIO_PIN_SET);
    /* Infinite loop */
    for (;;) {
//        if(audio_play_state==AUDIO_PLAY){
//            vTaskDelay(100);
//            audio_play((uint8_t *) global_BUFF, &audio_play_state);
//        }
        USART1_REC_Handler();
        PlayWaveFile(global_BUFF);
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
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
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
  const portTickType TimeIncrement = pdMS_TO_TICKS(5);
  PreviousWakeTime = xTaskGetTickCount();
  setup_ui(&ui_main, &ui_ROOT, NULL);
  /* Infinite loop */
  for(;;)
  {
    lv_task_handler();
    vTaskDelayUntil(&PreviousWakeTime, TimeIncrement);
  }
  /* USER CODE END StartAudioTask */
}

/* USER CODE BEGIN Header_StartDataTransferTask */
/**
* @brief Function implementing the dataTransferTas thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDataTransferTask */
void StartDataTransferTask(void *argument)
{
  /* USER CODE BEGIN StartDataTransferTask */
  static portTickType PreviousWakeTime;
  const portTickType TimeIncrement = pdMS_TO_TICKS(100);
  PreviousWakeTime = xTaskGetTickCount();
  /* Infinite loop */
  for(;;)
  {
    vTaskDelayUntil(&PreviousWakeTime, TimeIncrement);
  }
  /* USER CODE END StartDataTransferTask */
}

/* USER CODE BEGIN Header_StartSyscheckTask */
/**
* @brief Function implementing the syscheckTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSyscheckTask */
void StartSyscheckTask(void *argument)
{
  /* USER CODE BEGIN StartSyscheckTask */
  static portTickType PreviousWakeTime;
  const portTickType TimeIncrement = pdMS_TO_TICKS(100);
  PreviousWakeTime = xTaskGetTickCount();
  /* Infinite loop */
  for(;;)
  {
    vTaskDelayUntil(&PreviousWakeTime, TimeIncrement);
  }
  /* USER CODE END StartSyscheckTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

