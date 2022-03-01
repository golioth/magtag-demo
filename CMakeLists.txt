# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lightdb)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/epaper/EPD_2in9d.c)
target_sources(app PRIVATE src/epaper/DEV_Config.c)
target_sources(app PRIVATE src/epaper/ImageData.c)
target_sources(app PRIVATE src/ws2812/ws2812_control.c)
