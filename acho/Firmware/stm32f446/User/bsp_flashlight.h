#ifndef BSP_FLASHLIGHT_H
#define BSP_FLASHLIGHT_H

#include <stdbool.h>
#include "stm32f4xx_hal.h"

/* Call once after MX_TIM4_Init(). The light starts off. */
HAL_StatusTypeDef Flashlight_Init(void);
/* GUI/task context only. PB9 is active high. */
void Flashlight_SetEnabled(bool enabled);
bool Flashlight_IsEnabled(void);

#endif
