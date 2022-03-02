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
/* json */
#include <data/json.h>

/* Accelerometer */
#include "accelerometer/accel.h"

/* ePaper */
#include "epaper/EPD_2in9d.h"
const uint8_t demostr0[] = "I must not fear.";
const uint8_t demostr1[] = "Fear is the mind-killer.";
const uint8_t demostr2[] = "Fear is the little-death that brings total obliteration.";
const uint8_t demostr3[] = "I will face my fear.";
const uint8_t demostr4[] = "I will permit it to pass over me and through me.";
const uint8_t demostr5[] = "And when it has gone past I will turn the inner eye to see its path.";
const uint8_t demostr6[] = "Where the fear has gone there will be nothing.";
const uint8_t demostr7[] = "Only I will remain.";
const uint8_t *str_p[] = {demostr0, demostr1, demostr2, demostr3, demostr4, demostr5, demostr6, demostr7 };

/* ws2812 */
#include "ws2812/ws2812_control.h"

/* buttons*/
#include "buttons/buttons.h"
volatile uint64_t debounce = 0;

/* Golioth */
static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();
static struct coap_reply coap_replies[1];

/* JSON parsing structs */
#include "json/json-helper.h"

struct led_settings led_received_changes;
uint8_t leds_need_update_flag;

/*
 * This function is registed to be called when the data
 * stored at `/observed` changes.
 */
static int on_update(const struct coap_packet *response,
		     struct coap_reply *reply,
		     const struct sockaddr *from)
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

	int ret = json_obj_parse(str, sizeof(str),
			     led_settings_descr,
				 ARRAY_SIZE(led_settings_descr),
			     &led_received_changes);

	if (ret < 0)
	{
		LOG_ERR("Error parsing led_settings JSON: %d", ret);
	}
	else
	{
		LOG_DBG("JSON return code: %u", ret);
		if (ret & 1<<0)
		{
			LOG_INF("LED0 Color: %s State: %d", led_received_changes.led0_color, led_received_changes.led0_state);
			set_leds(0, led_received_changes.led0_color, led_received_changes.led0_state);
		}
		if (ret & 1<<2)
		{
			LOG_INF("LED1 Color: %s State: %d", led_received_changes.led1_color, led_received_changes.led1_state);
			set_leds(1, led_received_changes.led1_color, led_received_changes.led1_state);
		}
		if (ret & 1<<4)
		{
			LOG_INF("LED2 Color: %s State: %d", led_received_changes.led2_color, led_received_changes.led2_state);
			set_leds(2, led_received_changes.led2_color, led_received_changes.led2_state);
		}
		if (ret & 1<<6)
		{
			LOG_INF("LED3 Color: %s State: %d", led_received_changes.led3_color, led_received_changes.led3_state);
			set_leds(3, led_received_changes.led3_color, led_received_changes.led3_state);
		}
		++leds_need_update_flag;
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

	coap_replies_clear(coap_replies, ARRAY_SIZE(coap_replies));

	observe_reply = coap_reply_next_unused(coap_replies,
					       ARRAY_SIZE(coap_replies));

	/*
	 * Observe the data stored at `/observed` in LightDB.
	 * When that data is updated, the `on_update` callback
	 * will be called.
	 * This will get the value when first called, even if
	 * the value doesn't change.
	 */
	err = golioth_lightdb_observe(client,
				      GOLIOTH_LIGHTDB_PATH("magtag"),
				      COAP_CONTENT_FORMAT_TEXT_PLAIN,
				      observe_reply, on_update);

	if (err) {
		LOG_WRN("failed to observe lightdb path: %d", err);
	}
}


static void record_accelerometer(const struct device *sensor)
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
		LOG_WRN("Failed to update led0_state: %d", err);
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
			++leds_need_update_flag;

			/* Build the endpoint with the correct LED number */
			uint8_t endpoint[32];
			snprintk(endpoint, sizeof(endpoint)-1, ".d/magtag/led%d_state",	i);
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

	leds_need_update_flag = 0;

	/* Accelerometer */
	accelerometer_init();
	uint8_t lis3dh_loopcount = 0;

	/* WiFi */
	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLE_WIFI)) {
		LOG_INF("Connecting to WiFi");
		wifi_connect();
	}

	client->on_connect = golioth_on_connect;
	client->on_message = golioth_on_message;
	golioth_system_client_start();

	/* Init leds and set two blue pixels to show until we connect to Golioth */
	ws2812_init();
	leds_immediate(BLACK, BLUE, BLUE, BLACK);

	/* buttons */
	buttons_init(button_pressed);

	/* ePaper */
	epaper_init();

	uint8_t epaper_partial_demo_loopcount = 0;
	uint8_t epaper_partial_demo_linecount = 0;

	while (true) {
		if (++lis3dh_loopcount >= 50) {
			lis3dh_loopcount = 0;
			record_accelerometer(sensor);
		}

		if (leds_need_update_flag) {
			ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);
			leds_need_update_flag = 0;
		}

		if (++epaper_partial_demo_loopcount >= 5 && epaper_partial_demo_linecount < 8)
		{
			epaper_autowrite(
				(void *)str_p[epaper_partial_demo_linecount],
				strlen(str_p[epaper_partial_demo_linecount])
				);
			++epaper_partial_demo_linecount;
		}

		k_sleep(K_MSEC(200));
	}
}
