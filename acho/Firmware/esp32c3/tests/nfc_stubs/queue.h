#pragma once
#include "FreeRTOS.h"
QueueHandle_t xQueueCreateStatic(unsigned,size_t,uint8_t*,StaticQueue_t*);
int xQueueOverwrite(QueueHandle_t,const void*);
int xQueueReceive(QueueHandle_t,void*,TickType_t);
