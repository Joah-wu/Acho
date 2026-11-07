#include "bsp_flashlight.h"
#include "tim.h"

static bool light_enabled;

void Flashlight_SetEnabled(bool enabled)
{
    uint32_t mode = enabled ? TIM_OCMODE_FORCED_ACTIVE : TIM_OCMODE_FORCED_INACTIVE;

    /* CH4's OC mode is in CCMR2 bits 14:12. Keep CubeMX's forced-output
     * configuration: changing CCR4 would not change the output in this mode. */
    MODIFY_REG(htim4.Instance->CCMR2, TIM_CCMR2_OC4M, mode << 8U);
    light_enabled = enabled;
}

bool Flashlight_IsEnabled(void)
{
    return light_enabled;
}

HAL_StatusTypeDef Flashlight_Init(void)
{
    /* Select inactive before enabling CH4 to avoid a startup flash. */
    Flashlight_SetEnabled(false);
    return HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_4);
}
