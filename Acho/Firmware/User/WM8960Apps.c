/* Acho WM8960 half-duplex audio: 16-bit PCM, 16kHz / 44.1kHz / 48kHz.
 * HAL/CMSIS vendor sources are unchanged. File I/O runs only in task context. */
#include "WM8960Apps.h"
#include "WM8960.h"
#include "i2s.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define AUDIO_HALF_BYTES       (WAV_BUFFER_SIZE / 2U)
#define AUDIO_TX_HALF          (1UL << 0)
#define AUDIO_TX_FULL          (1UL << 1)
#define AUDIO_RX_HALF          (1UL << 2)
#define AUDIO_RX_FULL          (1UL << 3)
#define AUDIO_DMA_ERROR        (1UL << 4)
#define AUDIO_POLL_TICKS        pdMS_TO_TICKS(10)
#define AUDIO_DMA_TIMEOUT      pdMS_TO_TICKS(250)
#define AUDIO_FORMAT_ERROR     0x80U
#define AUDIO_DMA_FAILURE      0x81U
#define AUDIO_SHORT_READ       0x83U

_Static_assert(WAV_BUFFER_SIZE % 8U == 0U, "Stereo DMA halves must contain whole frames");
_Static_assert(sizeof(__WaveHeader) == 44U, "WAV recording header must be 44 bytes");

FIL WAV_File;
volatile wavctrl WaveCtrlData;
uint8_t WAV_Buffer[WAV_BUFFER_SIZE] __attribute__((aligned(4)));
static uint8_t TempBuf[AUDIO_HALF_BYTES] __attribute__((aligned(4)));
volatile I2S_CallBack_Flag I2S_Flag;
volatile uint8_t audio_play_request = AUDIO_NONE;
volatile uint8_t audio_play_state = AUDIO_NONE;
volatile uint8_t audio_rec_request = AUDIO_NONE;
volatile uint8_t audio_rec_state = AUDIO_NONE;
volatile uint8_t audio_last_error;
volatile uint32_t WAV_LastData;
uint8_t Music_Num_MAX;
uint8_t Music_Num;
static TaskHandle_t volatile audio_io_task;
static volatile uint32_t dma_sequence;
static uint32_t wav_file_remaining;
static uint8_t wav_read_error;
static bool wav_file_open;
static bool audio_hardware_started;
static char recording_path[_MAX_LFN + 16U];
static volatile uint8_t requested_volume = 100U;

void Volum_Control(uint8_t ctrl)
{
    /* Publish one byte only; the audio task owns all codec/I2C transactions. */
    requested_volume = ctrl > 100U ? 100U : ctrl;
}

static bool supported_rate(uint32_t fs)
{
    return fs == 16000U || fs == 44100U || fs == 48000U;
}

static uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint8_t read_exact(void *buf, UINT size)
{
    UINT count = 0;
    FRESULT result = f_read(&WAV_File, buf, size, &count);
    if(result != FR_OK) return (uint8_t)result;
    return count == size ? 0U : AUDIO_SHORT_READ;
}

