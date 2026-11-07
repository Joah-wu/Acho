#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
void acho_wifi_start(void);
bool acho_wifi_wait(TickType_t ticks);
void acho_server_url(char *out, size_t capacity);

#include "esp_err.h"
#include "acho_wifi_info.h"
void acho_wifi_info(uint8_t out[ACHO_WIFI_INFO_SIZE]);
bool acho_wifi_qr_payload(char *out,size_t capacity);
esp_err_t acho_wifi_prepare_reprovision(bool *restart_needed);
void acho_wifi_restart_for_provisioning(void);
