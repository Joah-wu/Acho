#include "acho_wifi_qr.h"
#include "acho_wifi_info.h"
#include "acho_protocol.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    uint8_t info[ACHO_WIFI_INFO_SIZE]={1,ACHO_WIFI_PROVISIONING};
    memcpy(info+2,"ACHO_AABBCC",12);
    memcpy(info+14,"0123456789abcdef",17);
    assert(acho_wifi_info_valid(info,sizeof(info)));
    assert(!acho_wifi_info_valid(info,sizeof(info)-1));
    info[13]='X';assert(!acho_wifi_info_valid(info,sizeof(info)));info[13]=0;
    info[14]='G';assert(!acho_wifi_info_valid(info,sizeof(info)));info[14]='0';
    info[1]=0x80;assert(!acho_wifi_info_valid(info,sizeof(info)));
    const char *payload="{\"ver\":\"v1\",\"name\":\"ACHO_AABBCC\",\"pop\":\"0123456789abcdef\",\"transport\":\"softap\",\"security\":1}";
    uint8_t image[ACHO_WIFI_QR_SIZE+1]; image[ACHO_WIFI_QR_SIZE]=0x5a;
    assert(!acho_wifi_qr_make(payload,image,ACHO_WIFI_QR_SIZE-1));
    assert(acho_wifi_qr_make(payload,image,ACHO_WIFI_QR_SIZE));
    assert(image[ACHO_WIFI_QR_SIZE]==0x5a);
    assert(acho_u32(image)==(7u|(108u<<10)|(108u<<21)));
    /* At least four white modules at two pixels/module surround the symbol. */
    for(unsigned y=0;y<108;y++) for(unsigned x=0;x<108;x++) {
        if(x<8 || y<8 || x>=100 || y>=100)
            assert(image[12+y*ACHO_WIFI_QR_STRIDE+x/8]&(0x80u>>(x%8)));
    }
    FILE *file=fopen("/tmp/acho-wifi-qr.bin","wb");assert(file);
    assert(fwrite(image,1,ACHO_WIFI_QR_SIZE,file)==ACHO_WIFI_QR_SIZE);fclose(file);
    puts("Wi-Fi info schema, QR image size, capacity and quiet border passed");
}