/* Walk RIFF chunks rather than assuming fmt/data are at fixed byte offsets. */
uint8_t Get_WAV_Message(char *fname, wavctrl *wavx)
{
    uint8_t header[12], chunk[8], fmt[16];
    wavctrl info = {0};
    bool have_fmt = false, have_data = false;
    uint32_t cursor = 12U, riff_end = 0, data_start = 0, data_size = 0;
    uint32_t byte_rate = 0;
    const char *reason = "file read/seek failed";
    uint8_t error;

    if(fname == NULL || wavx == NULL || wav_file_open) return FR_INVALID_PARAMETER;
    memset(wavx, 0, sizeof(*wavx));
    error = (uint8_t)f_open(&WAV_File, fname, FA_READ);
    if(error != FR_OK) {
        printf("WAV open failed: fr=%u file=%s\r\n", (unsigned int)error, fname);
        return error;
    }
    wav_file_open = true;
    error = read_exact(header, sizeof(header));
    if(error != 0U) goto fail;
    if(memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
        reason = "missing RIFF/WAVE signature";
        goto format_fail;
    }
    if(read_le32(header + 4) < 4U ||
       read_le32(header + 4) > f_size(&WAV_File) - 8U) {
        reason = "RIFF size exceeds file or is invalid";
        goto format_fail;
    }
    riff_end = read_le32(header + 4) + 8U;
    while(cursor <= riff_end && riff_end - cursor >= 8U) {
        error = (uint8_t)f_lseek(&WAV_File, cursor);
        if(error != FR_OK) goto fail;
        error = read_exact(chunk, sizeof(chunk));
        if(error != 0U) goto fail;
        uint32_t length = read_le32(chunk + 4);
        uint32_t payload = cursor + 8U;
        if(length > riff_end - payload) {
            reason = "chunk exceeds RIFF boundary";
            goto format_fail;
        }
        if(memcmp(chunk, "fmt ", 4) == 0 && !have_fmt) {
            if(length < sizeof(fmt)) { reason = "fmt chunk is too short"; goto format_fail; }
            error = read_exact(fmt, sizeof(fmt));
            if(error != 0U) goto fail;
            info.audioformat = read_le16(fmt);
            info.nchannels = read_le16(fmt + 2);
            info.samplerate = read_le32(fmt + 4);
            info.blockalign = read_le16(fmt + 12);
            info.bps = read_le16(fmt + 14);
            byte_rate = read_le32(fmt + 8);
            if(info.audioformat == 0xFFFEU) {
                /* WAVEFORMATEXTENSIBLE: validate the full PCM subtype GUID.
                 * A .wav extension alone does not imply integer PCM samples. */
                static const uint8_t pcm_guid[16] = {
                    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                    0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
                };
                uint8_t extension[24];
                if(length < 40U) { reason = "extensible fmt is too short"; goto format_fail; }
                error = read_exact(extension, sizeof(extension));
                if(error != 0U) goto fail;
                uint16_t extra_size = read_le16(extension);
                uint16_t valid_bits = read_le16(extension + 2);
                if(extra_size < 22U || extra_size > length - 18U) {
                    reason = "invalid extensible fmt size";
                    goto format_fail;
                }
                if(memcmp(extension + 8, pcm_guid, sizeof(pcm_guid)) != 0) {
                    reason = "extensible subtype is not integer PCM";
                    goto format_fail;
                }
                if(valid_bits == 0U || valid_bits > info.bps) {
                    reason = "invalid PCM valid-bit count";
                    goto format_fail;
                }
            } else if(info.audioformat != 1U) {
                reason = "encoding is not integer PCM";
                goto format_fail;
            }
            if(info.bps != 16U) { reason = "only 16-bit PCM is supported"; goto format_fail; }
            if(info.nchannels != 1U && info.nchannels != 2U) {
                reason = "only mono/stereo is supported";
                goto format_fail;
            }
            if(!supported_rate(info.samplerate)) {
                reason = "unsupported sample rate (use 16000, 44100 or 48000 Hz)";
                goto format_fail;
            }
            if(info.blockalign != info.nchannels * 2U) {
                reason = "invalid PCM block alignment";
                goto format_fail;
            }
            if(byte_rate != info.samplerate * info.blockalign) {
                reason = "byte rate does not match PCM format";
                goto format_fail;
            }
            info.audioformat = 1U; /* Both header variants use the same PCM16 path. */
            info.bitrate = byte_rate * 8U;
            have_fmt = true;
        } else if(memcmp(chunk, "data", 4) == 0 && !have_data) {
            data_start = payload;
            data_size = length;
            have_data = true;
        }
        if(have_fmt && have_data) break;
        cursor = payload + length;
        if(length & 1U) {
            if(cursor == riff_end) { reason = "missing chunk padding byte"; goto format_fail; }
            ++cursor;
        }
    }
    if(!have_fmt || !have_data) { reason = "missing fmt or data chunk"; goto format_fail; }
    if(data_size == 0U) { reason = "empty data chunk"; goto format_fail; }
    if(data_size % info.blockalign != 0U) { reason = "incomplete PCM frame"; goto format_fail; }
    info.datastart = data_start;
    info.datasize = data_size;
    info.totsec = data_size / (info.bitrate / 8U);
    error = (uint8_t)f_lseek(&WAV_File, data_start);
    if(error != FR_OK) goto fail;
    *wavx = info;
    wav_file_remaining = data_size;
    WAV_LastData = data_size;
    wav_read_error = 0;
    return 0;
format_fail:
    error = AUDIO_FORMAT_ERROR;
fail:
    printf("WAV rejected: %s (error=%u)\r\n", reason, (unsigned int)error);
    printf("WAV file: %s\r\n", fname);
    if(error == AUDIO_FORMAT_ERROR) {
        printf("WAV fmt: tag=0x%04X bits=%u ch=%u fs=%lu align=%u byte_rate=%lu\r\n",
               (unsigned int)info.audioformat, (unsigned int)info.bps,
               (unsigned int)info.nchannels, (unsigned long)info.samplerate,
               (unsigned int)info.blockalign, (unsigned long)byte_rate);
        printf("WAV bounds: file=%lu riff=%lu offset=%lu data=%lu\r\n",
               (unsigned long)f_size(&WAV_File), (unsigned long)riff_end,
               (unsigned long)cursor, (unsigned long)data_size);
    }
    f_close(&WAV_File);
    wav_file_open = false;
    return error;
}

