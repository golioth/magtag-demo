/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Logging */
#include <stdlib.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(golioth_magtag, LOG_LEVEL_DBG);

/* MagTag specific hardware includes */
#include "epaper/EPD_2in9d.h"
#include "epaper/ImageData.h"
#include "ws2812/ws2812_control.h"

/* Golioth platform includes */
#include <net/coap.h>
#include <net/golioth/system_client.h>
#include <samples/common/wifi.h>

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();
static struct coap_reply coap_replies[1];
uint8_t led_bitmask;
uint8_t update_leds_flag;

/*
 * This function is registed to be called when the data
 * stored at `/leds` changes.
 */
static int on_update(const struct coap_packet *response,
		     struct coap_reply *reply,
		     const struct sockaddr *from)
{
	char str[64];
	uint16_t payload_len;
	const uint8_t *payload;

	payload = coap_packet_get_payload(response, &payload_len);
	if (!payload) {
		LOG_WRN("packet did not contain data");
		return -ENOMSG;
	}

	if (payload_len + 1 > ARRAY_SIZE(str)) {
		payload_len = ARRAY_SIZE(str) - 1;
	}

	memcpy(str, payload, payload_len);
	str[payload_len] = '\0';

	LOG_DBG("payload: %s", log_strdup(str));

	/* Process the received payload */
	char *ptr;
	long ret;
	/* Convert string to a number */
	ret = strtol(str, &ptr, 10);
	if (ret < 0 || ret > 15) {
		/* Test for bounded value */
		LOG_DBG("Payload was not a number in range [0..15]");
	}
	else
	{
		/* Value is valid, save it and flag for an LED update */
		led_bitmask = (uint8_t)ret;
		++update_leds_flag;
	}

	return 0;
}

/*
 * In the `main` function, this function is registed to be
 * called when the device connects to the Golioth server.
 */
static void golioth_on_connect(struct golioth_client *client)
{
	struct coap_reply *observe_reply;
	int err;
	update_leds_flag = 0;

	coap_replies_clear(coap_replies, ARRAY_SIZE(coap_replies));

	observe_reply = coap_reply_next_unused(coap_replies,
					       ARRAY_SIZE(coap_replies));

	/*
	 * Observe the data stored at `/leds` in LightDB.
	 * When that data is updated, the `on_update` callback
	 * will be called.
	 * This will get the value when first called, even if
	 * the value doesn't change.
	 */
	err = golioth_lightdb_observe(client,
				      GOLIOTH_LIGHTDB_PATH("leds"),
				      COAP_CONTENT_FORMAT_TEXT_PLAIN,
				      observe_reply, on_update);

	if (err) {
		LOG_WRN("failed to observe lightdb path: %d", err);
	}
}

/*
 * In the `main` function, this function is registed to be
 * called when the device receives a packet from the Golioth server.
 */
static void golioth_on_message(struct golioth_client *client,
			       struct coap_packet *rx)
{
	/*
	 * In order for the observe callback to be called,
	 * we need to call this function.
	 */
	coap_response_received(rx, NULL, coap_replies,
			       ARRAY_SIZE(coap_replies));
}

void main(void)
{
	LOG_DBG("Start MagTag Hello demo");

	/* WiFi */
	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLE_WIFI)) {
		LOG_INF("Connecting to WiFi");
		wifi_connect();
	}

	client->on_connect = golioth_on_connect;
	client->on_message = golioth_on_message;
	golioth_system_client_start();


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
		if (update_leds_flag)
		{
			/* Set new LED on/off values based on led_bitmask */
			leds_immediate(
				(led_bitmask&8)?RED:BLACK,
				(led_bitmask&4)?GREEN:BLACK,
				(led_bitmask&2)?BLUE:BLACK,
				(led_bitmask&1)?RED:BLACK
			);
			update_leds_flag = 0;

			/* Write message to ePaper display about new led_bitmask */
			uint8_t sbuf[24];
			snprintk(sbuf, 24, "new led_bitmask: %d", led_bitmask);
			epaper_autowrite(sbuf, strlen(sbuf));
		}
		k_sleep(K_MSEC(200));
	}
}
