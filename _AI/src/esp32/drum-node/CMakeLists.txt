cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS
    # Shared components used by both core-node and drum-node (espnow_transport, etc.)
    "${CMAKE_CURRENT_LIST_DIR}/../shared"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(drum_node)