/* Return valid stereo output bytes; the remainder is always silent.
 * Mono PCM is duplicated to L/R, without changing the file-byte accounting. */
uint32_t Fill_WAV_Buffer(uint8_t *buf, uint16_t size)
{
    UINT read_count = 0;
    if(buf == NULL) { wav_read_error = FR_INVALID_PARAMETER; return 0; }
    memset(buf, 0, size);
    if(size % 4U != 0U || WaveCtrlData.bps != 16U ||
       (WaveCtrlData.nchannels != 1U && WaveCtrlData.nchannels != 2U) ||
       WaveCtrlData.blockalign != WaveCtrlData.nchannels * 2U) {
        wav_read_error = AUDIO_FORMAT_ERROR;
        return 0;
    }
    uint32_t wanted = WaveCtrlData.nchannels == 1U ? size / 2U : size;
    if(wanted > wav_file_remaining) wanted = wav_file_remaining;
    if(wanted == 0U) return 0;
    uint8_t *dst = WaveCtrlData.nchannels == 1U ? TempBuf : buf;
    if(dst == TempBuf && wanted > sizeof(TempBuf)) {
        wav_read_error = FR_INVALID_PARAMETER;
        return 0;
    }
    FRESULT result = f_read(&WAV_File, dst, (UINT)wanted, &read_count);
    if(result != FR_OK || read_count != wanted ||
       read_count % WaveCtrlData.blockalign != 0U) {
        wav_read_error = result != FR_OK ? (uint8_t)result : AUDIO_SHORT_READ;
        memset(buf, 0, size);
        return 0;
    }
    wav_file_remaining -= read_count;
    WAV_LastData = wav_file_remaining;
    if(WaveCtrlData.nchannels == 1U) {
        for(UINT i = 0; i < read_count / 2U; ++i) {
            memcpy(buf + i * 4U, TempBuf + i * 2U, 2);
            memcpy(buf + i * 4U + 2U, TempBuf + i * 2U, 2);
        }
        return read_count * 2U;
    }
    return read_count;
}

static void release_data_wire(void)
{
    GPIO_InitTypeDef gpio = {0};
    __HAL_I2S_DISABLE(&hi2s2);
    gpio.Pin = WM8960_I2S_DAT_Pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(WM8960_I2S_DAT_GPIO_Port, &gpio);
}

/* Called with DMA stopped. F446 has its own PLLI2SM, independent of system PLLM.
 * 16k:   12MHz/10 *256 /5 =61.44MHz, /15 /256 =16000Hz.
 * 44.1k: 12MHz/9  *254 /3 =112.888889MHz, /10 /256 =44097.222Hz.
 * 48k:   12MHz/10 *256 /5 =61.44MHz, /5 /256 =48000Hz. */
