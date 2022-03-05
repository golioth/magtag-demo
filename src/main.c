/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Logging */
#include <stdlib.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(golioth_magtag, LOG_LEVEL_DBG);

/* Golioth */
#include <net/coap.h>
#include <net/golioth/system_client.h>
#include <net/golioth/wifi.h>

/* MagTag specific hardware includes */
#include "epaper/EPD_2in9d.h"
#include "ws2812/ws2812_control.h"
#include "accelerometer/accel.h"
#include "json/json-helper.h"
#include <data/json.h>
#include "buttons/buttons.h"

volatile uint64_t debounce = 0;

/* Golioth */
static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();
static struct coap_reply coap_replies[4];

static int process_led_change(const struct coap_packet *response, uint8_t led_num)
{
	char str[160];
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

	struct atomic_led new_led_values;
	int ret = json_obj_parse(str, sizeof(str),
			     atomic_led_descr,
				 ARRAY_SIZE(atomic_led_descr),
			     &new_led_values);

	if (ret < 0)
	{
		LOG_ERR("Error parsing led_settings JSON: %d", ret);
	}
	else
	{
		LOG_DBG("JSON return code: %u", ret);
		if (ret & (1<<0 | 1<<1))
		{
			LOG_INF("LED%d Color: %s State: %d", led_num, new_led_values.color, new_led_values.state);
			set_leds(led_num, new_led_values.color, new_led_values.state);
		}
		ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);
	}

	return 0;
}

static int on_led0_update(const struct coap_packet *response,
		     struct coap_reply *reply,
		     const struct sockaddr *from)
{
	return process_led_change(response, 0);
}

static int on_led1_update(const struct coap_packet *response,
		     struct coap_reply *reply,
		     const struct sockaddr *from)
{
	return process_led_change(response, 1);
}

static int on_led2_update(const struct coap_packet *response,
		     struct coap_reply *reply,
		     const struct sockaddr *from)
{
	return process_led_change(response, 2);
}

static int on_led3_update(const struct coap_packet *response,
		     struct coap_reply *reply,
		     const struct sockaddr *from)
{
	return process_led_change(response, 3);
}

/*
 * In the `main` function, this function is registed to be
 * called when the device connects to the Golioth server.
 */
static void golioth_on_connect(struct golioth_client *client)
{
	struct coap_reply *observe_reply;
	int err;

	coap_replies_clear(coap_replies, ARRAY_SIZE(coap_replies));

	

	/*
	 * Observe the data stored at `/observed` in LightDB.
	 * When that data is updated, the `on_update` callback
	 * will be called.
	 * This will get the value when first called, even if
	 * the value doesn't change.
	 */
	observe_reply = coap_reply_next_unused(coap_replies,
					       ARRAY_SIZE(coap_replies));
	err = golioth_lightdb_observe(client,
				      GOLIOTH_LIGHTDB_PATH("led0"),
				      COAP_CONTENT_FORMAT_TEXT_PLAIN,
				      observe_reply, on_led0_update);
	if (err) {
		LOG_WRN("failed to observe lightdb path: %d", err);
	}
	observe_reply = coap_reply_next_unused(coap_replies,
					       ARRAY_SIZE(coap_replies));
	err = golioth_lightdb_observe(client,
				      GOLIOTH_LIGHTDB_PATH("led1"),
				      COAP_CONTENT_FORMAT_TEXT_PLAIN,
				      observe_reply, on_led1_update);
	if (err) {
		LOG_WRN("failed to observe lightdb path: %d", err);
	}
	observe_reply = coap_reply_next_unused(coap_replies,
					       ARRAY_SIZE(coap_replies));
	err = golioth_lightdb_observe(client,
				      GOLIOTH_LIGHTDB_PATH("led2"),
				      COAP_CONTENT_FORMAT_TEXT_PLAIN,
				      observe_reply, on_led2_update);
	if (err) {
		LOG_WRN("failed to observe lightdb path: %d", err);
	}
	observe_reply = coap_reply_next_unused(coap_replies,
					       ARRAY_SIZE(coap_replies));
	err = golioth_lightdb_observe(client,
				      GOLIOTH_LIGHTDB_PATH("led3"),
				      COAP_CONTENT_FORMAT_TEXT_PLAIN,
				      observe_reply, on_led3_update);
	if (err) {
		LOG_WRN("failed to observe lightdb path: %d", err);
	}
}

