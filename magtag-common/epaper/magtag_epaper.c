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

#include "magtag_epaper_hal.h"
#include "GoliothLogo.h"

/* Private Function Prototypes */
static bool EPD_2IN9D_IsAsleep(void);
static void EPD_2IN9D_Reset(void);
static void EPD_2IN9D_SendCommand(uint8_t Reg);
static void EPD_2IN9D_SendData(uint8_t Data);
static void EPD_2IN9D_ReadBusy(void);
static void EPD_2IN9D_SetPartReg(void);
static void EPD_2IN9D_Refresh(void);
static void EPD_2IN9D_Init(void);
static void EPD_2IN9D_SendRepeatedBytePattern(uint8_t byte_pattern, uint16_t how_many);
static void EPD_2IN9D_SendPartialAddr(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
static void EPD_2IN9D_SendPartialLineAddr(uint8_t line);
static void EPD_2IN9D_Clear(void);
static void EPD_2IN9D_Display(uint8_t *Image);
static void EPD_2IN9D_DisplayPart(uint8_t *Image);
static void EPD_2in9D_PartialClear(void);
static void EPD_2IN9D_DeepSleep(void);
static void EPD_2IN9D_Standby(void);
static void epaper_letter_to_ram(uint8_t letter, struct font_meta *font_m);
static void epaper_string_to_ram(uint8_t *str, uint8_t str_len, uint8_t line, int8_t show_n_chars, struct font_meta *font_m);

/**
 * There's no way around this Zephyr-specific log register. This ifdef statement
 * tests for the following symbol: CONFIG_KERNEL_BIN_NAME="zephyr"
 **/
#ifdef CONFIG_KERNEL_BIN_NAME
    LOG_MODULE_REGISTER(golioth_epaper, EPAPER_LOG_LEVEL);
#endif

bool _display_asleep = true;

/*
 * Fonts
 */
#include "font6x8.h"
#include "ubuntu_monospaced_bold_10x16.h"
#include "ubuntu_monospaced_bold_19x32.h"

struct font_meta font_6x8 = { font6x8, 6, 1, false };
struct font_meta font_10x16 = { (const char *)u_mono_bold_10x16, 10, 2, false };
struct font_meta font_19x32 = { (const char *)u_mono_bold_19x32, 19, 4, false };

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

static bool EPD_2IN9D_IsAsleep(void) {
    return _display_asleep;
}

/******************************************************************************
function : Software reset
parameter:
******************************************************************************/
static void EPD_2IN9D_Reset(void)
{
    epaper_hal_digital_write(EPD_RST_PIN, 0);
    epaper_delay_ms(10);
    epaper_hal_digital_write(EPD_RST_PIN, 1);
    epaper_delay_ms(10);
    _display_asleep = false;
}

/******************************************************************************
function : Send command
parameter:
     Reg : Command register
******************************************************************************/
static void EPD_2IN9D_SendCommand(uint8_t Reg)
{
    epaper_hal_digital_write(EPD_DC_PIN, 0);
    epaper_hal_digital_write(EPD_CS_PIN, 0);
    epaper_hal_spi_writebyte(Reg);
    epaper_hal_digital_write(EPD_CS_PIN, 1);
}

/******************************************************************************
function : Send data
parameter:
    Data : Write data
******************************************************************************/
static void EPD_2IN9D_SendData(uint8_t Data)
{
    epaper_hal_digital_write(EPD_DC_PIN, 1);
    epaper_hal_digital_write(EPD_CS_PIN, 0);
    epaper_hal_spi_writebyte(Data);
    epaper_hal_digital_write(EPD_CS_PIN, 1);
}

/******************************************************************************
function : Wait until the busy_pin goes LOW
parameter:
******************************************************************************/
static void EPD_2IN9D_ReadBusy(void)
{
    EPAPER_LOG_DBG("e-Paper busy");
    uint8_t busy;
    do {
        EPD_2IN9D_SendCommand(0x71);
        busy = epaper_hal_digital_read(EPD_BUSY_PIN);
        busy =!(busy & 0x01);
                epaper_delay_ms(20);
    } while(busy);
    epaper_delay_ms(20);
    EPAPER_LOG_DBG("e-Paper busy release");
}

/******************************************************************************
function : LUT download
parameter:
******************************************************************************/
static void EPD_2IN9D_SetPartReg(void)
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

}

/******************************************************************************
function : Turn On Display
parameter:
******************************************************************************/
static void EPD_2IN9D_Refresh(void)
{
    EPD_2IN9D_SendCommand(0x12); //DISPLAY REFRESH
    epaper_delay_ms(1); //!!!The delay here is necessary, 200uS at least!!!

    EPD_2IN9D_ReadBusy();
}

