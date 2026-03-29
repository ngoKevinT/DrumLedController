idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        # espnow_transport  # Phase 1 — uncomment when component is implemented
        driver       # gpio_reset_pin, gpio_set_direction, gpio_set_level, etc.
        # esp_log is omitted — it's the header name, not the cmake component name.
        # The actual component is 'log', which IDF automatically adds to every build.
)
