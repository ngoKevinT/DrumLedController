cmake_minimum_required(VERSION 3.5)

# Reuse all Waveshare hardware driver components from the reference example.
# This avoids duplicating low-level drivers (rgb_lcd_port, touch, lvgl_port, etc.)
# and keeps our project focused on application-layer code only.
set(EXTRA_COMPONENT_DIRS
    # First level: picks up rgb_lcd_port, touch, lvgl_port, i2c, io_extension, etc.
    "${CMAKE_CURRENT_LIST_DIR}/../_examples/13_LVGL_TRANSPLANT/components"
    # Second level: picks up lvgl__lvgl (the LVGL library itself, nested one level deeper)
    "${CMAKE_CURRENT_LIST_DIR}/../_examples/13_LVGL_TRANSPLANT/components/lvgl_port"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include_directories($ENV{IDF_PATH}/components/esp_system/include)

add_compile_options("-Wno-attributes")

project(core_node_ui)

# Build a SPIFFS image from spiffs_data/ and flash it automatically with
# `idf.py flash`. The partition size (5M) must match partitions.csv.
spiffs_create_partition_image(spiffs spiffs_data FLASH_IN_PROJECT)
