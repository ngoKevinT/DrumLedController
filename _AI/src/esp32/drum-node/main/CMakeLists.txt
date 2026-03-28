idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        espnow_transport
        driver       # gpio_reset_pin, gpio_set_direction, gpio_set_level, etc.
        esp_log
)
