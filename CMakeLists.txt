# Anchor Drag Pro - ESP-IDF Project
# Author: Colin Bitterfield
# Email: colin@bitterfield.com
# Date Created: 2025-12-19
# Date Updated: 2025-12-19

cmake_minimum_required(VERSION 3.16)

# LVGL configuration
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)
idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_INCLUDE_SIMPLE=1" APPEND)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(anchor-drag-pro)
