idf_component_register(
    SRCS "espnow_transport.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_netif esp_event esp_wifi
)
