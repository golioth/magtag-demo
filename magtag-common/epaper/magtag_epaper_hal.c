/*****************************************************************************
* | File      	:   DEV_Config.c
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
#include "magtag_epaper_hal.h"
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/**
 * There's no way around this Zephyr-specific log register. This ifdef statement
 * tests for the following symbol: CONFIG_KERNEL_BIN_NAME="zephyr"
 **/
#ifdef CONFIG_KERNEL_BIN_NAME
	LOG_MODULE_REGISTER(epaper_driver_dev, LOG_LEVEL_DBG);
#endif

/*
 * Get busy pin configuration from the devicetree
 */
#define BUSY_NODE	DT_ALIAS(busy)
static const struct gpio_dt_spec busy = GPIO_DT_SPEC_GET_OR(BUSY_NODE, gpios,{0});


/*
 * Get mosi, sclk, csel, dc and rst pins from devicetree
 */
static struct gpio_dt_spec mosi = GPIO_DT_SPEC_GET_OR(DT_ALIAS(mosi), gpios, {0});
static struct gpio_dt_spec sclk = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sclk), gpios, {0});
static struct gpio_dt_spec csel = GPIO_DT_SPEC_GET_OR(DT_ALIAS(csel), gpios, {0});
static struct gpio_dt_spec dc = GPIO_DT_SPEC_GET_OR(DT_ALIAS(dc), gpios, {0});
static struct gpio_dt_spec rst = GPIO_DT_SPEC_GET_OR(DT_ALIAS(rst), gpios, {0});

void DEV_Digital_Write(uint8_t pin, uint8_t value)
{
    switch(pin) {
        case EPD_CS_PIN:
            gpio_pin_set_dt(&csel, value == 0? LOW:HIGH);
            break;
        case EPD_RST_PIN:
            gpio_pin_set_dt(&rst, value == 0? LOW:HIGH);
            break;
        case EPD_DC_PIN:
            gpio_pin_set_dt(&dc, value == 0? LOW:HIGH);
            break;
    }
}

uint8_t DEV_Digital_Read(uint8_t pin)
{
    return gpio_pin_get_dt(&busy);
}

void GPIO_Config(void)
{

    int ret;

	ret = gpio_pin_configure_dt(&busy, GPIO_INPUT);
	if (ret != 0) {
		EPAPER_LOG_ERR("Error %d: failed to configure %s pin %d",
		       ret, busy.port->name, busy.pin);
		return;
	} else {
		EPAPER_LOG_INF("Set up %s pin %d as busy", busy.port->name, busy.pin);
	}

	ret = gpio_pin_configure_dt(&dc, GPIO_OUTPUT);
	if (ret != 0) {
		EPAPER_LOG_ERR("Error %d: failed to configure %s pin %d",
				ret, dc.port->name, dc.pin);
		dc.port = NULL;
	} else {
		EPAPER_LOG_INF("Set up %s pin %d as dc", dc.port->name, dc.pin);
	}
	ret = gpio_pin_configure_dt(&rst, GPIO_OUTPUT);
	if (ret != 0) {
		EPAPER_LOG_ERR("Error %d: failed to configure %s pin %d",
				ret, rst.port->name, rst.pin);
		rst.port = NULL;
	} else {
		EPAPER_LOG_INF("Set up %s pin %d as rst", rst.port->name, rst.pin);
	}

   
    ret = gpio_pin_configure_dt(&mosi, GPIO_OUTPUT);
	if (ret != 0) {
		EPAPER_LOG_ERR("Error %d: failed to configure %s pin %d",
				ret, mosi.port->name, mosi.pin);
	} else {
		EPAPER_LOG_INF("Set up %s pin %d as mosi", mosi.port->name, mosi.pin);
	}
	ret = gpio_pin_configure_dt(&sclk, GPIO_OUTPUT);
	if (ret != 0) {
		EPAPER_LOG_ERR("Error %d: failed to configure %s pin %d",
				ret, sclk.port->name, sclk.pin);
	} else {
		EPAPER_LOG_INF("Set up %s pin %d as sclk", sclk.port->name, sclk.pin);
	}
    ret = gpio_pin_configure_dt(&csel, GPIO_OUTPUT);
	if (ret != 0) {
		EPAPER_LOG_ERR("Error %d: failed to configure %s pin %d",
				ret, csel.port->name, csel.pin);
	} else {
		EPAPER_LOG_INF("Set up %s pin %d as csel", csel.port->name, csel.pin);
	}

    gpio_pin_set_dt(&csel, HIGH);
    gpio_pin_set_dt(&sclk, LOW);

}
/******************************************************************************
function:	Module Initialize, the BCM2835 library and initialize the pins, SPI protocol
parameter:
Info:
******************************************************************************/
UBYTE DEV_Module_Init(void)
{
	//gpio
	GPIO_Config();

	return 0;
}

/******************************************************************************
function:
			SPI read and write
******************************************************************************/
void DEV_SPI_WriteByte(UBYTE data)
{
    for (int i = 0; i < 8; i++)
    {
        if ((data & 0x80) == 0) gpio_pin_set_dt(&mosi, GPIO_PIN_RESET); 
        else                    gpio_pin_set_dt(&mosi, GPIO_PIN_SET);

        data <<= 1;
        gpio_pin_set_dt(&sclk, GPIO_PIN_SET);     
        gpio_pin_set_dt(&sclk, GPIO_PIN_RESET);
    }
}
