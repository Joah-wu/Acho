#ifndef ACHO_PAGE_WIFI_H
#define ACHO_PAGE_WIFI_H
#include "gui_setup.h"
extern lv_ui ui_wifi;
/* NFC menu entry: prefer the tap illustration; QR and hotspot details remain available. */
void WifiPage_OpenNfc(lv_ui *last_ui);
/* Initialize once after the main screen exists. Uses RAM-only serial requests. */
void WifiPage_Init(void);
int WifiPage_GetInfo(uint8_t out[31]);
void WifiPage_RequestRefresh(void);
#endif
