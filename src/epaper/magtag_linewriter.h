/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MAGTAG_LINEWRITER_H
#define __MAGTAG_LINEWRITER_H

#include <stdint.h>
#include <stdbool.h>

#define Debug(__info) printk(__info)
uint8_t flip_invert(uint8_t column);
void epaper_WriteLine(uint8_t *str, uint8_t str_len, uint8_t line);
void double_flip_invert(uint8_t orig_column, uint8_t return_cols[2]);
void epaper_SendDoubleColumn(uint8_t *str, uint8_t str_len, bool full);
void epaper_WriteDoubleLine(uint8_t *str, uint8_t str_len, uint8_t line);
void EPD_2IN9D_FullRefreshDoubleLine(uint8_t *str, uint8_t str_len);
void epaper_autowrite(uint8_t *str, uint8_t str_len);

#endif

