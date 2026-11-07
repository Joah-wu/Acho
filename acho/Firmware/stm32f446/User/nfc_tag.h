#ifndef ACHO_NFC_TAG_H
#define ACHO_NFC_TAG_H
#include <stdint.h>
#include "acho_ndef.h"
typedef struct { char uid[17]; uint32_t memory_bytes; uint8_t ic_reference; } NfcTagInfo;
int NfcTag_Init(NfcTagInfo *info);
int NfcTag_FieldPresent(void);
int NfcTag_Publish(const uint8_t image[ACHO_NFC_MEMORY]);
#endif
