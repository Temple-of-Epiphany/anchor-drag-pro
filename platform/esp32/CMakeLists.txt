# Anchor Drag Pro — ESP-IDF project root
#
# Phase B (Workstream 11 / #67) brings LVGL back; lv_conf.h lives at
# platform/esp32/lv_conf.h and is pointed-at via LV_CONF_PATH so the
# lvgl/lvgl managed component picks it up without depending on the
# include-path search order.
#
# Author:  Colin Bitterfield <colin@bitterfield.com>
# License: Proprietary

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# LVGL configuration — must be set before project() so it propagates to the
# managed component build.
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)
idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_INCLUDE_SIMPLE=1" APPEND)

project(anchor-drag-pro)
