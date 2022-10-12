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
#include "magtag-common/magtag_epaper.h"
#include "magtag-common/ws2812_control.h"

/* Golioth platform includes */
#include <net/golioth/system_client.h>
#include <net/golioth/rpc.h>
#include <samples/common/net_connect.h>
#include <qcbor/qcbor.h>
#include <qcbor/qcbor_spiffy_decode.h>
#include <zephyr/net/coap.h>

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();
static K_SEM_DEFINE(connected, 0, 1);
static struct coap_reply coap_replies[1];
#define LEDS_ENDPOINT		"leds"
#define LEDS_DEFAULT_MASK	15
uint8_t led_bitmask;

#define LINE_ARRAY_SIZE		16
#define LINE_STRING_LEN		28	/* must be a multiple of 4!! */
K_MSGQ_DEFINE(line_msgq, LINE_STRING_LEN, LINE_ARRAY_SIZE, 4);

void write_screen_from_buffer_work_handler(struct k_work *work) {
	char str[LINE_STRING_LEN];
	while(k_msgq_num_used_get(&line_msgq)) {
		int err = k_msgq_get(&line_msgq, &str, K_NO_WAIT);
		if (err) {
			LOG_ERR("Error reading from message queue: %d", err);
		}
		else {
			epaper_autowrite(str, strlen(str));
		}
	}
}
K_WORK_DEFINE(write_screen_from_buffer_work, write_screen_from_buffer_work_handler);

void led_work_handler(struct k_work *work) {
	/* Set new LED on/off values based on led_bitmask */
	leds_immediate(
		(led_bitmask&8)?RED:BLACK,
		(led_bitmask&4)?GREEN:BLACK,
		(led_bitmask&2)?BLUE:BLACK,
		(led_bitmask&1)?RED:BLACK
	);
}
K_WORK_DEFINE(led_work, led_work_handler);

void write_to_screen(char *str, uint8_t len) {
	if (k_msgq_put(&line_msgq, str, K_NO_WAIT) != 0) {
		LOG_ERR("Message buffer is full, skipping epaper write");
	}
	else {
		k_work_submit(&write_screen_from_buffer_work);
	}
}

static int lightdb_set_handler(struct golioth_req_rsp *rsp) {
	if (rsp->err) {
		LOG_WRN("Failed to set LightDB: %d", rsp->err);
		return rsp->err;
	}
	return 0;
}

/*
 * This function is registed to be called when the data
 * stored at `/leds` changes.
 */
static int observe_handler(struct golioth_req_rsp *rsp) {
	int err;
	uint8_t payload_len = rsp->len;
	char str[16] = {0};

	if (rsp->err) {
		LOG_ERR("Failed to receive observed data: %d", rsp->err);
		return rsp->err;
	}

	if (payload_len + 1 > ARRAY_SIZE(str)) {
		payload_len = ARRAY_SIZE(str) - 1;
	}

	memcpy(str, rsp->data, payload_len);
	str[payload_len] = '\0';

	LOG_DBG("payload: %s", str);

	if (strcmp(str,"null") == 0) {
		LOG_INF("Payload is null; initializing cloud endpoint: %s = %d",
				LEDS_ENDPOINT,
				LEDS_DEFAULT_MASK);
		snprintk(str, 6, "%d", LEDS_DEFAULT_MASK);

		/* Use async set because you cannot call a synchronous set from inside
		 * of a callback */
		err = golioth_lightdb_set_cb(client,
				LEDS_ENDPOINT,
				GOLIOTH_CONTENT_FORMAT_APP_JSON,
				str, strlen(str),
				lightdb_set_handler, NULL);

		if (err) {
			LOG_WRN("Failed to create %s endpoint: %d",
					LEDS_ENDPOINT,
					err);
		}
		return 0;
	}

	/* Process the received payload */
	char *ptr;
	long ret;
	uint8_t sbuf[LINE_STRING_LEN] = {0};

	/* Convert string to a number */
	ret = strtol(str, &ptr, 10);
	if (ret < 0 || ret > 15) {
		/* Test for bounded value */
		LOG_DBG("Payload was not a number in range [0..15]");
		/* Write message to display that value is not valid */
		snprintk(sbuf, LINE_STRING_LEN, "Invalid bitmask: %s", str);
	}
	else
	{
		/* Value is valid, save it and submit worker for LED update */
		led_bitmask = (uint8_t)ret;
		k_work_submit(&led_work);

		/* Write message to ePaper display about new led_bitmask */
		snprintk(sbuf, LINE_STRING_LEN, "new led_bitmask: %d", led_bitmask);
	}
	write_to_screen(sbuf, strlen(sbuf));

	return 0;
}

/*
 * In the `main` function, this function is registed to be
 * called when the device connects to the Golioth server.
 */
static enum golioth_rpc_status on_epaper(QCBORDecodeContext *request_params_array,
										 QCBOREncodeContext *response_detail_map,
										 void *callback_arg)
{
	UsefulBufC rpc_string;
	double value;
	QCBORError qerr;

	QCBORDecode_GetTextString(request_params_array, &rpc_string);
	qerr = QCBORDecode_GetError(request_params_array);
	if (qerr != QCBOR_SUCCESS) {
		LOG_ERR("Failed to decode array items: %d (%s)", qerr, qcbor_err_to_str(qerr));
		return GOLIOTH_RPC_INVALID_ARGUMENT;
	}

	/* Write message to ePaper display about new led_bitmask */
	uint8_t sbuf[LINE_STRING_LEN];
	uint8_t cbor_len = (uint8_t)rpc_string.len+1;	/* Add room for a null terminator */
	uint8_t len = LINE_STRING_LEN >= cbor_len ? cbor_len : LINE_STRING_LEN;
	snprintk(sbuf, len, "%s", (char *)rpc_string.ptr);
	if (k_msgq_put(&line_msgq, sbuf, K_NO_WAIT) != 0) {
		LOG_ERR("Message buffer is full, skipping epaper write");
	}
	else {
		k_work_submit(&write_screen_from_buffer_work);
	}
	return GOLIOTH_RPC_OK;
}

static void golioth_on_connect(struct golioth_client *client)
{
	int err;

	k_sem_give(&connected);

	err = golioth_rpc_register(client, "epaper", on_epaper, NULL);
	if (err) {
		LOG_ERR("Failed to register RPC: %d", err);
	}

	/*
	 * Observe the data stored at `/leds` in LightDB. When that data is
	 * updated, the `observe_handler` callback will be called. This will get
	 * the value when first called, even if the value doesn't change.
	 */
	err = golioth_lightdb_observe_cb(client,
			LEDS_ENDPOINT,
			COAP_CONTENT_FORMAT_APP_JSON,
			observe_handler, NULL);

	if (err) {
		LOG_WRN("failed to observe lightdb path: %d", err);
	}
}

void main(void)
{
	LOG_DBG("Start MagTag Hello demo");

	/* Initialize MagTag hardware */
	ws2812_init();
	/* show two blue pixels to show until we connect to Golioth */
	leds_immediate(BLACK, BLUE, BLUE, BLACK);
	epaper_init();
	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLES_COMMON)) {
		net_connect();
	}

	client->on_connect = golioth_on_connect;
	golioth_system_client_start();

	/* wait until we've connected to golioth */
	k_sem_take(&connected, K_FOREVER);

	/* turn LEDs green to indicate connection */
	leds_immediate(GREEN, GREEN, GREEN, GREEN);
	write_to_screen("Connected to Golioth!", 21);
}