static int record_accelerometer(const struct device *sensor)
{
	struct sensor_value accel[3];
	fetch_and_display(sensor, accel);
	char str[160];
	snprintk(str, sizeof(str) - 1,
			"{\"x\":%f,\"y\":%f,\"z\":%f}",
			sensor_value_to_double(&accel[0]),
			sensor_value_to_double(&accel[1]),
			sensor_value_to_double(&accel[2])
			);
	int err = golioth_lightdb_set(client,
				GOLIOTH_LIGHTDB_STREAM_PATH("accel"),
				COAP_CONTENT_FORMAT_TEXT_PLAIN,
				str, strlen(str));
	if (err) {
		return err;
	}
	snprintk(str, sizeof(str) -1, 
			"%.4f %.4f %.4f",
			sensor_value_to_double(&accel[0]),
			sensor_value_to_double(&accel[1]),
			sensor_value_to_double(&accel[2])
			);
	epaper_autowrite(str, strlen(str));
	return 0;
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

/**
 * @brief Handle button presses and update LEDs
 *
 * This function will update the on/off state of the LED by setting turning the
 * actual LED on or off and writing the new value to LightDB state
 *
 * @param dev 	device struct
 * @param cb 	callback struct
 * @param pins 	pinmask representing buttons that are pressed
 */
void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	if (debounce > k_uptime_ticks())
	{
		/* too soon to register another button press */
		return;
	}
	else
	{
		/* register the timeout value for the next press */
		debounce = sys_clock_timeout_end_calc(K_MSEC(100));
	}

	/* Array to check which button was pressed */
	uint32_t button_result[] = {
		pins & BIT(button0.pin),
		pins & BIT(button1.pin),
		pins & BIT(button2.pin),
		pins & BIT(button3.pin)
	};

	/* Process each button press that was detected */
	for (uint8_t i=0; i<4; i++) {
		if (button_result[i] != 0)
		{
			LOG_INF("Button%d pressed.", i);
			if (led_states[i].state < 0) continue; //Indicates no cloud sync yet
			int8_t toggle_val = led_states[i].state > 0 ? 0 : 1;
			led_states[i].state = toggle_val;
			/* update local LED output immediately */
			ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);

			/* Build the endpoint with the correct LED number */
			uint8_t endpoint[32];
			snprintk(endpoint, sizeof(endpoint)-1, ".d/led%d/state",	i);
			/* convert the toggle_val digit into a string */
			char state_buf[2] = { '0'+toggle_val, 0 };

			/* Update the LightDB state endpoint on the Golioth Cloud */
			int err = golioth_lightdb_set(client,
				  endpoint,
				  COAP_CONTENT_FORMAT_TEXT_PLAIN,
				  state_buf, 1);
			if (err) {
				LOG_WRN("Failed to update led%d_state: %d", i, err);
			}
		}
	}
}

void main(void)
{
	LOG_DBG("Start MagTag demo");

	/* Accelerometer */
	accelerometer_init();

	/* WiFi */
	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLE_WIFI)) {
		LOG_INF("Connecting to WiFi");
		wifi_connect();
	}

	/* Init leds and set two blue pixels to show until we connect to Golioth */
	ws2812_init();
	leds_immediate(BLACK, BLUE, BLUE, BLACK);

	/* buttons */
	buttons_init(button_pressed);

	client->on_connect = golioth_on_connect;
	client->on_message = golioth_on_message;
	golioth_system_client_start();

	/* ePaper */
	epaper_init();

	/* wait until we've connected to golioth */
	while (golioth_ping(client) != 0)
	{
		k_msleep(1000);
	}
	/* write successful connection message to screen */
	epaper_autowrite("Connected to Golioth!", 21);

	int err;
	while (true) {
		err = record_accelerometer(sensor);
		if (err) {
			LOG_WRN("Failed to send accel data to LightDB stream: %d", err);
		}

		k_sleep(K_SECONDS(5));
	}
}
