/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Golioth */
#include <logging/log.h>
LOG_MODULE_REGISTER(golioth_magtag, LOG_LEVEL_DBG);
#include <net/coap.h>
#include <net/golioth/system_client.h>
#include <net/golioth/wifi.h>

#include <stdlib.h>

/* ePaper */
#include "epaper/EPD_2in9d.h"
#include "epaper/ImageData.h"

/* ws2812 */
#include "ws2812/ws2812_control.h"

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();

/*
 * In the `main` function, this function is registed to be
 * called when the device receives a packet from the Golioth server.
 */
static void golioth_on_message(struct golioth_client *client,
			       struct coap_packet *rx)
{
	uint16_t payload_len;
	const uint8_t *payload;
	uint8_t type;

	type = coap_header_get_type(rx);
	payload = coap_packet_get_payload(rx, &payload_len);

	if (!IS_ENABLED(CONFIG_LOG_BACKEND_GOLIOTH) && payload) {
		LOG_HEXDUMP_DBG(payload, payload_len, "Payload");
	}
}

void main(void)
{
	LOG_DBG("Start MagTag Hello demo");

	/* WiFi */
	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLE_WIFI)) {
		LOG_INF("Connecting to WiFi");
		wifi_connect();
	}

	client->on_message = golioth_on_message;
	golioth_system_client_start();

	/* Init leds and set two blue pixels to show until we connect to Golioth */
	ws2812_init();
	leds_immediate(BLACK, BLUE, BLUE, BLACK);

	/* ePaper */
	epaper_init();

	while (golioth_ping(client) != 0)
	{
		/* wait until we've connected to golioth */
		k_msleep(1000);
	}
	/* turn LEDs green to indicate connection */
	leds_immediate(GREEN, GREEN, GREEN, GREEN);
	epaper_WriteDoubleLine("Connected to Golioth!", 21, 0);


	int counter = 0;
	int err;
	while (true) {
		LOG_INF("Sending hello! %d", counter);

		err = golioth_send_hello(client);
		if (err) {
			LOG_WRN("Failed to send hello!");
		}
		else if (counter < 7)
		{
			uint8_t sbuf[24];
			snprintk(sbuf, sizeof(sbuf) - 1, "Sending hello! %d", counter);
			epaper_WriteDoubleLine(sbuf, strlen(sbuf), counter+1);
		}
		++counter;
		k_sleep(K_SECONDS(5));
	}
}
