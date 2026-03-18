cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS
    # Waveshare hardware drivers (rgb_lcd_port, touch, lvgl_port, i2c, gpio, io_extension)
    "${CMAKE_CURRENT_LIST_DIR}/vendor"
    # lvgl__lvgl is nested one level inside lvgl_port — must be listed separately
    "${CMAKE_CURRENT_LIST_DIR}/vendor/lvgl_port"
    # Shared components used by both core-node and drum-node (espnow_transport, etc.)
    "${CMAKE_CURRENT_LIST_DIR}/../shared"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include_directories($ENV{IDF_PATH}/components/esp_system/include)

add_compile_options("-Wno-attributes")

project(core_node_ui)

# Build a SPIFFS image from spiffs_data/ and flash it automatically with
# `idf.py flash`. The partition size (5M) must match partitions.csv.
spiffs_create_partition_image(spiffs spiffs_data FLASH_IN_PROJECT)
