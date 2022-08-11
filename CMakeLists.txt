# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lightdb)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/epaper/magtag_epaper.c)
target_sources(app PRIVATE src/epaper/magtag_epaper_hal.c)
target_sources(app PRIVATE src/ws2812/ws2812_control.c)
target_sources(app PRIVATE src/buttons/buttons.c)
target_sources(app PRIVATE src/accelerometer/accel.c)