/******************************************************************************
function : Initialize the e-Paper register
parameter:
******************************************************************************/
static void EPD_2IN9D_Init(void)
{
    if (_display_asleep) {
        EPD_2IN9D_Reset();

        /* Rewrite LUTs after reset */
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
    EPD_2IN9D_SendCommand(0x00); //panel setting
    EPD_2IN9D_SendData(0x1f);    //LUT from OTP，KW-BF KWR-AF BWROTP 0f BWOTP 1f

    EPD_2IN9D_SendCommand(0X50); //VCOM AND DATA INTERVAL SETTING
    EPD_2IN9D_SendData(0x97); //WBmode:VBDF 17|D7 VBDW 97 VBDB 57 WBRmode:VBDF F7 VBDW 77 VBDB 37  VBDR B7

    EPD_2IN9D_SendCommand(0x04);
    EPD_2IN9D_ReadBusy();
}


static void EPD_2IN9D_SendRepeatedBytePattern(uint8_t byte_pattern, uint16_t how_many) {
    for (uint16_t i = 0; i < how_many; i++) {
        EPD_2IN9D_SendData(byte_pattern);
    }
}

static void EPD_2IN9D_SendPartialAddr(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    EPD_2IN9D_SendCommand(0x91); //enter partial mode
    EPD_2IN9D_SendCommand(0x90); //resolution setting
    EPD_2IN9D_SendData(x); //x-start
    EPD_2IN9D_SendData(x+w - 1); //x-end

    EPD_2IN9D_SendData(0);
    EPD_2IN9D_SendData(y); //y-start
    EPD_2IN9D_SendData((y+h) / 256);
    EPD_2IN9D_SendData((y+h) % 256 - 1); //y-end
    EPD_2IN9D_SendData(0x01);
}

static void EPD_2IN9D_SendPartialLineAddr(uint8_t line) {
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
static void EPD_2IN9D_Clear(void)
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
static void EPD_2IN9D_Display(uint8_t *Image)
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
static void EPD_2IN9D_DisplayPart(uint8_t *Image)
{
    /* Set partial Windows */
    EPD_2IN9D_SetPartReg();
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
void epaper_full_clear(void) {
    EPD_2IN9D_Init();
    EPD_2IN9D_Clear();
    EPD_2IN9D_Standby();
}

void epaper_standby(void) {
    EPD_2IN9D_Standby();
}

void epaper_deep_sleep(void) {
    EPD_2IN9D_DeepSleep();
}

bool epaper_is_asleep(void) {
    return EPD_2IN9D_IsAsleep();
}

void epaper_show_full_frame(const char *frame) {
    if (_display_asleep) {
        EPD_2IN9D_Init();
        EPD_2IN9D_SetPartReg();
    }
    EPD_2IN9D_Display((unsigned char *)frame);
    EPD_2IN9D_Refresh();
    EPD_2IN9D_Display((unsigned char *)frame);
    EPD_2IN9D_SetPartReg();
    EPD_2IN9D_Standby();
}

/**
 * @brief Run HAL initialization
 *
 * This calls the OS/implementation specific function that configures hardware
 * pins, logging, and delay functions. This will be called by epaper_init().
 */
void epaper_initialize_hal(void) {
    _display_asleep = true;
    EPAPER_LOG_INF("Setup ePaper pins");
    epaper_hal_setup_hardware();
}

void epaper_show_golioth(void) {
    EPD_2IN9D_Init();
    EPAPER_LOG_INF("Show Golioth logo");
    epaper_show_full_frame((void *)golioth_logo); /* cast because function is not expecting a CONST array) */
    EPD_2IN9D_Standby();
}

/**
 * @brief Initialize pins used to drive the display, execute the initialization
 * process, and display the Golioth logo
 *
 */
void epaper_init(void) {
    epaper_initialize_hal();
    EPAPER_LOG_INF("ePaper Init and Clear");
    EPD_2IN9D_Init();
    EPD_2IN9D_Clear();
    epaper_show_golioth();
}

/******************************************************************************
function :        Enter power off mode
parameter:
******************************************************************************/
static void EPD_2IN9D_Standby(void)
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
static void EPD_2IN9D_DeepSleep(void)
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
 * @brief write one character from font file epaper display ram
 *
 * @param letter    the letter to write to the display
 * @param font_p    pointer to the font array
 * @param bytes_in_letter    total bytes neede from the font file for this
 *                                 letter
 */
static void epaper_letter_to_ram(uint8_t letter, struct font_meta *font_m)
{
    /* Write space if letter is out of bounds */
    if ((letter < ' ') || (letter> '~')) { letter = ' '; }

    /* ASCII space=32 but font file begins at 0 */
    letter -= ASCII_OFFSET;

    uint8_t bytes_in_letter = font_m->letter_width_bits * font_m->letter_height_bytes;

    for (uint16_t i=0; i<bytes_in_letter; i++) {
        uint8_t letter_column = *(font_m->font_p + (letter*bytes_in_letter) + i);
        if (font_m->inverted) {
            EPD_2IN9D_SendData(letter_column);
        }
        else {
            EPD_2IN9D_SendData(~letter_column);
        }
    }
}

static void epaper_string_to_ram(uint8_t *str, uint8_t str_len, uint8_t line, int8_t show_n_chars, struct font_meta *font_m)
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

        epaper_letter_to_ram(letter, font_m);
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
void epaper_write_string(uint8_t *str,
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
    if (x_left < -2) {
        EPAPER_LOG_ERR("Unrecognized x_left value: %d", x_left);
        return;
    }
    else if (x_left == FULL_WIDTH) {
        /* Full screen width write */
        DO_FULL_WIDTH:
        char_limit = FULL_WIDTH;
        col_start = 0;
        col_width = EPD_2IN9D_HEIGHT;
    }
    else {
        uint16_t string_cols_needed = str_len*font_m->letter_width_bits;
        if (x_left == CENTER) {
            /* Trick to center the text on screen */
            if (string_cols_needed < EPD_2IN9D_HEIGHT-1) {
                x_left = EPD_2IN9D_HEIGHT-((EPD_2IN9D_HEIGHT-string_cols_needed)/2);
            }
            else {
                /* Too wide, so used full width (will center) */
                x_left = FULL_WIDTH;
                goto DO_FULL_WIDTH;
            }

        }
        if (string_cols_needed > x_left) {
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
        EPD_2IN9D_SendPartialAddr(line*8,
                                  col_start,
                                  pixel_height,
                                  col_width);
        EPD_2IN9D_SendCommand(0x13);
        epaper_string_to_ram(str, str_len, line, char_limit, font_m);
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

struct font_meta* get_font_meta(uint8_t linesize) {
    switch(linesize) {
        case 1:
            return &font_6x8;
        case 2:
            return &font_10x16;
        case 4:
            return &font_19x32;
        default:
            EPAPER_LOG_ERR("Unsupported font height: %d", linesize);
            return 0;
    }
}
/**
 * @brief Use partial refresh to write text of different sizes to the screen
 *
 * This function will wake the screen, write your text using a font height of
 * your choice, and place the screen back into power-down mode
 *
 * @param *str  String to be written to display
 * @param str_len  Length of string to be written
 * @param line  Line of display as y value; 0=top 15=bottom
 * @param x_left  Pixel of display as x value;
 *                295=left 0=right -1=use full line -2=center text
 * @param font_size_in_lines  Height of characters (1, 2, or 4)
 */
void epaper_write(uint8_t *str, uint8_t str_len, uint8_t line, int16_t x_left, uint8_t font_size_in_lines)
{
    struct font_meta *font_m = get_font_meta(font_size_in_lines);
    if (font_m == 0) { return; }

    EPD_2IN9D_Init();
    EPD_2IN9D_SetPartReg();
    epaper_write_string(str, str_len, line, x_left, font_m);
    EPD_2IN9D_Standby();
}

/**
 * @brief Use partial refresh to write inverted text of different sizes to the screen
 *
 * This function will wake the screen, write your text using a font height of
 * your choice, and place the screen back into power-down mode
 *
 * @param *str  String to be written to display
 * @param str_len  Length of string to be written
 * @param line  Line of display as y value; 0=top 15=bottom
 * @param x_left  Pixel of display as x value;
 *                295=left 0=right -1=use full line -2=center text
 * @param font_size_in_lines  Height of characters (1, 2, or 4)
 */
void epaper_write_inverted(uint8_t *str, uint8_t str_len, uint8_t line, int16_t x_left, uint8_t font_size_in_lines)
{
    struct font_meta *font_m = get_font_meta(font_size_in_lines);
    if (font_m == 0) { return; }

    font_m->inverted = true;
    epaper_write(str, str_len, line, x_left, font_size_in_lines);
    font_m->inverted = false;
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
void epaper_write_line(uint8_t *str, uint8_t str_len, uint8_t line)
{
    epaper_write_string(str, str_len, line, FULL_WIDTH, &font_6x8);
}

void epaper_write_line_2x(uint8_t *str, uint8_t str_len, uint8_t line) {
    epaper_write_string(str, str_len, line, FULL_WIDTH, &font_10x16);
}

void epaper_write_line_4x(uint8_t *str, uint8_t str_len, uint8_t line) {
    epaper_write_string(str, str_len, line, FULL_WIDTH, &font_19x32);
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
    epaper_write_line_2x(str, str_len, (line++)*2);

    EPD_2IN9D_Standby();
}
