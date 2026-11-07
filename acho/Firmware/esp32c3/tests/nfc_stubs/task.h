#pragma once
#include "FreeRTOS.h"
void vTaskDelay(TickType_t);
TaskHandle_t xTaskCreateStatic(void(*)(void*),const char*,uint32_t,void*,unsigned,StackType_t*,StaticTask_t*);
