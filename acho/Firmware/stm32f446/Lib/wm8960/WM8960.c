#include "WM8960.h"
#include "i2s.h"
#include "stdio.h"
#include <string.h>

#define WM8960_ADDRESS  0x34
#define USE_BOARD_MIC

static void delay_us(uint32_t nus)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t cycles = nus * (SystemCoreClock / 1000000U);
  while((uint32_t)(DWT->CYCCNT - start) < cycles) { __NOP(); }
}
void IIC_Init(void)
{
  GPIO_InitTypeDef GPIO_Initure = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();   //GPIOB CLOCK ENABLE

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  // PB8 = SCL, PB7 = SDA on the Acho motherboard.
  GPIO_Initure.Pin=WM8960_I2C_SCL_Pin|WM8960_I2C_SDA_Pin;
  GPIO_Initure.Mode=GPIO_MODE_OUTPUT_OD;
  GPIO_Initure.Pull=GPIO_NOPULL;
  GPIO_Initure.Speed=GPIO_SPEED_HIGH;
  HAL_GPIO_Init(GPIOB,&GPIO_Initure);

  IIC_SDA=1;
  IIC_SCL=1;
}
void SDA_OUT(void)
{
  GPIO_InitTypeDef GPIO_Initure = {0};

  GPIO_Initure.Pin=WM8960_I2C_SDA_Pin;
  GPIO_Initure.Mode=GPIO_MODE_OUTPUT_OD;
  GPIO_Initure.Pull=GPIO_NOPULL;
  GPIO_Initure.Speed=GPIO_SPEED_HIGH;
  HAL_GPIO_Init(GPIOB,&GPIO_Initure);
}
void SDA_IN(void)
{
  GPIO_InitTypeDef GPIO_Initure = {0};
  GPIO_Initure.Pin=WM8960_I2C_SDA_Pin;
  GPIO_Initure.Mode=GPIO_MODE_INPUT;
  GPIO_Initure.Pull=GPIO_NOPULL;
  GPIO_Initure.Speed=GPIO_SPEED_HIGH;
  HAL_GPIO_Init(GPIOB,&GPIO_Initure);
}
void IIC_Start(void)
{
  SDA_OUT();
  IIC_SDA=1;
  delay_us(2);
  IIC_SCL=1;
  delay_us(2);
  IIC_SDA=0;
  delay_us(2);
  IIC_SCL=0;
  delay_us(2);
}
void IIC_Stop(void)
{
  SDA_OUT();
  IIC_SCL=0;
  delay_us(2);
  IIC_SDA=0;
  delay_us(2);
  IIC_SCL=1;
  delay_us(2);
  IIC_SDA=1;
  delay_us(2);
}
uint8_t IIC_Wait_Ack(void)
{
  SDA_IN();
  delay_us(5);
  IIC_SCL=1;
  delay_us(5);
  if(!READ_SDA)
  {
    IIC_SCL=0;
    return 0;
  }
  IIC_SCL=0;
  return 1;
}
void IIC_Ack(void)
{
  delay_us(3);
  SDA_OUT();
  IIC_SDA=0;
  delay_us(3);
  IIC_SCL=1;
  delay_us(5);
  IIC_SCL=0;
}
void IIC_NAck(void)
{
  delay_us(3);
  SDA_OUT();
  IIC_SDA=1;
  delay_us(3);
  IIC_SCL=1;
  delay_us(5);
  IIC_SCL=0;
}
void IIC_Send_Byte(uint8_t data)
{
  uint8_t i = 8;

  SDA_OUT();
  while(i--)
  {
    IIC_SCL=0;
    //delay_us(9);
    if(data & 0x80)
    {
      IIC_SDA=1;
    }
    else
    {
      IIC_SDA=0;
    }

    delay_us(5);
    data <<= 1;
    IIC_SCL=1;
    delay_us(5);
    IIC_SCL=0;
    delay_us(1);
  }
}

/**
  * @brief send one byte command with parameters
  * return value 3,2,1: failed ,0:success
  * Send the WM8960 write address (0x34), then its register/data bytes.
  */
uint8_t IIC_Write_Bytes(const uint8_t *pdata, uint16_t num)
{
  uint16_t i;
  IIC_Start();
  IIC_Send_Byte(WM8960_ADDRESS);
  if(IIC_Wait_Ack() == 1)
  {
    IIC_Stop();
    return 1;
  }
  delay_us(30);

  for(i = 0; i < num; i++)
  {
    IIC_Send_Byte(*pdata++);
    if(IIC_Wait_Ack() == 1)
    {
      IIC_Stop();
      return 3;
    }
    delay_us(50);
  }
  IIC_Stop();
  return 0;
}

