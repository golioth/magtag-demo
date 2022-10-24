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

#define AUTOWRITE_REFRESH_AFTER_N_LINES	  16

/*
 * Fonts
 */
struct font_meta {
    const char *font_p;
    uint8_t letter_width_bits;
    uint8_t letter_height_bytes;
};

bool EPD_2IN9D_IsAsleep(void);
void EPD_2IN9D_Reset(void);
void EPD_2IN9D_SendCommand(uint8_t Reg);
void EPD_2IN9D_SendData(uint8_t Data);
void EPD_2IN9D_ReadBusy(void);
void EPD_2IN9D_SetPartReg(void);
void EPD_2IN9D_Refresh(void);
void EPD_2IN9D_Init(void);
void EPD_2IN9D_SendRepeatedBytePattern(uint8_t byte_pattern, uint16_t how_many);
void EPD_2IN9D_SendPartialAddr(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void EPD_2IN9D_SendPartialLineAddr(uint8_t line);
void EPD_2IN9D_Clear(void);
void EPD_2IN9D_Display(uint8_t *Image);
void EPD_2IN9D_DisplayPart(uint8_t *Image);
void EPD_2in9D_PartialClear(void);
void epaper_FullClear(void);
void epaper_ShowFullFrame(const char *frame);
void epaper_init(void);
void EPD_2IN9D_Sleep(void);
void EPD_2IN9D_PowerOff(void);

uint8_t flip_invert(uint8_t column);
void double_flip_invert(uint8_t orig_column, uint8_t return_cols[2]);
void epaper_SendTextLine(uint8_t *str, uint8_t str_len);
void epaper_SendDoubleTextLine(uint8_t *str, uint8_t str_len, bool full);
void epaper_SendLetter(uint8_t letter, const char *font_p, uint8_t bytes_in_letter);
void epaper_SendLetterNew(uint8_t letter, struct font_meta *font_m);
void epaper_WriteLine(uint8_t *str, uint8_t str_len, uint8_t line);
void epaper_WriteDoubleLine(uint8_t *str, uint8_t str_len, uint8_t line);
void epaper_SendString(uint8_t *str, uint8_t str_len, uint8_t line, int8_t show_n_chars, struct font_meta *font_m);
void epaper_WriteString(uint8_t *str, uint8_t str_len, uint8_t line, int16_t x_left, struct font_meta *font_m);
void epaper_WriteLargeString(uint8_t *str, uint8_t str_len, uint8_t line, int16_t x_left);
void epaper_WriteLargeLine(uint8_t *str, uint8_t str_len, uint8_t line);
void epaper_WriteLargeLetter(uint8_t letter, uint16_t x, uint8_t line);
void epaper_WriteVeryLargeLetter(uint8_t letter, uint16_t x, uint8_t line);
void epaper_autowrite(uint8_t *str, uint8_t str_len);

#endif

