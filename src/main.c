/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Logging */
#include <stdlib.h>
#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(magtag_newepaper, LOG_LEVEL_DBG);

/* MagTag specific hardware includes */
#include "epaper/magtag_epaper.h"
#include "epaper/ImageData.h"

void main(void)
{
	LOG_DBG("Start MagTag Hello demo");

	/* show two blue pixels to show until we connect to Golioth */
	epaper_init();
	/* turn LEDs green to indicate connection */
	epaper_autowrite("Connected to Golioth!", 21);
	epaper_WriteLine("Line 2",6,2);
	epaper_WriteLine("Line 3",6,3);
	epaper_WriteLine("Line 4",6,4);
	epaper_WriteLine("Line 5",6,5);
	epaper_WriteLine("Line 6",6,6);
	epaper_WriteLine("Line 16",7,16);

	int counter = 0;
	int err;
	while (true) {
		/* Send hello message to the Golioth Cloud */
		LOG_INF("Sending hello! %d", counter);
		/* Write messages on epaper for user feedback */
		uint8_t sbuf[24];
		snprintk(sbuf, sizeof(sbuf) - 1, "Sending hello! %d", counter);
		epaper_autowrite(sbuf, strlen(sbuf));
		++counter;
// 		k_sleep(K_SECONDS(5));
	}
}
