/* originally based on Wavesahre E-Paper Library: */

/*
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "magtag-common/magtag_epaper.h"
#include "magtag_epaper_hal.h"
#include "GoliothLogo.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(golioth_epaper, LOG_LEVEL_DBG);

bool _display_asleep = true;

/*
 * Fonts
 */
#include "font5x8.h"
#include "ubuntu_monospaced_bold_10x16.h"
#include "ubuntu_monospaced_bold_19x32.h"

struct font_meta font_5x8 = { font5x8, 5, 1 };
struct font_meta font_10x16 = { u_mono_bold_10x16, 10, 2 };
struct font_meta font_19x32 = { u_mono_bold_19x32, 19, 4 };

/**
 * partial screen update LUT
**/
const unsigned char EPD_2IN9D_lut_vcom1[] = {
    0x00, 0x00, 0x00, 0x25, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_ww1[] = {
    0x02, 0x00, 0x00, 0x25, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_bw1[] = {
    0x48, 0x00, 0x00, 0x25, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_wb1[] = {
    0x84, 0x00, 0x00, 0x25, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char EPD_2IN9D_lut_bb1[] = {
    0x01, 0x00, 0x00, 0x25, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

bool EPD_2IN9D_IsAsleep(void) {
    return _display_asleep;
}

/******************************************************************************
function : Software reset
parameter:
******************************************************************************/
void EPD_2IN9D_Reset(void)
{
    DEV_Digital_Write(EPD_RST_PIN, 0);
    DEV_Delay_ms(10);
    DEV_Digital_Write(EPD_RST_PIN, 1);
    DEV_Delay_ms(10);
    _display_asleep = false;
}

/******************************************************************************
function : Send command
parameter:
     Reg : Command register
******************************************************************************/
void EPD_2IN9D_SendCommand(uint8_t Reg)
{
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Reg);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function : Send data
parameter:
    Data : Write data
******************************************************************************/
void EPD_2IN9D_SendData(uint8_t Data)
{
    DEV_Digital_Write(EPD_DC_PIN, 1);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_SPI_WriteByte(Data);
    DEV_Digital_Write(EPD_CS_PIN, 1);
}

/******************************************************************************
function : Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
void EPD_2IN9D_ReadBusy(void)
{
    Debug("e-Paper busy\r\n");
    uint8_t busy;
    do {
        EPD_2IN9D_SendCommand(0x71);
        busy = DEV_Digital_Read(EPD_BUSY_PIN);
        busy =!(busy & 0x01);
                DEV_Delay_ms(20);
    } while(busy);
    DEV_Delay_ms(20);
    Debug("e-Paper busy release\r\n");
}

/******************************************************************************
function : LUT download
parameter:
******************************************************************************/
void EPD_2IN9D_SetPartReg(void)
{
    EPD_2IN9D_SendCommand(0x01); //POWER SETTING
    EPD_2IN9D_SendData(0x03);
    EPD_2IN9D_SendData(0x00);
    EPD_2IN9D_SendData(0x2b);
    EPD_2IN9D_SendData(0x2b);
    EPD_2IN9D_SendData(0x03);

    EPD_2IN9D_SendCommand(0x06); //boost soft start
    EPD_2IN9D_SendData(0x17); //A
    EPD_2IN9D_SendData(0x17); //B
    EPD_2IN9D_SendData(0x17); //C

    EPD_2IN9D_SendCommand(0x04);
    EPD_2IN9D_ReadBusy();

    EPD_2IN9D_SendCommand(0x00); //panel setting
    EPD_2IN9D_SendData(0xbf); //LUT from OTP，128x296

    EPD_2IN9D_SendCommand(0x30); //PLL setting
    EPD_2IN9D_SendData(0x3C); // 3a 100HZ   29 150Hz 39 200HZ 31 171HZ

    EPD_2IN9D_SendCommand(0x61); //resolution setting
    EPD_2IN9D_SendData(EPD_2IN9D_WIDTH);
    EPD_2IN9D_SendData((EPD_2IN9D_HEIGHT >> 8) & 0xff);
    EPD_2IN9D_SendData(EPD_2IN9D_HEIGHT & 0xff);

    EPD_2IN9D_SendCommand(0x82); //vcom_DC setting
    EPD_2IN9D_SendData(0x12);

    EPD_2IN9D_SendCommand(0X50);
    EPD_2IN9D_SendData(0x97);

    unsigned int count;
    EPD_2IN9D_SendCommand(0x20);
    for(count=0; count<44; count++) {
        EPD_2IN9D_SendData(EPD_2IN9D_lut_vcom1[count]);
    }

    EPD_2IN9D_SendCommand(0x21);
    for(count=0; count<42; count++) {
        EPD_2IN9D_SendData(EPD_2IN9D_lut_ww1[count]);
    }

    EPD_2IN9D_SendCommand(0x22);
    for(count=0; count<42; count++) {
        EPD_2IN9D_SendData(EPD_2IN9D_lut_bw1[count]);
    }

    EPD_2IN9D_SendCommand(0x23);
    for(count=0; count<42; count++) {
        EPD_2IN9D_SendData(EPD_2IN9D_lut_wb1[count]);
    }

    EPD_2IN9D_SendCommand(0x24);
    for(count=0; count<42; count++) {
        EPD_2IN9D_SendData(EPD_2IN9D_lut_bb1[count]);
    }
}

/******************************************************************************
function : Turn On Display
parameter:
******************************************************************************/
void EPD_2IN9D_Refresh(void)
{
    EPD_2IN9D_SendCommand(0x12); //DISPLAY REFRESH
    DEV_Delay_ms(1); //!!!The delay here is necessary, 200uS at least!!!

    EPD_2IN9D_ReadBusy();
}

/******************************************************************************
function : Initialize the e-Paper register
parameter:
******************************************************************************/
void EPD_2IN9D_Init(void)
{
    if (_display_asleep) { EPD_2IN9D_Reset(); }
    EPD_2IN9D_SendCommand(0x00); //panel setting
    EPD_2IN9D_SendData(0x1f);    //LUT from OTP，KW-BF KWR-AF BWROTP 0f BWOTP 1f

    EPD_2IN9D_SendCommand(0X50); //VCOM AND DATA INTERVAL SETTING
    EPD_2IN9D_SendData(0x97); //WBmode:VBDF 17|D7 VBDW 97 VBDB 57 WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7

    EPD_2IN9D_SendCommand(0x04);
    EPD_2IN9D_ReadBusy();
}


void EPD_2IN9D_SendRepeatedBytePattern(uint8_t byte_pattern, uint16_t how_many) {
    for (uint16_t i = 0; i < how_many; i++) {
        EPD_2IN9D_SendData(byte_pattern);
    }
}

void EPD_2IN9D_SendPartialAddr(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    EPD_2IN9D_SendCommand(0x90); //resolution setting
    EPD_2IN9D_SendData(x); //x-start
    EPD_2IN9D_SendData(x+w - 1); //x-end

    EPD_2IN9D_SendData(0);
    EPD_2IN9D_SendData(y); //y-start
    EPD_2IN9D_SendData((y+h) / 256);
    EPD_2IN9D_SendData((y+h) % 256 - 1); //y-end
    EPD_2IN9D_SendData(0x01);
}

void EPD_2IN9D_SendPartialLineAddr(uint8_t line) {
    EPD_2IN9D_SendCommand(0x90); //resolution setting
    EPD_2IN9D_SendData(line*16); //x-start
    EPD_2IN9D_SendData((line*16)+16 - 1); //x-end

    EPD_2IN9D_SendData(0);
    EPD_2IN9D_SendData(0); //y-start
    EPD_2IN9D_SendData(296 / 256);
    EPD_2IN9D_SendData(296 % 256 - 1); //y-end
    EPD_2IN9D_SendData(0x01);
}

/******************************************************************************
function : Clear screen
parameter:
******************************************************************************/
void EPD_2IN9D_Clear(void)
{
    uint16_t Width, Height;
    Width = (EPD_2IN9D_WIDTH % 8 == 0)? (EPD_2IN9D_WIDTH / 8 ): (EPD_2IN9D_WIDTH / 8 + 1);
    Height = EPD_2IN9D_HEIGHT;

    EPD_2IN9D_SendCommand(0x13);
    EPD_2IN9D_SendRepeatedBytePattern(0xFF, Width*Height);

    EPD_2IN9D_SendCommand(0x10);
    EPD_2IN9D_SendRepeatedBytePattern(0xFF, Width*Height);

    EPD_2IN9D_Refresh();

    EPD_2IN9D_SendCommand(0x13);
    EPD_2IN9D_SendRepeatedBytePattern(0xFF, Width*Height);
}

/******************************************************************************
function : Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_2IN9D_Display(uint8_t *Image)
{
    uint16_t Width, Height;
    Width = (EPD_2IN9D_WIDTH % 8 == 0)? (EPD_2IN9D_WIDTH / 8 ): (EPD_2IN9D_WIDTH / 8 + 1);
    Height = EPD_2IN9D_HEIGHT;

    EPD_2IN9D_SendCommand(0x13);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            EPD_2IN9D_SendData(Image[i + j * Width]);
        }
    }
}

/******************************************************************************
function : Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void EPD_2IN9D_DisplayPart(uint8_t *Image)
{
    /* Set partial Windows */
    EPD_2IN9D_SetPartReg();
    EPD_2IN9D_SendCommand(0x91); //This command makes the display enter partial mode
    EPD_2IN9D_SendPartialAddr(0, 0, EPD_2IN9D_WIDTH, EPD_2IN9D_HEIGHT);
    EPD_2IN9D_Display(Image);
}

void EPD_2in9D_PartialClear(void) {
    EPD_2IN9D_SendCommand(0x91); //This command makes the display enter partial mode
    EPD_2IN9D_SendCommand(0x90); //resolution setting
    EPD_2IN9D_SendData(0); //x-start
    EPD_2IN9D_SendData(EPD_2IN9D_WIDTH - 1); //x-end

    EPD_2IN9D_SendData(0);
    EPD_2IN9D_SendData(0); //y-start
    EPD_2IN9D_SendData(EPD_2IN9D_HEIGHT / 256);
    EPD_2IN9D_SendData(EPD_2IN9D_HEIGHT % 256 - 1); //y-end
    EPD_2IN9D_SendData(0x01);

    uint16_t Width;
    Width = (EPD_2IN9D_WIDTH % 8 == 0)? (EPD_2IN9D_WIDTH / 8 ): (EPD_2IN9D_WIDTH / 8 + 1);

    /* send data */
    EPD_2IN9D_SendCommand(0x13);
    for (uint16_t j = 0; j < EPD_2IN9D_HEIGHT; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            EPD_2IN9D_SendData(0xFF);
        }
    }
}
/**
 * @brief Clear the displays
 *
 * This handles waking up the display, performing a full clear, and putting it
 * back to sleep
 *
 */
void epaper_FullClear(void) {
    EPD_2IN9D_Init();
    EPD_2IN9D_Clear();
    EPD_2IN9D_PowerOff();
}

void epaper_ShowFullFrame(const char *frame) {
    if (_display_asleep) {
        EPD_2IN9D_Init();
        EPD_2IN9D_SetPartReg();
    }
    EPD_2IN9D_Display((char *)frame);
    EPD_2IN9D_Refresh();
    EPD_2IN9D_Display((char *)frame);
    EPD_2IN9D_SetPartReg();
    EPD_2IN9D_PowerOff();
}

/**
 * @brief Initialize pins used to drive the display, execute the initialization
 * process, and display the Golioth logo
 *
 */
void epaper_init(void) {
    _display_asleep = true;
    LOG_INF("Setup ePaper pins");
    DEV_Module_Init();

    LOG_INF("ePaper Init and Clear");
    EPD_2IN9D_Init();
    EPD_2IN9D_Clear();

    LOG_INF("Show Golioth logo");
    epaper_ShowFullFrame((void *)golioth_logo); /* cast because function is not expecting a CONST array) */
}

/******************************************************************************
function :        Enter power off mode
parameter:
******************************************************************************/
void EPD_2IN9D_PowerOff(void)
{
    EPD_2IN9D_SendCommand(0X50);
    EPD_2IN9D_SendData(0xf7);
    EPD_2IN9D_SendCommand(0X02); //power off
    EPD_2IN9D_ReadBusy();
    _display_asleep = true;
}

/******************************************************************************
function :        Enter deep sleep mode
parameter:
******************************************************************************/
void EPD_2IN9D_Sleep(void)
{
    EPD_2IN9D_SendCommand(0X50);
    EPD_2IN9D_SendData(0xf7);
    EPD_2IN9D_SendCommand(0X02); //power off
    EPD_2IN9D_ReadBusy();
    EPD_2IN9D_SendCommand(0X07); //deep sleep
    EPD_2IN9D_SendData(0xA5);
    _display_asleep = true;
}

/**
 *
 * @brief Double each pixel to enlarge the font, and invert the value to match
 * the needs of the display
 *
 * @param orig_column   Font column input
 * @param return_cols   Two-byte array to store the results
 */
void double_invert(uint8_t orig_column, uint8_t return_cols[2]) {
    // Double the pixesl, the flip endianness and invert
    uint8_t upper_column = 0;
    uint8_t lower_column = 0;
    for (uint8_t i=0; i<4; i++)
    {
        if (orig_column & (1<<(i+4))) upper_column |= 0b11 << (i*2);
        if (orig_column & (1<<(i))) lower_column |= 0b11 << (i*2);
    }
    return_cols[1] = ~upper_column;
    return_cols[0] = ~lower_column;
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
void epaper_SendDoubleTextLine(uint8_t *str, uint8_t str_len, bool full)
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
                /* Artifically adding a space so decrement the index */
                uint8_t letter_column = font5x8[(5*letter)+column-1];
                double_invert(letter_column, send_col);
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

            /* Artifically adding a space, so loop 6 times, not 5 */
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
 * @brief Write one character from font file ePaper display RAM
 *
 * @param letter    The letter to write to the display
 * @param font_p    Pointer to the font array
 * @param bytes_in_letter    Total bytes neede from the font file for this
 *                                 letter
 */
void epaper_LetterToRam(uint8_t letter, struct font_meta *font_m)
{
    /* Write space if letter is out of bounds */
    if ((letter < ' ') || (letter> '~')) { letter = ' '; }

    /* ASCII space=32 but font file begins at 0 */
    letter -= ASCII_OFFSET;

    uint8_t bytes_in_letter = font_m->letter_width_bits * font_m->letter_height_bytes;

    for (uint16_t i=0; i<bytes_in_letter; i++) {
        uint8_t letter_column = *(font_m->font_p + (letter*bytes_in_letter) + i);
        EPD_2IN9D_SendData(~letter_column);
    }
}

void epaper_StringToRam(uint8_t *str, uint8_t str_len, uint8_t line, int8_t show_n_chars, struct font_meta *font_m)
{
    uint8_t letter;
    uint8_t letter_column;
    uint8_t char_count;
    uint8_t line_space_front = 0;
    uint8_t line_space_back = 0;

    if (show_n_chars < 0) {
        char_count = EPD_2IN9D_HEIGHT/font_m->letter_width_bits;
        /* Calculate leftover columns */
        uint16_t text_columns = char_count * font_m->letter_width_bits;
        line_space_front = (EPD_2IN9D_HEIGHT - (text_columns))/2;
        line_space_back = EPD_2IN9D_HEIGHT - text_columns - line_space_front;
        /* Adjust for font height */
        line_space_front *= font_m->letter_height_bytes;
        line_space_back *= font_m->letter_height_bytes;
        //Unused columns
        for (uint8_t i=0; i<line_space_front; i++) EPD_2IN9D_SendData(0xff);
    }
    else {
        char_count = show_n_chars;
    }

    for (uint8_t j=1; j<=char_count; j++) {
        if (char_count-j >= str_len) {
            /* String too short, send a space */
            letter = ' ';
        }
        else {
            letter = str[char_count-j];
        }

        epaper_LetterToRam(letter, font_m);
    }

    if (show_n_chars < 0) {
        //Unused columns
        for (uint8_t i=0; i<line_space_back; i++) EPD_2IN9D_SendData(0xff);
    }
}

/**
 * @brief Write a string to ePaper display
 *
 * Strings will be truncated to fit in avaialble display space. Strings are
 * place on the screen by selecting one a line number (lines are 8-bits tall)
 * as y value and the pixel location as an x value with the left column begging
 * with 295 and decrementing as you move to the right. Specify the entire row
 * by passing a negative x_left value.
 *
 * This function can be used for any size of font. A font_meta struct is passed
 * that includes a pointer to the monospace font array, and the height and width
 * of a single character.
 *
 * @param *str  Pointer to the string to be written
 * @param str_len  Length of the string to be written
 * @param line  Line on display; 0=top 15=bottom
 * @param x_left  Pixel position to begin; 295=left 0=right
 * @param *font_m  Pointer to a font_meta struct describing the font array
 */
void epaper_WriteString(uint8_t *str,
                        uint8_t str_len,
                        uint8_t line,
                        int16_t x_left,
                        struct font_meta *font_m)
{
    /* Bounding */
    line %= EPD_2IN9D_PAGECNT;
    if (line > (EPD_2IN9D_PAGECNT - font_m->letter_height_bytes)) { return; }
    if ((x_left < font_m->letter_width_bits) && (x_left > 0)) { return; }

    /* Calculate how much screen space is available */
    int16_t char_limit;
    uint16_t col_start;
    uint16_t col_width;
    if (x_left < 0) {
        /* Full screen width write */
        char_limit = -1;
        col_start = 0;
        col_width = EPD_2IN9D_HEIGHT;
    }
    else {
        if (str_len*font_m->letter_width_bits > x_left) {
            /* Truncate number chars to fit on screen */
            char_limit = x_left/font_m->letter_width_bits;
        }
        else {
            /* All chars can fit on screen */
            char_limit = str_len;
        }

        col_width = char_limit * font_m->letter_width_bits;
        col_start = x_left - col_width;
    }

    uint8_t pixel_height = font_m->letter_height_bytes * 8;

    for (uint8_t i=0; i<2; i++) {
        EPD_2IN9D_SendCommand(0x91);
        EPD_2IN9D_SendPartialAddr(line*8,
                                  col_start,
                                  pixel_height,
                                  col_width);
        EPD_2IN9D_SendCommand(0x13);
        epaper_StringToRam(str, str_len, line, char_limit, font_m);
        EPD_2IN9D_SendCommand(0x92);

        if (i==0) {
            /*
             * Refresh display the first time, then write data again to prewind
             * the "last-frame" into display memory. Do not refresh the second
             * time so that a partial write possible
             */
            EPD_2IN9D_Refresh();
        }
    }
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
    epaper_WriteString(str, str_len, line, -1, &font_5x8);
}

void epaper_WriteLargeLine(uint8_t *str, uint8_t str_len, uint8_t line) {
    epaper_WriteString(str, str_len, line, -1, &font_10x16);
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
    epaper_SendDoubleTextLine(str, str_len, false);
    EPD_2IN9D_SendCommand(0x92);

    /* Refresh display, then write data again to prewind the "last-frame" */
    EPD_2IN9D_Refresh();

    EPD_2IN9D_SendCommand(0x91);
    EPD_2IN9D_SendPartialLineAddr(line);
    EPD_2IN9D_SendCommand(0x13);
    epaper_SendDoubleTextLine(str, str_len, false);
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
    EPD_2IN9D_Init();
    if (line > 0) {
        if ((line%AUTOWRITE_REFRESH_AFTER_N_LINES == 0) || EPD_2IN9D_IsAsleep()) {
            EPD_2IN9D_Clear();
        }
    }
    EPD_2IN9D_SetPartReg();
    epaper_WriteLargeLine(str, str_len, line);
    ++line;

    EPD_2IN9D_PowerOff();
}


