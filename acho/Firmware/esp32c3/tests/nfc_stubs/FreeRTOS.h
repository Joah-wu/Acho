#pragma once
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
typedef uint32_t TickType_t;
typedef uint32_t StackType_t;
typedef int StaticTask_t;
typedef int StaticQueue_t;
typedef void *TaskHandle_t;
typedef void *QueueHandle_t;
#define pdMS_TO_TICKS(x) ((TickType_t)(x))
#define pdTRUE 1
#define tskIDLE_PRIORITY 0
#define configASSERT(x) assert(x)
#define taskENTER_CRITICAL() ((void)0)
#define taskEXIT_CRITICAL() ((void)0)
