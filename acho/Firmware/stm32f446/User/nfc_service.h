#ifndef ACHO_NFC_SERVICE_H
#define ACHO_NFC_SERVICE_H
#include <stdint.h>
#include <stddef.h>
#include "acho_wifi_info.h"
enum { NFC_OWNER_NONE,NFC_OWNER_WIFI,NFC_OWNER_PAIR };
enum { NFC_UNAVAILABLE,NFC_PREPARING,NFC_BUSY,NFC_READY,NFC_ERROR,NFC_EXPIRED };
typedef struct {
    uint32_t requested_revision,published_revision,expires_at;
    uint8_t state,owner;
    char uid[17];
} NfcStatus;
void NfcService_Init(void);
uint32_t NfcService_Publish(const char *payload,size_t length,uint8_t owner,uint32_t valid_ms);
uint32_t NfcService_PublishWifi(const uint8_t info[ACHO_WIFI_INFO_SIZE]);
void NfcService_Clear(uint8_t owner);
void NfcService_Status(NfcStatus *out);
#endif
