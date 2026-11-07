#include "acho_wifi.h"

void acho_bridge_start(void);

void app_main(void)
{
    acho_wifi_start();
    acho_bridge_start();
}