static HAL_StatusTypeDef configure_i2s(uint32_t fs, uint32_t mode)
{
    RCC_PeriphCLKInitTypeDef clock = {0};
    GPIO_InitTypeDef gpio = {0};
    if(!supported_rate(fs) || HSE_VALUE != 12000000U) return HAL_ERROR;
    __HAL_I2S_DISABLE(&hi2s2);
    clock.PeriphClockSelection = RCC_PERIPHCLK_I2S_APB1;
    clock.I2sApb1ClockSelection = RCC_I2SAPB1CLKSOURCE_PLLI2S;
    clock.PLLI2S.PLLI2SM = fs == 44100U ? 9U : 10U;
    clock.PLLI2S.PLLI2SN = fs == 44100U ? 254U : 256U;
    clock.PLLI2S.PLLI2SR = fs == 44100U ? 3U : 5U;
    clock.PLLI2S.PLLI2SP = RCC_PLLI2SP_DIV2;
    clock.PLLI2S.PLLI2SQ = 2U;
    clock.PLLI2SDivQ = 1U;
    if(HAL_RCCEx_PeriphCLKConfig(&clock) != HAL_OK) return HAL_ERROR;
    hi2s2.Init.Mode = mode;
    hi2s2.Init.DataFormat = I2S_DATAFORMAT_16B;
    hi2s2.Init.AudioFreq = fs;
    if(HAL_I2S_Init(&hi2s2) != HAL_OK) return HAL_ERROR;
    /* Both directions use AF5. I2S RX itself releases the shared SD output. */
    gpio.Pin = WM8960_I2S_DAT_Pin | WM8960_I2S_BCLK_Pin | WM8960_I2S_LRCLK_Pin;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(WM8960_I2S_DAT_GPIO_Port, &gpio);
    /* MCLK reaches 12.288MHz at 48kHz; use fast GPIO edges on the clocks too. */
    gpio.Pin = WM8960_I2S_MCLK_Pin;
    HAL_GPIO_Init(WM8960_I2S_MCLK_GPIO_Port, &gpio);
    return HAL_OK;
}

static void begin_dma_notifications(void)
{
    uint32_t stale;
    xTaskNotifyWait(0, UINT32_MAX, &stale, 0);
    dma_sequence = 0;
    I2S_Flag = I2S_No_CallBack;
}

static void end_audio_hardware(void)
{
    /* Stop callbacks before releasing the task owner and shared buffers. */
    if(audio_hardware_started) {
        WM8960_SetMute(1);
        WM8960_SetSpeakerEnable(0);
    }
    HAL_I2S_DMAStop(&hi2s2);
    if(audio_hardware_started) WM8960_PowerDown();
    audio_hardware_started = false;
    release_data_wire();
    HAL_GPIO_WritePin(SPK_EN_GPIO_Port, SPK_EN_Pin, GPIO_PIN_RESET);
}

/* Starts with both chips released from the data wire, and a stable speaker rail. */
static uint8_t prepare_audio(uint8_t codec_mode, uint32_t fs, uint32_t i2s_mode,
                             const char **stage)
{
    release_data_wire();
    HAL_GPIO_WritePin(SPK_EN_GPIO_Port, SPK_EN_Pin,
                      codec_mode == WM8960_MODE_DAC_ENABLE ? GPIO_PIN_SET : GPIO_PIN_RESET);
    audio_hardware_started = true;
    vTaskDelay(pdMS_TO_TICKS(10));
    *stage = "codec.init";
    uint8_t error = WM89060_Init(codec_mode);
    if(error != 0U) return error;
    *stage = "i2s.clock_init";
    if(configure_i2s(fs, i2s_mode) != HAL_OK) return AUDIO_DMA_FAILURE;
    *stage = "codec.sample_rate";
    error = WM8960_SetSampleRate(fs);
    if(error != 0U) return error;
    /* Let VMID settle without blocking the lower priority GUI task. */
    vTaskDelay(pdMS_TO_TICKS(300));
    return 0;
}