/* The two-wire interface is write-only; retain a shadow for masked updates. */
static const uint16_t WM8960_REG_DEFAULT[56] =
{  
  0x0097, 0x0097, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x000A,
  0x01C0, 0x0000, 0x00FF, 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x007B, 0x0100, 0x0032, 0x0000, 0x00C3, 0x00C3, 0x01C0,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0100, 0x0100, 0x0050, 0x0050, 0x0050, 0x0050, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0040, 0x0000, 0x0000, 0x0050, 0x0050, 0x0000,
  0x0000, 0x0037, 0x004D, 0x0080, 0x0008, 0x0031, 0x0026, 0x00ED
};

static uint16_t WM8960_REG_VAL[56];

/**
  * @brief  Write register of WM8960.
  * @param  reg: The number of resigter which to be read.
  * @param  dat: The data which will be writeen to the register.
  * @retval The value of regsiter.
  */
uint8_t WM8960_Write_Reg(uint8_t reg, uint16_t dat)  {
  if(reg >= 56U || dat > 0x01FFU) {
    printf("WM8960 invalid write: reg=0x%02X value=0x%03X\r\n",
           (unsigned int)reg, (unsigned int)dat);
    return 1;
  }
  
  uint8_t res,I2C_Data[2];

  I2C_Data[0] = (reg<<1)|((uint8_t)((dat>>8)&0x0001));  //RegAddr
  I2C_Data[1] = (uint8_t)(dat&0x00FF);                  //RegValue

  res = IIC_Write_Bytes(I2C_Data,2);
  if(res == HAL_OK) {
    if(reg == 0x0FU) memcpy(WM8960_REG_VAL, WM8960_REG_DEFAULT, sizeof(WM8960_REG_VAL));
    else WM8960_REG_VAL[reg] = dat;
  } else {
    printf("WM8960 I2C failed: addr=0x1A reg=0x%02X value=0x%03X status=%u (%s)\r\n",
           (unsigned int)reg, (unsigned int)dat, (unsigned int)res,
           res == 1U ? "address NACK" : "data NACK");
  }

  return res;
}

/**
  * @brief  Read the local WM8960 register shadow.
  * @param  reg: The number of resigter which to be read.
  * @retval The value of regsiter.
  */
uint16_t WM8960_Read_Reg(uint8_t reg) {
  
  return (reg < 56U) ? WM8960_REG_VAL[reg] : 0U;
}


/**
  * @brief  Initialize WM8960 device.
  * @param  mode: DAC_ENABLE or ADC_ENABLE (shared SD wire, half duplex).
  * @retval res :Status
  */
/* Propagate every failed write: this codec has a write-only control interface. */
#define WM_TRY_WRITE(reg, value) do { \
    uint8_t status = WM8960_Write_Reg((reg), (value)); \
    if(status != 0U) return status; \
} while(0)

uint8_t WM89060_Init(uint8_t mode)
{
    if(mode != WM8960_MODE_DAC_ENABLE && mode != WM8960_MODE_ADC_ENABLE) return 1;
    IIC_Init();
    WM_TRY_WRITE(0x0F, 0);
    /* Slave, half duplex. DACDAT and ADCDAT share the motherboard data wire. */
    WM_TRY_WRITE(0x18, (1U << 6) | ((mode == WM8960_MODE_DAC_ENABLE) ? (1U << 3) : 0U));
    WM_TRY_WRITE(0x19, (1U << 7) | (1U << 6) |
                 ((mode == WM8960_MODE_ADC_ENABLE) ? 0x3EU : 0U));
    WM_TRY_WRITE(0x2F, (mode == WM8960_MODE_DAC_ENABLE) ? 0x0CU : 0x30U);
    WM_TRY_WRITE(0x04, 0);       /* SYSCLK=MCLK, ADC/DAC divide by 256. */
    WM_TRY_WRITE(0x07, 0x0002);  /* I2S, 16-bit, codec slave. */
    WM_TRY_WRITE(0x05, WM8960_REG_VAL[0x05] | (1U << 3)); /* Start muted. */

    if(mode == WM8960_MODE_DAC_ENABLE) {
        WM_TRY_WRITE(0x1A, 0x01FA);
        WM_TRY_WRITE(0x02, 0x01EF);
        WM_TRY_WRITE(0x03, 0x01EF);
        WM_TRY_WRITE(0x28, 0x01FA);
        WM_TRY_WRITE(0x29, 0x01FA);
        /* Enable Class-D only after clocks, power and DMA have been prepared. */
        WM_TRY_WRITE(0x31, WM8960_REG_VAL[0x31] & ~0x00C0U);
        WM_TRY_WRITE(0x0A, 0x01FF);
        WM_TRY_WRITE(0x0B, 0x01FF);
        WM_TRY_WRITE(0x22, 0x0100);
        WM_TRY_WRITE(0x25, 0x0100);
    } else {
        WM_TRY_WRITE(0x00, 0x013F);
        WM_TRY_WRITE(0x01, 0x013F);
#ifdef USE_BOARD_MIC
        WM_TRY_WRITE(0x20, 0x0108);
        WM_TRY_WRITE(0x21, 0);
        WM_TRY_WRITE(0x17, 1U << 2);
#else
        WM_TRY_WRITE(0x20, 0);
        WM_TRY_WRITE(0x21, 0x0108);
        WM_TRY_WRITE(0x17, 1U << 3);
#endif
        WM_TRY_WRITE(0x2B, 0);
        WM_TRY_WRITE(0x2C, 0);
        WM_TRY_WRITE(0x15, 0x01EA);
        WM_TRY_WRITE(0x16, 0x01EA);
        WM_TRY_WRITE(0x14, 0x00F9);
    }
    WM_TRY_WRITE(0x17, WM8960_REG_VAL[0x17] | 0x01C3);
    WM_TRY_WRITE(0x30, 0x000B);  /* JD2, MICBIAS=0.65*AVDD, thermal sensor. */
    return 0;
}

