/*****************************************************************************
* | File      	:   DEV_Config.h
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2020-02-19
* | Info        :
#
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
#ifndef _EPAPER_HAL_CONFIG_H_
#define _EPAPER_HAL_CONFIG_H_

#include <zephyr/logging/log.h>
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>

/**
 * Logging
 **/
#define EPAPER_LOG_LEVEL   LOG_LEVEL_DBG
#define EPAPER_LOG_ERR(...)    Z_LOG(LOG_LEVEL_ERR, __VA_ARGS__)
#define EPAPER_LOG_WRN(...)    Z_LOG(LOG_LEVEL_WRN, __VA_ARGS__)
#define EPAPER_LOG_INF(...)    Z_LOG(LOG_LEVEL_INF, __VA_ARGS__)
#define EPAPER_LOG_DBG(...)    Z_LOG(LOG_LEVEL_DBG, __VA_ARGS__)

/**
 * delay x ms
**/
#define epaper_delay_ms(__xms) k_msleep(__xms)

/**
 * GPIO config
**/
#define EPD_SCK_PIN  13
#define EPD_MOSI_PIN 14
#define EPD_CS_PIN   15
#define EPD_RST_PIN  26
#define EPD_DC_PIN   27
#define EPD_BUSY_PIN 25

#define GPIO_PIN_SET   1
#define GPIO_PIN_RESET 0
#define HIGH 1
#define LOW 0

/**
 * GPIO read and write
**/

uint8_t epaper_hal_digital_read(uint8_t pin);
void epaper_hal_digital_write(uint8_t pin, uint8_t value);
void epaper_hal_setup_hardware(void);
void epaper_hal_spi_writebyte(uint8_t data);

#endif
