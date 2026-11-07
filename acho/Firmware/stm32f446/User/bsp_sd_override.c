/**
 * @file bsp_sd_override.c
 * @brief Board-specific SD card initialization.
 *
 * Initialize in 1-bit mode for card identification, then enable the 4-bit
 * SDIO data bus.
 *
 * BSP_SD_Init in FATFS/Target/bsp_driver_sd.c is weak; this strong definition
 * deliberately overrides it and survives CubeMX code generation.
 */

#include "bsp_driver_sd.h"
#include "sdio.h"

uint8_t BSP_SD_Init(void)
{
  if (BSP_SD_IsDetected() != SD_PRESENT)
  {
    return MSD_ERROR;
  }

  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;

  if (HAL_SD_Init(&hsd) != HAL_OK)
  {
    return MSD_ERROR;
  }

  return (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) == HAL_OK)
             ? MSD_OK
             : MSD_ERROR;
}