uint8_t WM8960_SetSampleRate(uint32_t samplerate)
{
    uint16_t class_d_div;
    if(samplerate == 16000U) class_d_div = 4U;       /* 4.096MHz / 6 = 682.7kHz. */
    else if(samplerate == 44100U) class_d_div = 7U;  /* 11.289MHz / 16 = 705.6kHz. */
    else if(samplerate == 48000U) class_d_div = 7U;  /* 12.288MHz / 16 = 768kHz. */
    else return 1;
    WM_TRY_WRITE(0x04, 0);
    WM_TRY_WRITE(0x08, (WM8960_REG_VAL[0x08] & ~0x01C0U) | (class_d_div << 6));
    WM_TRY_WRITE(0x1B, (WM8960_REG_VAL[0x1B] & ~7U) |
                 ((samplerate == 16000U) ? 3U : 0U));
    return 0;
}

/* PCM playback master volume for both headphone and speaker paths.
 * Convert a linear amplitude percentage to the DAC's 0.5dB steps offline,
 * keeping floating-point logarithms out of the firmware (50% is about -6dB).
 * Output PGA gains and the pause/startup soft-mute state remain independent. */
uint8_t WM8960_SetVolume(uint8_t percent)
{
    static const uint8_t dac_levels[101] = {
        0, 175, 187, 194, 199, 203, 206, 209, 211, 213, 215, 217, 218, 220, 221, 222,
        223, 224, 225, 226, 227, 228, 229, 229, 230, 231, 232, 232, 233, 233, 234, 235,
        235, 236, 236, 237, 237, 238, 238, 239, 239, 240, 240, 240, 241, 241, 242, 242,
        242, 243, 243, 243, 244, 244, 244, 245, 245, 245, 246, 246, 246, 246, 247, 247,
        247, 248, 248, 248, 248, 249, 249, 249, 249, 250, 250, 250, 250, 250, 251, 251,
        251, 251, 252, 252, 252, 252, 252, 253, 253, 253, 253, 253, 254, 254, 254, 254,
        254, 254, 255, 255, 255
    };
    if(percent > 100U) percent = 100U;
    uint16_t level = dac_levels[percent];
    WM_TRY_WRITE(0x0A, level);          /* Stage left, DACVU=0. */
    WM_TRY_WRITE(0x0B, level | 0x100U); /* DACVU=1: apply both channels together. */
    return 0;
}

uint8_t WM8960_SetMute(uint8_t mute)
{
    return WM8960_Write_Reg(0x05, (WM8960_REG_VAL[0x05] & ~(1U << 3)) |
                           (mute ? (1U << 3) : 0U));
}

uint8_t WM8960_SetSpeakerEnable(uint8_t enable)
{
    return WM8960_Write_Reg(0x31, (WM8960_REG_VAL[0x31] & ~0x00C0U) |
                           (enable ? 0x00C0U : 0U));
}

uint8_t WM8960_PowerDown(void)
{
    uint8_t status = WM8960_SetMute(1);
    status |= WM8960_SetSpeakerEnable(0);
    /* Release the data wire before switching the STM32 I2S direction. */
    status |= WM8960_Write_Reg(0x18, WM8960_REG_VAL[0x18] | (1U << 3));
    status |= WM8960_Write_Reg(0x1A, 0);
    status |= WM8960_Write_Reg(0x2F, 0);
    status |= WM8960_Write_Reg(0x19, 0);
    return status;
}

