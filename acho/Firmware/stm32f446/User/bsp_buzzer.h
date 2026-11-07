#ifndef BSP_BUZZER_H
#define BSP_BUZZER_H

#include "stm32f4xx_hal.h"

/* Call once after MX_TIM2_Init(), before enabling input. */
HAL_StatusTypeDef Buzzer_Init(void);
/* Non-blocking, task-context requests. A new request replaces the old cue. */
void Buzzer_ShortBeep(void);
void Buzzer_DoubleBeep(void);
/* Called only by the existing TIM1 1 ms interrupt. */
void Buzzer_Tick1ms(void);

#endif
