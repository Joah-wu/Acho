#include "bsp_buzzer.h"
#include "tim.h"
#include <stdbool.h>

#define BUZZER_COUNTER_HZ  1000000U
#define BUZZER_TONE_HZ        4000U
#define BUZZER_STEP_MS          60U
#define BUZZER_PERIOD      (BUZZER_COUNTER_HZ / BUZZER_TONE_HZ)

static volatile uint16_t remaining_ms;
static volatile uint8_t remaining_steps;
static bool initialized;

HAL_StatusTypeDef Buzzer_Init(void)
{
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq();

    /* CubeMX uses TIMPRE=0: APB1 timers run at 2 * PCLK1 when divided. */
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U) timer_clock *= 2U;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, 0U);
    __HAL_TIM_SET_PRESCALER(&htim2, timer_clock / BUZZER_COUNTER_HZ - 1U);
    __HAL_TIM_SET_AUTORELOAD(&htim2, BUZZER_PERIOD - 1U);
    __HAL_TIM_SET_COUNTER(&htim2, 0U);
    /* Load the prescaler and preload registers before starting PWM. */
    if (HAL_TIM_GenerateEvent(&htim2, TIM_EVENTSOURCE_UPDATE) != HAL_OK) return HAL_ERROR;
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4) != HAL_OK) return HAL_ERROR;
    initialized = true;
    return HAL_OK;
}

static void play(uint8_t steps)
{
    uint32_t saved_primask;
    if (!initialized) return;

    /* Protect the sequence from TIM1, including when called inside a task's
     * FreeRTOS critical section (BASEPRI alone does not mask every IRQ). */
    saved_primask = __get_PRIMASK();
    __disable_irq();
    remaining_steps = steps;
    remaining_ms = BUZZER_STEP_MS;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, BUZZER_PERIOD / 2U);
    __set_PRIMASK(saved_primask);
}

void Buzzer_ShortBeep(void)
{
    play(1U);
}

void Buzzer_DoubleBeep(void)
{
    play(3U); /* on, gap, on */
}

void Buzzer_Tick1ms(void)
{
    if (remaining_ms == 0U) return;
    if (--remaining_ms != 0U) return;

    --remaining_steps;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4,
                          (remaining_steps & 1U) ? BUZZER_PERIOD / 2U : 0U);
    if (remaining_steps != 0U) remaining_ms = BUZZER_STEP_MS;
}
