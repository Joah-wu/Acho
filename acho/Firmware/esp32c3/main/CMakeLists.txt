idf_component_register(SRCS "esp32c3.c" "acho_wifi.c" "acho_bridge.c" "acho_wifi_qr.c"
    INCLUDE_DIRS "." "../../common"
    REQUIRES driver esp_wifi esp_event esp_netif nvs_flash wifi_provisioning
             esp_http_client esp_websocket_client json mdns mbedtls qrcode)
