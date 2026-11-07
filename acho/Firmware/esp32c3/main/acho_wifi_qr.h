#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Called only by the serial bridge worker. Produces an LVGL indexed binary. */
bool acho_wifi_qr_make(const char *payload,uint8_t *out,size_t capacity);
