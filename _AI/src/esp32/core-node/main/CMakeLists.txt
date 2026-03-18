idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES ui espnow_transport lvgl_port rgb_lcd_port touch spiffs
    WHOLE_ARCHIVE
)

# Suppress format warnings that originate inside the LVGL library itself.
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
