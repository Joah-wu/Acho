#ifndef ACHO_WIFI_INFO_H
#define ACHO_WIFI_INFO_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Versioned byte layout, independent of struct padding on ESP32/STM32/host. */
#define ACHO_WIFI_INFO_SIZE 31u
#define ACHO_WIFI_CONNECTED 1u
#define ACHO_WIFI_PROVISIONING 2u
#define ACHO_WIFI_CONFIGURED 4u
#define ACHO_WIFI_RESTARTING 8u
#define ACHO_WIFI_QR_WIDTH 108u
#define ACHO_WIFI_QR_STRIDE ((ACHO_WIFI_QR_WIDTH+7u)/8u)
#define ACHO_WIFI_QR_SIZE (12u+ACHO_WIFI_QR_STRIDE*ACHO_WIFI_QR_WIDTH)
#define ACHO_WIFI_SSID_OFFSET 2u
#define ACHO_WIFI_POP_OFFSET 14u
static inline int acho_wifi_info_valid(const uint8_t *data,size_t size) {
    if(size!=ACHO_WIFI_INFO_SIZE || data[0]!=1 || (data[1]&~15u) ||
       memcmp(data+ACHO_WIFI_SSID_OFFSET,"ACHO_",5) || data[13]!=0 || data[30]!=0) return 0;
    for(size_t i=7;i<13;i++)
        if(!((data[i]>='0' && data[i]<='9') || (data[i]>='A' && data[i]<='F'))) return 0;
    for(size_t i=14;i<30;i++)
        if(!((data[i]>='0' && data[i]<='9') || (data[i]>='a' && data[i]<='f'))) return 0;
    return 1;
}
#endif
