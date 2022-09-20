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
#include "accelerometer/accel.h"

/* Golioth platform includes */
#include <net/golioth/system_client.h>
#include <net/golioth/settings.h>
#include <samples/common/net_connect.h>
#include <zephyr/net/coap.h>

static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();
static K_SEM_DEFINE(connected, 0, 1);

static int32_t _loop_delay_s = 5;
static k_tid_t _system_thread = 0;

enum golioth_settings_status on_setting(
		const char *key,
		const struct golioth_settings_value *value)
{
	LOG_DBG("Received setting: key = %s, type = %d", key, value->type);
	if (strcmp(key, "LOOP_DELAY_S") == 0) {
		/* This setting is expected to be numeric, return an error if it's not */
		if (value->type != GOLIOTH_SETTINGS_VALUE_TYPE_INT64) {
			return GOLIOTH_SETTINGS_VALUE_FORMAT_NOT_VALID;
		}

		/* This setting must be in range [1, 100], return an error if it's not */
		if (value->i64 < 1 || value->i64 > 100) {
			return GOLIOTH_SETTINGS_VALUE_OUTSIDE_RANGE;
		}

		/* Setting has passed all checks, so apply it to the loop delay */
		_loop_delay_s = (int32_t)value->i64;
		LOG_INF("Set loop delay to %d seconds", _loop_delay_s);
		k_wakeup(_system_thread);

		return GOLIOTH_SETTINGS_SUCCESS;
	}

	/* If the setting is not recognized, we should return an error */
	return GOLIOTH_SETTINGS_KEY_NOT_RECOGNIZED;
}

static void golioth_on_connect(struct golioth_client *client)
{
	k_sem_give(&connected);

	int err = golioth_settings_register_callback(client, on_setting);

	if (err) {
		LOG_ERR("Failed to register settings callback: %d", err);
	}
}

static int record_accelerometer(const struct device *sensor)
{
	/* Struct to store new sensor reading */
	struct sensor_value accel[3];
	/* Call sensor library to take sensor reading */
	fetch_and_display(sensor, accel);

	/* Turn sensor data into a string */
	char str[160];
	snprintk(str, sizeof(str) - 1,
			"{\"x\":%f,\"y\":%f,\"z\":%f}",
			sensor_value_to_double(&accel[0]),
			sensor_value_to_double(&accel[1]),
			sensor_value_to_double(&accel[2])
			);

	/* Send string to Golioth LightDB Stream */
	int err = golioth_lightdb_set(client,
				GOLIOTH_LIGHTDB_STREAM_PATH("accel"),
				COAP_CONTENT_FORMAT_TEXT_PLAIN,
				str, strlen(str));
	if (err) {
		return err;
	}
	return 0;
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
	LOG_DBG("Start MagTag LightDB Stream demo");

	/* Get system thread id so loop delay change event can wake main */
	_system_thread = k_current_get();

	/* Initialize MagTag hardware */
	ws2812_init();
	/* show two blue pixels to show until we connect to Golioth */
	leds_immediate(BLACK, BLUE, BLUE, BLACK);
	epaper_init();
	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLES_COMMON)) {
		net_connect();
	}

	client->on_connect = golioth_on_connect;
	client->on_message = golioth_on_message;
	golioth_system_client_start();

	/* wait until we've connected to golioth */
	k_sem_take(&connected, K_FOREVER);

	/* turn LEDs green to indicate connection */
	leds_immediate(GREEN, GREEN, GREEN, GREEN);
	epaper_autowrite("Connected to Golioth!", 21);

	/* Accelerometer */
	accelerometer_init();

	int err;
	while (true) {
		/* Send accelerometer data to the Golioth Cloud */
		LOG_INF("Sending accel data");

		err = record_accelerometer(sensor);
		if (err) {
			LOG_WRN("Failed to accel data to LightDB stream: %d", err);
		}
		else
		{
			epaper_autowrite("Sent accel data", 16);
		}

		k_sleep(K_SECONDS(_loop_delay_s));
	}
}