uint8_t PlayWaveFile(char *pname)
{
    uint8_t result = 0;
    bool dma_started = false, cancelled = false;
    uint32_t valid[2] = {0, 0}, sequence = 0, next_half = 0;
    wavctrl info;
    TickType_t last_boundary;
    const char *stage = "wav.open_parse";
    uint32_t file_position = 0;
    uint8_t applied_volume = 100U;

    taskENTER_CRITICAL();
    if(audio_play_request != AUDIO_PLAY || audio_io_task != NULL) {
        taskEXIT_CRITICAL();
        return 0xFF;
    }
    audio_io_task = xTaskGetCurrentTaskHandle();
    audio_play_state = AUDIO_PRE_PLAY;
    taskEXIT_CRITICAL();
    audio_last_error = 0;

    result = Get_WAV_Message(pname, &info);
    if(result != 0U) goto finish;
    WaveCtrlData = info;
    result = prepare_audio(WM8960_MODE_DAC_ENABLE, info.samplerate, I2S_MODE_MASTER_TX, &stage);
    if(result != 0U) goto finish;
    if(audio_play_request == AUDIO_CANCEL) { cancelled = true; goto finish; }

    stage = "fatfs.prefill_first";
    valid[0] = Fill_WAV_Buffer(WAV_Buffer, AUDIO_HALF_BYTES);
    if(wav_read_error != 0U) { result = wav_read_error; goto finish; }
    stage = "fatfs.prefill_second";
    valid[1] = Fill_WAV_Buffer(WAV_Buffer + AUDIO_HALF_BYTES, AUDIO_HALF_BYTES);
    if(wav_read_error != 0U) { result = wav_read_error; goto finish; }
    stage = "codec.volume";
    applied_volume = requested_volume;
    result = WM8960_SetVolume(applied_volume);
    if(result != 0U) goto finish;
    begin_dma_notifications();
    stage = "i2s.tx_dma_start";
    if(HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t *)WAV_Buffer, WAV_BUFFER_SIZE / 2U) != HAL_OK) {
        result = AUDIO_DMA_FAILURE;
        goto finish;
    }
    dma_started = true;
    stage = "codec.speaker_enable";
    result = WM8960_SetSpeakerEnable(1);
    if(result != 0U) goto finish;
    /* A pause requested during setup remains paused rather than briefly sounding. */
    if(audio_play_request == AUDIO_PAUSE) {
        stage = "i2s.pause";
        if(HAL_I2S_DMAPause(&hi2s2) != HAL_OK) { result = AUDIO_DMA_FAILURE; goto finish; }
        audio_play_state = AUDIO_PAUSE;
    } else {
        audio_play_state = AUDIO_PLAY;
        stage = "codec.unmute";
        result = WM8960_SetMute(0);
        if(result != 0U) goto finish;
    }
    last_boundary = xTaskGetTickCount();
    printf("Audio: %lu Hz, PCM16, %u channel(s)\r\n",
           (unsigned long)info.samplerate, (unsigned int)info.nchannels);

    for(;;) {
        if(audio_play_request == AUDIO_CANCEL) { cancelled = true; break; }
        uint8_t volume = requested_volume;
        if(volume != applied_volume) {
            stage = "codec.volume";
            result = WM8960_SetVolume(volume);
            if(result != 0U) break;
            applied_volume = volume;
        }
        if(audio_play_request == AUDIO_PAUSE && audio_play_state == AUDIO_PLAY) {
            stage = "codec.pause_mute";
            result = WM8960_SetMute(1);
            if(result != 0U) break;
            stage = "i2s.pause";
            if(HAL_I2S_DMAPause(&hi2s2) != HAL_OK) { result = AUDIO_DMA_FAILURE; break; }
            audio_play_state = AUDIO_PAUSE;
        } else if(audio_play_request == AUDIO_PLAY &&
                  audio_play_state == AUDIO_PAUSE) {
            stage = "i2s.resume";
            if(HAL_I2S_DMAResume(&hi2s2) != HAL_OK) { result = AUDIO_DMA_FAILURE; break; }
            stage = "codec.resume_unmute";
            result = WM8960_SetMute(0);
            if(result != 0U) break;
            audio_play_state = AUDIO_PLAY;
            last_boundary = xTaskGetTickCount();
        }

        stage = "i2s.wait_dma";
        uint32_t events = 0;
        if(xTaskNotifyWait(0, UINT32_MAX, &events, AUDIO_POLL_TICKS) != pdTRUE) {
            if(audio_play_state == AUDIO_PLAY &&
               xTaskGetTickCount() - last_boundary > AUDIO_DMA_TIMEOUT) {
                result = AUDIO_DMA_FAILURE;
                break;
            }
            continue;
        }
        uint32_t expected = next_half == 0U ? AUDIO_TX_HALF : AUDIO_TX_FULL;
        if(events != expected || dma_sequence != sequence + 1U) {
            result = AUDIO_DMA_FAILURE; /* Missed deadline or DMA hardware error. */
            break;
        }
        ++sequence;
        last_boundary = xTaskGetTickCount();
        valid[next_half] = 0;
        if(valid[0] == 0U && valid[1] == 0U && wav_file_remaining == 0U) break;
        uint8_t *half = WAV_Buffer + next_half * AUDIO_HALF_BYTES;
        stage = "fatfs.read_pcm";
        valid[next_half] = Fill_WAV_Buffer(half, AUDIO_HALF_BYTES);
        if(wav_read_error != 0U) { result = wav_read_error; break; }
        if(dma_sequence != sequence) { stage = "i2s.refill_deadline"; result = AUDIO_DMA_FAILURE; break; }
        next_half ^= 1U;
    }
