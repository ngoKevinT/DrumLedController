idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        espnow_transport   # WiFi, ESP-NOW, NVS init — all via the shared component
        driver             # gpio_reset_pin, gpio_set_direction, gpio_set_level, etc.
        nvs_flash          # nvs_open/nvs_get_str/nvs_set_blob used in main.c directly
        led_strip          # SK6812 RGBW output over RMT (managed component, see idf_component.yml)
        # esp_log is the header name; IDF adds the 'log' component automatically.
)
