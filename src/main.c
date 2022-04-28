/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* (Required) Create a node identifier for the project's on-board led */
  /* MagTag - red led "D13" - pin 13 - assigned in esp32s2_saola.overlay */
  /* -- The .overlay file is used for device specific info so main.c stays generic -- */
/* 3 options to create a node identifier*/
#define LED0_NODE DT_ALIAS(board_led) // from .overlay to .c file, the (-) changes to (_)
// #define LED0_NODE DT_NODELABEL(led0)
// #define LED0_NODE DT_PATH(leds, led_0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
	int ret;

	if (!device_is_ready(led.port)) {
		return;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}

	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return;
		}
		k_msleep(SLEEP_TIME_MS);
	}
}