finish:
    if(wav_file_open) file_position = (uint32_t)f_tell(&WAV_File);
    if(dma_started || audio_io_task != NULL) end_audio_hardware();
    if(wav_file_open) { f_close(&WAV_File); wav_file_open = false; }
    if(audio_play_request == AUDIO_CANCEL) cancelled = true;
    audio_last_error = result;
    taskENTER_CRITICAL();
    audio_io_task = NULL;
    audio_play_state = cancelled ? AUDIO_CANCEL : (result == 0U ? AUDIO_END : AUDIO_NONE);
    audio_play_request = AUDIO_NONE; /* File is closed: GUI may now publish a new path. */
    taskEXIT_CRITICAL();
    if(result != 0U) {
        printf("Audio playback error: %u stage=%s file_pos=%lu\r\n",
               (unsigned int)result, stage, (unsigned long)file_position);
    }
    return result;
}

static uint8_t create_recording(char *name, uint32_t fs, __WaveHeader *header)
{
    FRESULT result = FR_EXIST;
    if(!supported_rate(fs)) return AUDIO_FORMAT_ERROR;
    if(name == NULL) {
        for(unsigned int i = 0; i < 100U; ++i) {
            snprintf(recording_path, sizeof(recording_path), "0:/REC/%u.wav", i);
            result = f_open(&WAV_File, recording_path, FA_CREATE_NEW | FA_WRITE);
            if(result != FR_EXIST) break;
        }
    } else {
        int len = snprintf(recording_path, sizeof(recording_path), "0:/REC/%s.wav", name);
        if(len < 0 || (size_t)len >= sizeof(recording_path)) return FR_INVALID_NAME;
        result = f_open(&WAV_File, recording_path, FA_CREATE_ALWAYS | FA_WRITE);
    }
    if(result != FR_OK) return (uint8_t)result;
    wav_file_open = true;
    memset(header, 0, sizeof(*header));
    header->riff.ChunkID = 0x46464952U;
    header->riff.ChunkSize = 36U;
    header->riff.Format = 0x45564157U;
    header->fmt.ChunkID = 0x20746D66U;
    header->fmt.ChunkSize = 16U;
    header->fmt.AudioFormat = 1U;
    header->fmt.NumOfChannels = 2U;
    header->fmt.SampleRate = fs;
    header->fmt.ByteRate = fs * 4U;
    header->fmt.BlockAlign = 4U;
    header->fmt.BitsPerSample = 16U;
    header->data.ChunkID = 0x61746164U;
    UINT count = 0;
    result = f_write(&WAV_File, header, sizeof(*header), &count);
    return result != FR_OK ? (uint8_t)result :
           (count == sizeof(*header) ? 0U : (uint8_t)FR_DISK_ERR);
}

