#include "acho_wifi_qr.h"
#include "acho_protocol.h"
#include "acho_wifi_info.h"
#include "qrcode.h"

static uint8_t *target;
static bool rendered;
static void render(esp_qrcode_handle_t qr) {
    int modules=esp_qrcode_get_size(qr);
    int scale=ACHO_WIFI_QR_WIDTH/(modules+8); /* Four white modules on each side. */
    if(scale<2) return; /* Keep every module at least 2 LCD pixels wide. */
    int margin=(ACHO_WIFI_QR_WIDTH-modules*scale)/2;
    memset(target,0,ACHO_WIFI_QR_SIZE);
    acho_put32(target,7u | (ACHO_WIFI_QR_WIDTH<<10) | (ACHO_WIFI_QR_WIDTH<<21));
    target[7]=0xff; /* Black palette entry RGBA. */
    memset(target+8,0xff,ACHO_WIFI_QR_SIZE-8); /* White palette and pixels. */
    for(int y=0;y<modules;y++) for(int x=0;x<modules;x++) {
        if(!esp_qrcode_get_module(qr,x,y)) continue;
        for(int dy=0;dy<scale;dy++) for(int dx=0;dx<scale;dx++) {
            unsigned px=margin+x*scale+dx, py=margin+y*scale+dy;
            target[12+py*ACHO_WIFI_QR_STRIDE+px/8]&=(uint8_t)~(0x80u>>(px%8));
        }
    }
    rendered=true;
}
bool acho_wifi_qr_make(const char *payload,uint8_t *out,size_t capacity) {
    if(!payload || !out || capacity<ACHO_WIFI_QR_SIZE) return false;
    target=out; rendered=false;
    esp_qrcode_config_t config={.display_func=render,.max_qrcode_version=6,.qrcode_ecc_level=ESP_QRCODE_ECC_MED};
    esp_err_t result=esp_qrcode_generate(&config,payload);
    target=NULL;
    return result==ESP_OK && rendered;
}
