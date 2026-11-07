#ifndef ACHO_NDEF_H
#define ACHO_NDEF_H
#include <stddef.h>
#include <stdint.h>
#define ACHO_NFC_MEMORY 512u
#define ACHO_NFC_MAX_PAYLOAD 384u
#define ACHO_NFC_MIME "application/vnd.acho.device"
#define ACHO_ANDROID_PACKAGE "com.example.websocktest"
#define ACHO_NFC_IDLE "{\"purpose\":\"idle\"}"
/* E1 for 4-Kbit tags, E2 for larger tags; first 512 bytes are reserved. */
size_t acho_ndef_encode(uint8_t *out,size_t capacity,const uint8_t *payload,size_t length,int extended_cc);
typedef struct {
    void *context;
    int (*read)(void*,uint16_t,uint8_t*,uint16_t);
    int (*write)(void*,uint16_t,const uint8_t*,uint16_t);
} acho_ndef_io_t;
/* Invalidate length, replace/verify body, then commit a complete header last. */
int acho_ndef_publish(const acho_ndef_io_t *io,const uint8_t image[ACHO_NFC_MEMORY]);
#endif
