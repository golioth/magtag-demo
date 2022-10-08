# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

list(APPEND OVERLAY_CONFIG "../credentials.conf")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lightdb)

add_subdirectory_ifdef(CONFIG_MAGTAG_COMMON ../magtag-common magtag-common)

target_sources(app PRIVATE src/main.c)
