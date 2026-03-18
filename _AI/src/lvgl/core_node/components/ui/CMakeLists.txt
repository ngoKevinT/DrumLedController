idf_component_register(
    SRCS
        "ui.c"
        "ui_main.c"
        "ui_global_settings.c"
        "ui_drum_settings.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl
)
