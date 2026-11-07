#pragma once
#include "FreeRTOS.h"
typedef void *QueueHandle_t;
typedef struct { int unused; } StaticQueue_t;
static inline QueueHandle_t xQueueCreateStatic(int n,size_t size,uint8_t *data,StaticQueue_t *q) {
    (void)n;(void)size;(void)data;return q;
}
static inline int xQueueSend(QueueHandle_t q,const void *item,TickType_t ticks) {
    (void)q;(void)item;(void)ticks;return 1;
}
static inline int xQueueReceive(QueueHandle_t q,void *item,TickType_t ticks) {
    (void)q;(void)item;(void)ticks;return 0;
}
