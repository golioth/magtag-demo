/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Logging */
#include <stdlib.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(golioth_magtag, LOG_LEVEL_DBG);
#include <soc/soc_memory_layout.h>

#include "epaper/EPD_2in9d.h"
#include "epaper/ImageData.h"

void main(void)
{
	LOG_DBG("Start MagTag Hello demo");

	uint8_t *m1;
	uint8_t *m2;
	m1 = k_malloc(296*16);
	m2 = k_malloc(296*16);
	if (esp_ptr_external_ram(m1)) LOG_INF("External SPI successful!");

	m1[0] = 72;
	m1[1] = 27;
	LOG_INF("m1=%d, m1+1=%d",m1[0], m1[1]);

	epaper_init();

	//epaper_autowrite("Connected to Golioth!", 21);

	//memcpy(&m1, &gImage_2in9, 296*16);
	for (uint16_t i=0; i<296*16; i++) {
		m1[i] = ~gImage_2in9[i];
		if ((i%16)%2==0) m2[i] = 0xff;
		else m2[i] = 0x00;
	}
	EPD_2IN9D_SendDoubleColumn("Golioth", 8, m1, 4);
	EPD_2IN9D_Init();
	EPD_2IN9D_Clear();
	EPD_2IN9D_Display(m1);
	k_sleep(K_SECONDS(5));
	while (1)
	{
		EPD_2IN9D_DisplaySwap(m1, m2);
		k_sleep(K_SECONDS(3));
		EPD_2IN9D_DisplaySwap(m2, m1);
		k_sleep(K_SECONDS(3));
	}

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
		k_sleep(K_SECONDS(5));
	}
}
