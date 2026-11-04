#ifndef __WM8960_H__
#define __WM8960_H__

#include "stm32f4xx_hal.h"
#include "simple_gpio.h"
#define IIC_SCL    PBout(8) //SCL
#define IIC_SDA    PBout(7) //SDA
#define READ_SDA   PBin(7)  //SDA_IN


enum {
    WM8960_MODE_DAC_ENABLE=1,
    WM8960_MODE_ADC_ENABLE
};


uint8_t WM8960_Write_Reg(uint8_t reg, uint16_t dat);
uint16_t WM8960_Read_Reg(uint8_t reg);
uint8_t WM89060_Init(uint8_t mode);
uint8_t WM8960_SetSampleRate(uint32_t samplerate);
uint8_t WM8960_SetVolume(uint8_t percent);
uint8_t WM8960_SetMute(uint8_t mute);
uint8_t WM8960_SetSpeakerEnable(uint8_t enable);
uint8_t WM8960_PowerDown(void);




#endif
