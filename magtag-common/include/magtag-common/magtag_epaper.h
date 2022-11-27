/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MAGTAG_EPAPER_H
#define __MAGTAG_EPAPER_H

#include <stdint.h>
#include <stdbool.h>

// Display resolution
#define EPD_2IN9D_WIDTH   128
#define EPD_2IN9D_HEIGHT  296
#define EPD_2IN9D_PAGECNT EPD_2IN9D_WIDTH/8

#define ASCII_OFFSET    32  // Font start with space (char 32)
#define AUTOWRITE_REFRESH_AFTER_N_LINES	  16

/* Defines used for special-function x_lines values */
#define FULL_WIDTH  -1
#define CENTER      -2

/*
 * Fonts
 */
struct font_meta {
    const char *font_p;
    uint8_t letter_width_bits;
    uint8_t letter_height_bytes;
    bool inverted;
};

void epaper_init(void);
void epaper_initialize_hal(void);

void epaper_full_clear(void);
void epaper_show_golioth(void);
void epaper_show_full_frame(const char *frame);

void epaper_write(uint8_t *str, uint8_t str_len, uint8_t line, int16_t x_left,
	uint8_t font_size_in_lines);
void epaper_write_inverted(uint8_t *str, uint8_t str_len, uint8_t line, int16_t
	x_left, uint8_t font_size_in_lines);
/* Like epaper_write but you can specify a custom font */
void epaper_write_string(uint8_t *str, uint8_t str_len, uint8_t line, int16_t
	x_left, struct font_meta *font_m);

void epaper_write_line(uint8_t *str, uint8_t str_len, uint8_t line);
void epaper_write_line_2x(uint8_t *str, uint8_t str_len, uint8_t line);
void epaper_write_line_4x(uint8_t *str, uint8_t str_len, uint8_t line);
void epaper_autowrite(uint8_t *str, uint8_t str_len);

#endif

