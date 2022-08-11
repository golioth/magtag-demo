/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "magtag_linewriter.h"
#include "magtag_epaper.h"
#include "font5x8.h"

/**
 * @brief Flip endianness of input and invert the value to match the needs of
 * the display
 *
 * @param column    Font column input
 * @return uint8_t  Flipped and inverted column
 */
uint8_t flip_invert(uint8_t column) {
    // Flip endianness and invert
    uint8_t ret_column = 0;
    for (uint8_t i=0; i<8; i++)
    {
        if (~column & (1<<i))
        {
            ret_column |= (1<<(7-i));
        }
    }
    return ret_column;
}

/**
 * @brief Flip endianness of input, double each pixel to enlarge the font, and
 * invert the value to match the needs of the display
 *
 * @param orig_column   Font column input
 * @param return_cols   Two-byte array to store the results
 */
void double_flip_invert(uint8_t orig_column, uint8_t return_cols[2]) {
    // Double the pixesl, the flip endianness and invert
    uint8_t upper_column = 0;
    uint8_t lower_column = 0;
    for (uint8_t i=0; i<4; i++)
    {
        if (orig_column & (1<<(i+4))) upper_column |= 0b11 << (i*2);
        if (orig_column & (1<<(i))) lower_column |= 0b11 << (i*2);
    }
    return_cols[0] = flip_invert(upper_column);
    return_cols[1] = flip_invert(lower_column);
}

/**
 * @brief Write data for showing text on display
 *
 * This is meant to be used with partial writes.
 *
 * @param str       String to display
 * @param str_len   Length of string
 */
void epaper_SendColumn(uint8_t *str, uint8_t str_len)
{
    uint8_t send_col;
    uint8_t letter;
    uint8_t column = 0;
    uint8_t str_idx = 48;
    EPD_2IN9D_SendData(0xff); //Unused column
    for (uint16_t j = 0; j < 294; j++) {
        for (uint16_t i = 0; i < 1; i++) {
            if (column == 0 || str_idx >= str_len)
            {
                send_col = 0xff;
            }
            else
            {
                if (str[str_idx] < 32 || str[str_idx] > 127)
                {
                    //Out of bounds, print a space
                    letter = 0;
                }
                else
                {
                    letter = str[str_idx] - 32;
                }
                send_col = flip_invert(font5x8[(5*letter)+(5-column)]);
            }
            EPD_2IN9D_SendData(send_col);
            if (++column > 5)
            {
                column = 0;
                --str_idx;
            }
        }
    }
    EPD_2IN9D_SendData(0xff); //Unused column
}

/**
 * @brief Write data for double-height text to display
 *
 * Tihs can be used for both partial and full writes.
 *
 * @param str       String to display
 * @param str_len   Length of string
 * @param full      True if called by a full refresh, false for a partial
 * refresh
 */
void epaper_SendDoubleColumn(uint8_t *str, uint8_t str_len, bool full)
{
    uint8_t send_col[2] = {0};
    uint8_t letter;
    uint8_t column = 0;
    uint8_t str_idx = 23;
    uint8_t vamp_count = full? 64:8;
    for (uint8_t i=0; i<vamp_count; i++) EPD_2IN9D_SendData(0xff); //Unused columns
    for (uint16_t j = 0; j < 144; j++) {
        for (uint16_t i = 0; i < 1; i++) {
            if (column == 0 || str_idx >= str_len)
            {
                send_col[0] = 0xff;
                send_col[1] = 0xff;
            }
            else
            {
                if (str[str_idx] < 32 || str[str_idx] > 127)
                {
                    //Out of bounds, print a space
                    letter = 0;
                }
                else
                {
                    letter = str[str_idx] - 32;
                }
                uint8_t letter_column = font5x8[(5*letter)+(5-column)];
                double_flip_invert(letter_column, send_col);
            }

            for (uint8_t i=0; i<2; i++)
            {
                EPD_2IN9D_SendData(send_col[1]);
                EPD_2IN9D_SendData(send_col[0]);
                if (full)
                {
                    for (uint8_t j=0; j<14; j++) EPD_2IN9D_SendData(0xff); //Unused columns
                }
            }

            if (++column > 5)
            {
                column = 0;
                --str_idx;
            }
        }
    }
    for (uint8_t i=0; i<vamp_count; i++) EPD_2IN9D_SendData(0xff); //Unused columns
}

/**
 * @brief Use partial refresh to show string on one line of the display
 *
 * Display is considered protrait-mode 296x128. This leaves 16 lines that are
 * 8-bits tall, and 296 columns
 *
 * @param str           string to be written to display
 * @param str_len       numer of characters in string
 * @param line          0..16
 */
void epaper_WriteLine(uint8_t *str, uint8_t str_len, uint8_t line)
{
    line %= 16;  /* Bounding */

    EPD_2IN9D_SendCommand(0x91);
    EPD_2IN9D_SendPartialAddr(line*8, 0, 8, 296);
    EPD_2IN9D_SendCommand(0x13);
    epaper_SendColumn(str, str_len);
    EPD_2IN9D_SendCommand(0x92);

    /* Refresh display, then write data again to prewind the "last-frame" */
    EPD_2IN9D_Refresh();

    EPD_2IN9D_SendCommand(0x91);
    EPD_2IN9D_SendPartialAddr(line*8, 0, 8, 296);
    EPD_2IN9D_SendCommand(0x13);
    epaper_SendColumn(str, str_len);
    EPD_2IN9D_SendCommand(0x92);

    /* Don't refresh, this data will be used in the next partial refresh */
}

/**
 * @brief Use partial refresh to show string on one double-sized line of the
 * display
 *
 * @param str           string to be written to display
 * @param str_len       numer of characters in string
 * @param line          0..8
 */
void epaper_WriteDoubleLine(uint8_t *str, uint8_t str_len, uint8_t line)
{
    line %= 8;  /* Bounding */

    EPD_2IN9D_SendCommand(0x91);
    EPD_2IN9D_SendPartialLineAddr(line); 
    EPD_2IN9D_SendCommand(0x13);
    epaper_SendDoubleColumn(str, str_len, false);
    EPD_2IN9D_SendCommand(0x92);

    /* Refresh display, then write data again to prewind the "last-frame" */
    EPD_2IN9D_Refresh();

    EPD_2IN9D_SendCommand(0x91);
    EPD_2IN9D_SendPartialLineAddr(line);    
    EPD_2IN9D_SendCommand(0x13);
    epaper_SendDoubleColumn(str, str_len, false);
    EPD_2IN9D_SendCommand(0x92);

    /* Don't refresh, this data will be used in the next partial refresh */
}

/**
 * @brief Write double-sized lines of text to ePaper display, automatically
 * handling screen refreshing
 *
 * @param str       String to write to next available line
 * @param str_len   Length of string
 */
void epaper_autowrite(uint8_t *str, uint8_t str_len)
{
    static int8_t line = 0;
    if (line > 0) {
        if ((line%32 == 0) || EPD_2IN9D_IsAsleep()) {
            EPD_2IN9D_Init();
            EPD_2IN9D_Clear();
            EPD_2IN9D_SetPartReg();
        }
    }
    epaper_WriteDoubleLine(str, str_len, line%8);
    ++line;
}

