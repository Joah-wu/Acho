#pragma once
#include <stdint.h>
typedef struct { unsigned unused; } I2C_HandleTypeDef;
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
extern I2C_HandleTypeDef hi2c2;
#define I2C_MEMADD_SIZE_16BIT 2U
uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *,uint16_t,uint32_t,uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *,uint16_t,uint16_t,uint16_t,uint8_t *,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *,uint16_t,uint16_t,uint16_t,uint8_t *,uint16_t,uint32_t);
uint32_t HAL_I2C_GetError(I2C_HandleTypeDef *);
uint32_t HAL_I2C_GetState(I2C_HandleTypeDef *);
GPIO_PinState HAL_GPIO_ReadPin(void *,uint16_t);
#define ST25D_I2C_SCL_GPIO_Port ((void *)1)
#define ST25D_I2C_SDA_GPIO_Port ((void *)1)
#define ST25D_I2C_SCL_Pin 1024U
#define ST25D_I2C_SDA_Pin 8U
