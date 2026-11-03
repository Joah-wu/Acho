#include <string.h>
#include "mlcd.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;

MLCD mlcd;
#define MLCD_LINE_STRIDE (1U + LCD_BYTES_PER_LINE + 1U)
#define MLCD_PIXEL_OFFSET 2U
#define MLCD_FRAME_SIZE (1U + LCD_HEIGHT * MLCD_LINE_STRIDE + 1U) //写命令+每行数据：行地址 + 像素数据 + 行末 dummy+...+帧末 dummy

static uint8_t mlcd_tx_frame[MLCD_FRAME_SIZE];
static GPIO_PinState mlcd_vcom_state = GPIO_PIN_RESET;
static uint32_t mlcd_vcom_tick;
static volatile uint8_t mlcd_dma_busy;
static MLCD_RefreshDoneCallback mlcd_refresh_callback;
static void *mlcd_refresh_context;

static uint8_t *mlcd_LinePixels(uint16_t line)
{
  return &mlcd_tx_frame[MLCD_PIXEL_OFFSET + line * MLCD_LINE_STRIDE];
}

static HAL_StatusTypeDef mlcd_TransmitData(const uint8_t *data, uint16_t size)
{
  return HAL_SPI_Transmit(&hspi1, (uint8_t *)data, size, HAL_MAX_DELAY);
}

void mlcd_Init(void)
{
  mlcd.CurrentX = 0;
  mlcd.CurrentY = 0;
  mlcd.Inverted = FALSE;
  mlcd.Buffer = mlcd_tx_frame;
  mlcd.BufferSize = sizeof(mlcd_tx_frame);
  mlcd.Initialized = FALSE;
  memset(mlcd_tx_frame, 0xFF, sizeof(mlcd_tx_frame));
  mlcd_tx_frame[0] = BIT_WRITECMD;
  for (uint16_t line = 0; line < LCD_HEIGHT; ++line) {
    mlcd_tx_frame[1U + line * MLCD_LINE_STRIDE] = (uint8_t)(line + 1U);
    mlcd_tx_frame[1U + line * MLCD_LINE_STRIDE + 1U + LCD_BYTES_PER_LINE] = 0;
  }
  mlcd_tx_frame[MLCD_FRAME_SIZE - 1U] = 0;

  mlcd_vcom_state = GPIO_PIN_RESET;
  mlcd_vcom_tick = HAL_GetTick();
  HAL_GPIO_WritePin(MLCD_EXTCOMIN_GPIO_Port, MLCD_EXTCOMIN_Pin, mlcd_vcom_state);
  HAL_GPIO_WritePin(DISP_DISPLAY_GPIO_Port, DISP_DISPLAY_Pin, GPIO_PIN_SET);
  HAL_Delay(1);
  mlcd_Clear();
  HAL_Delay(1);
  mlcd.Initialized = TRUE;
}

void mlcd_Clear(void)
{
  uint8_t clear_data[2] = {
    (uint8_t)(BIT_CLEAR | (mlcd_vcom_state == GPIO_PIN_SET ? BIT_VCOM : 0)),
    0x00
  };

  if (mlcd_dma_busy) {
    return;
  }
  for (uint16_t line = 0; line < LCD_HEIGHT; ++line) {
    memset(mlcd_LinePixels(line), 0xFF, LCD_BYTES_PER_LINE);
  }

  HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_SET);
  (void)mlcd_TransmitData(clear_data, sizeof(clear_data));
  HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_RESET);
  mlcd.CurrentX = 0;
  mlcd.CurrentY = 0;
}

void mlcd_DrawPixel(uint8_t x, uint8_t y, LCD_COLOR color)
{
  if (x >= LCD_WIDTH || y >= LCD_HEIGHT || mlcd.Buffer == NULL || mlcd_dma_busy) {
    return;
  }

  uint16_t bit = (uint16_t)x * 3U;
  uint8_t *byte = &mlcd_LinePixels(y)[bit / 8U];
  uint8_t shift = (uint8_t)(bit & 7U);
  uint8_t value = (uint8_t)color & 0x07U;

  if (shift <= 5U) {
    uint8_t mask = (uint8_t)(0x07U << shift);
    *byte = (uint8_t)((*byte & (uint8_t)~mask) | (uint8_t)(value << shift));
  } else {
    uint8_t low_mask = (uint8_t)(0x07U << shift);
    uint8_t high_mask = (uint8_t)(0x07U >> (8U - shift));
    *byte = (uint8_t)((*byte & (uint8_t)~low_mask) | (uint8_t)(value << shift));
    byte[1] = (uint8_t)((byte[1] & (uint8_t)~high_mask) | (value >> (8U - shift)));
  }
}

static uint16_t mlcd_BuildRefreshFrame(void)
{
  uint16_t offset = 0;
  mlcd_tx_frame[offset++] = (uint8_t)(BIT_WRITECMD |
      (mlcd_vcom_state == GPIO_PIN_SET ? BIT_VCOM : 0));
  return MLCD_FRAME_SIZE;
}

static void mlcd_RefreshComplete(void)
{
  MLCD_RefreshDoneCallback callback = mlcd_refresh_callback;
  void *context = mlcd_refresh_context;

  HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_RESET);
  mlcd_dma_busy = FALSE;
  mlcd_refresh_callback = NULL;
  mlcd_refresh_context = NULL;
  if (callback != NULL) {
    callback(context);
  }
}

void mlcd_Refresh(void)
{
  if (mlcd.Buffer == NULL || mlcd_dma_busy) {
    return;
  }

  uint16_t frame_size = mlcd_BuildRefreshFrame();
  HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_SET);
  if (HAL_SPI_Transmit(&hspi1, mlcd_tx_frame, frame_size, HAL_MAX_DELAY) != HAL_OK) {
    HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_RESET);
  }
}

HAL_StatusTypeDef mlcd_RefreshDMA(MLCD_RefreshDoneCallback callback, void *context)
{
  if (mlcd.Buffer == NULL || mlcd_dma_busy) {
    return HAL_BUSY;
  }

  uint16_t frame_size = mlcd_BuildRefreshFrame();
  mlcd_refresh_callback = callback;
  mlcd_refresh_context = context;
  mlcd_dma_busy = TRUE;
  HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_SET);
  HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(&hspi1, mlcd_tx_frame, frame_size);
  if (status != HAL_OK) {
    HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_RESET);
    mlcd_dma_busy = FALSE;
    mlcd_refresh_callback = NULL;
    mlcd_refresh_context = NULL;
  }
  return status;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1 && mlcd_dma_busy) {
    mlcd_RefreshComplete();
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1 && mlcd_dma_busy) {
    mlcd_RefreshComplete();
  }
}

void mlcd_Service(void)
{
  uint32_t now = HAL_GetTick();
  if ((uint32_t)(now - mlcd_vcom_tick) >= 500U) {
    mlcd_vcom_tick = now;
    mlcd_vcom_state = mlcd_vcom_state == GPIO_PIN_SET ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(MLCD_EXTCOMIN_GPIO_Port, MLCD_EXTCOMIN_Pin, mlcd_vcom_state);
  }
}

void mlcd_fill(uint8_t color)
{
  if (mlcd.Buffer == NULL || mlcd_dma_busy) {
    return;
  }
  for (uint16_t line = 0; line < LCD_HEIGHT; ++line) {
    memset(mlcd_LinePixels(line), color == Black ? 0x00 : 0xFF, LCD_BYTES_PER_LINE);
  }
  mlcd_Refresh();
}