uint8_t RecordWaveFile(char *fname, uint32_t fs)
{
    uint8_t result;
    bool cancelled = false;
    uint32_t recorded = 0, sequence = 0, next_half = 0;
    __WaveHeader header;
    TickType_t last_boundary;
    const char *stage = "wav.create";

    taskENTER_CRITICAL();
    if(audio_rec_request != AUDIO_RECORD || audio_io_task != NULL) {
        taskEXIT_CRITICAL();
        return 0xEE;
    }
    audio_io_task = xTaskGetCurrentTaskHandle();
    audio_rec_state = AUDIO_PRE_PLAY;
    taskEXIT_CRITICAL();
    audio_last_error = 0;
    result = create_recording(fname, fs, &header);
    if(result != 0U) goto finish;
    result = prepare_audio(WM8960_MODE_ADC_ENABLE, fs, I2S_MODE_MASTER_RX, &stage);
    if(result != 0U) goto finish;
    if(audio_rec_request == AUDIO_CANCEL) { cancelled = true; goto finish; }
    begin_dma_notifications();
    if(HAL_I2S_Receive_DMA(&hi2s2, (uint16_t *)WAV_Buffer, WAV_BUFFER_SIZE / 2U) != HAL_OK) {
        result = AUDIO_DMA_FAILURE;
        goto finish;
    }
    audio_rec_state = AUDIO_RECORD;
    last_boundary = xTaskGetTickCount();
    for(;;) {
        if(audio_rec_request == AUDIO_CANCEL) { cancelled = true; break; }
        if(audio_rec_request == AUDIO_END) break;
        if(audio_rec_request == AUDIO_PAUSE && audio_rec_state == AUDIO_RECORD) {
            if(HAL_I2S_DMAPause(&hi2s2) != HAL_OK) { result = AUDIO_DMA_FAILURE; break; }
            audio_rec_state = AUDIO_PAUSE;
        } else if(audio_rec_request == AUDIO_RESUME && audio_rec_state == AUDIO_PAUSE) {
            if(HAL_I2S_DMAResume(&hi2s2) != HAL_OK) { result = AUDIO_DMA_FAILURE; break; }
            audio_rec_state = AUDIO_RECORD;
            last_boundary = xTaskGetTickCount();
        }
        uint32_t events = 0;
        if(xTaskNotifyWait(0, UINT32_MAX, &events, AUDIO_POLL_TICKS) != pdTRUE) {
            if(audio_rec_state == AUDIO_RECORD &&
               xTaskGetTickCount() - last_boundary > AUDIO_DMA_TIMEOUT) {
                result = AUDIO_DMA_FAILURE;
                break;
            }
            continue;
        }
        uint32_t expected = next_half == 0U ? AUDIO_RX_HALF : AUDIO_RX_FULL;
        if(events != expected || dma_sequence != sequence + 1U) {
            result = AUDIO_DMA_FAILURE;
            break;
        }
        ++sequence;
        last_boundary = xTaskGetTickCount();
        UINT count = 0;
        FRESULT write_result = f_write(&WAV_File, WAV_Buffer + next_half * AUDIO_HALF_BYTES,
                                      AUDIO_HALF_BYTES, &count);
        if(write_result != FR_OK || count != AUDIO_HALF_BYTES) {
            result = write_result != FR_OK ? (uint8_t)write_result : (uint8_t)FR_DISK_ERR;
            break;
        }
        recorded += count;
        if(dma_sequence != sequence) { result = AUDIO_DMA_FAILURE; break; }
        next_half ^= 1U;
    }
finish:
    /* Stop RX before touching the WAV header, closing or unlinking its file. */
    end_audio_hardware();
    if(wav_file_open) {
        if(!cancelled && result == 0U) {
            UINT count = 0;
            header.riff.ChunkSize = recorded + 36U;
            header.data.ChunkSize = recorded;
            FRESULT status = f_lseek(&WAV_File, 0);
            if(status == FR_OK) status = f_write(&WAV_File, &header, sizeof(header), &count);
            if(status == FR_OK && count != sizeof(header)) status = FR_DISK_ERR;
            if(status == FR_OK) status = f_sync(&WAV_File);
            result = (uint8_t)status;
        }
        f_close(&WAV_File);
        wav_file_open = false;
        if(cancelled || result != 0U) f_unlink(recording_path);
    }
    audio_last_error = result;
    taskENTER_CRITICAL();
    audio_io_task = NULL;
    audio_rec_state = cancelled ? AUDIO_CANCEL : (result == 0U ? AUDIO_END : AUDIO_NONE);
    audio_rec_request = AUDIO_NONE;
    taskEXIT_CRITICAL();
    return result;
}

static void notify_dma(I2S_HandleTypeDef *hi2s, uint32_t event)
{
    if(hi2s != &hi2s2 || audio_io_task == NULL) return;
    BaseType_t higher_priority_woken = pdFALSE;
    if(event != AUDIO_DMA_ERROR) ++dma_sequence;
    xTaskNotifyFromISR(audio_io_task, event, eSetBits, &higher_priority_woken);
    portYIELD_FROM_ISR(higher_priority_woken);
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    I2S_Flag = I2S_Half_Callback;
    notify_dma(hi2s, AUDIO_TX_HALF);
}
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    I2S_Flag = I2S_Callback;
    notify_dma(hi2s, AUDIO_TX_FULL);
}
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    I2S_Flag = I2S_Half_Callback;
    notify_dma(hi2s, AUDIO_RX_HALF);
}
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    I2S_Flag = I2S_Callback;
    notify_dma(hi2s, AUDIO_RX_FULL);
}
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    notify_dma(hi2s, AUDIO_DMA_ERROR);
}
