/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Logging */
#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(golioth_magtag, LOG_LEVEL_DBG);

/* MagTag specific hardware includes */
#include "epaper/EPD_2in9d.h"
#include "epaper/ImageData.h"
#include "ws2812/ws2812_control.h"

/* Golioth platform includes */
#include <net/golioth/system_client.h>
#include <samples/common/net_connect.h>
#include <zephyr/net/coap.h>

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();
static K_SEM_DEFINE(connected, 0, 1);

static void golioth_on_connect(struct golioth_client *client)
{
	k_sem_give(&connected);
}

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

	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLES_COMMON)) {
		net_connect();
	}

	client->on_connect = golioth_on_connect;
	client->on_message = golioth_on_message;
	golioth_system_client_start();

	k_sem_take(&connected, K_FOREVER);

	/* Initialize MagTag hardware */
	ws2812_init();
	/* show two blue pixels to show until we connect to Golioth */
	leds_immediate(BLACK, BLUE, BLUE, BLACK);
	epaper_init();

	/* wait until we've connected to golioth */
	while (golioth_ping(client) != 0)
	{
		k_msleep(1000);
	}
	/* turn LEDs green to indicate connection */
	leds_immediate(GREEN, GREEN, GREEN, GREEN);
	epaper_autowrite("Connected to Golioth!", 21);


	int counter = 0;
	int err;
	while (true) {
		/* Send hello message to the Golioth Cloud */
		LOG_INF("Sending hello! %d", counter);
		err = golioth_send_hello(client);
		if (err) {
			LOG_WRN("Failed to send hello!");
		}
		else
		{
			/* Write messages on epaper for user feedback */
			uint8_t sbuf[24];
			snprintk(sbuf, sizeof(sbuf) - 1, "Sending hello! %d", counter);
			epaper_autowrite(sbuf, strlen(sbuf));
		}
		++counter;
		k_sleep(K_SECONDS(5));
	}
}
