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
#include <drivers/sensor.h>

/* ePaper */
#include "epaper/DEV_Config.h"
#include "epaper/EPD_2in9d.h"
#include "epaper/ImageData.h"
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

/* Buttons */
#define SW0_NODE	DT_ALIAS(sw0)
#define SW1_NODE	DT_ALIAS(sw1)
#define SW2_NODE	DT_ALIAS(sw2)
#define SW3_NODE	DT_ALIAS(sw3)
static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(SW1_NODE, gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(SW2_NODE, gpios);
static const struct gpio_dt_spec button3 = GPIO_DT_SPEC_GET(SW3_NODE, gpios);
static struct gpio_callback button_cb_data;

/* Golioth */
static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();
static struct coap_reply coap_replies[1];

/* JSON parsing structs */
struct led_settings {
	const char *led0_color;
	int8_t led0_state;
	const char *led1_color;
	int8_t led1_state;
	const char *led2_color;
	int8_t led2_state;
	const char *led3_color;
	int8_t led3_state;
};

static const struct json_obj_descr led_settings_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct led_settings, led0_color, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct led_settings, led0_state, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct led_settings, led1_color, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct led_settings, led1_state, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct led_settings, led2_color, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct led_settings, led2_state, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct led_settings, led3_color, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct led_settings, led3_state, JSON_TOK_NUMBER)
};

struct led_settings led_received_changes;
uint8_t leds_need_update_flag;

/* prototypes */
static void set_leds(uint8_t led_num, struct led_settings *ls);

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
			set_leds(0, &led_received_changes);
		}
		if (ret & 1<<2)
		{
			LOG_INF("LED1 Color: %s State: %d", led_received_changes.led1_color, led_states[1].state);
			set_leds(1, &led_received_changes);
		}
		if (ret & 1<<4)
		{
			LOG_INF("LED2 Color: %s State: %d", led_received_changes.led2_color, led_states[2].state);
			set_leds(2, &led_received_changes);
		}
		if (ret & 1<<6)
		{
			LOG_INF("LED3 Color: %s State: %d", led_received_changes.led3_color, led_states[3].state);
			set_leds(3, &led_received_changes);
		}
		++leds_need_update_flag;
	}

	return 0;
}

static uint16_t get_fasthash(const char *word)
{
	uint16_t sum = 0;
	for (uint8_t i=0; i<strlen(word); i++) {
		sum ^= (uint16_t) word[i];
	}
	return sum;
}

static void set_leds(uint8_t led_num, struct led_settings *ls)
{
	int8_t l_state; const char * l_color;
	switch(led_num)
	{
		//FIXME: led_states needs to be passed by reference to this function
		case(0): l_color = ls->led0_color; l_state = ls->led0_state; led_states[0].state = ls->led0_state; break;
		case(1): l_color = ls->led1_color; l_state = ls->led1_state; led_states[1].state = ls->led1_state;break;
		case(2): l_color = ls->led2_color; l_state = ls->led2_state; led_states[2].state = ls->led2_state;break;
		case(3): l_color = ls->led3_color; l_state = ls->led3_state; led_states[3].state = ls->led3_state;break;
	}
	switch(get_fasthash(l_color))
	{
		case (111):
			LOG_INF("LED #%d is off!!!", led_num);
			//set_pixel(led_num, colors[0], l_state);
			led_states[led_num].color = 0;
			break;
		case (115):
			LOG_INF("LED #%d is red!!!", led_num);
			//set_pixel(led_num, colors[1], l_state);
			led_states[led_num].color = 1;
			break;
		case (123):
			LOG_INF("LED #%d is green!!!", led_num);
			//set_pixel(led_num, colors[2], l_state);
			led_states[led_num].color = 2;
			break;
		case (30):
			LOG_INF("LED #%d is blue!!!", led_num);
			//set_pixel(led_num, colors[3], l_state);
			led_states[led_num].color = 3;
			break;
	}
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
				      GOLIOTH_LIGHTDB_PATH("led_settings"),
				      COAP_CONTENT_FORMAT_TEXT_PLAIN,
				      observe_reply, on_update);

	if (err) {
		LOG_WRN("failed to observe lightdb path: %d", err);
	}
}

static void fetch_and_display(const struct device *sensor)
{
	static unsigned int count;
	struct sensor_value accel[3];
	struct sensor_value temperature;
	const char *overrun = "";
	int rc = sensor_sample_fetch(sensor);

	++count;
	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_LIS2DH_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor,
					SENSOR_CHAN_ACCEL_XYZ,
					accel);
	}
	if (rc < 0) {
		LOG_ERR("ERROR: Update failed: %d", rc);
	} else {
		LOG_INF("#%u @ %u ms: %sx %f , y %f , z %f",
		       count, k_uptime_get_32(), overrun,
		       sensor_value_to_double(&accel[0]),
		       sensor_value_to_double(&accel[1]),
		       sensor_value_to_double(&accel[2]));

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

	if (IS_ENABLED(CONFIG_LIS2DH_MEASURE_TEMPERATURE)) {
		if (rc == 0) {
			rc = sensor_channel_get(sensor, SENSOR_CHAN_DIE_TEMP, &temperature);
			if (rc < 0) {
				LOG_ERR("ERROR: Unable to read temperature:%d", rc);
			} else {
				LOG_INF(", t %f", sensor_value_to_double(&temperature));
			}
		}
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

void button_pressed(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	if (pins & BIT(button0.pin)) {
		if (led_states[0].state < 0) return;
		int8_t toggle_val = led_states[0].state > 0 ? 0 : 1;
		led_states[0].state = toggle_val;
		++leds_need_update_flag;
		char buf[2] = { '0'+toggle_val, 0 };
		int err = golioth_lightdb_set(client,
				  GOLIOTH_LIGHTDB_PATH("led_settings/led0_state"),
				  COAP_CONTENT_FORMAT_TEXT_PLAIN,
				  buf, 1);
		if (err) {
			LOG_WRN("Failed to update led0_state: %d", err);
		}
	}
	if (pins & BIT(button1.pin)) {
		if (led_states[1].state < 0) return;
		int8_t toggle_val = led_states[1].state > 0 ? 0 : 1;
		led_states[1].state = toggle_val;
		++leds_need_update_flag;
		char buf[2] = { '0'+toggle_val, 0 };
		int err = golioth_lightdb_set(client,
				  GOLIOTH_LIGHTDB_PATH("led_settings/led1_state"),
				  COAP_CONTENT_FORMAT_TEXT_PLAIN,
				  buf, 1);
		if (err) {
			LOG_WRN("Failed to update led1_state: %d", err);
		}
	}
	if (pins & BIT(button2.pin)) {
		if (led_states[2].state < 0) return;
		int8_t toggle_val = led_states[2].state > 0 ? 0 : 1;
		led_states[2].state = toggle_val;
		++leds_need_update_flag;
		char buf[2] = { '0'+toggle_val, 0 };
		int err = golioth_lightdb_set(client,
				  GOLIOTH_LIGHTDB_PATH("led_settings/led2_state"),
				  COAP_CONTENT_FORMAT_TEXT_PLAIN,
				  buf, 1);
		if (err) {
			LOG_WRN("Failed to update led2_state: %d", err);
		}
	}
	if (pins & BIT(button3.pin)) {
		if (led_states[3].state < 0) return;
		int8_t toggle_val = led_states[3].state > 0 ? 0 : 1;
		led_states[3].state = toggle_val;
		++leds_need_update_flag;
		char buf[2] = { '0'+toggle_val, 0 };
		int err = golioth_lightdb_set(client,
				  GOLIOTH_LIGHTDB_PATH("led_settings/led3_state"),
				  COAP_CONTENT_FORMAT_TEXT_PLAIN,
				  buf, 1);
		if (err) {
			LOG_WRN("Failed to update led3_state: %d", err);
		}
	}
}



void main(void)
{
	LOG_DBG("Start MagTag demo");

	leds_need_update_flag = 0;

	/* Accelerometer */
	const struct device *sensor = DEVICE_DT_GET_ANY(st_lis2dh);
	uint8_t lis3dh_loopcount = 0;
	if (sensor == NULL) {
		LOG_ERR("No device found");
		return;
	}
	if (!device_is_ready(sensor)) {
		LOG_ERR("Device %s is not ready", sensor->name);
		return;
	}

	/* WiFi */
	if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLE_WIFI)) {
		LOG_INF("Connecting to WiFi");
		wifi_connect();
	}

	client->on_connect = golioth_on_connect;
	client->on_message = golioth_on_message;
	golioth_system_client_start();

	ws2812_init();

	clear_pixels();
	set_pixel(1, colors[3], 1);
	led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);

	/* buttons */
	gpio_pin_configure_dt(&button0, GPIO_INPUT);
	gpio_pin_configure_dt(&button1, GPIO_INPUT);
	gpio_pin_configure_dt(&button2, GPIO_INPUT);
	gpio_pin_configure_dt(&button3, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button0, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&button2, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&button3, GPIO_INT_EDGE_TO_ACTIVE);
	uint32_t button_mask = BIT(button0.pin) | BIT(button1.pin) | BIT(button2.pin) | BIT(button3.pin);
	gpio_init_callback(&button_cb_data, button_pressed, button_mask);
	gpio_add_callback(button0.port, &button_cb_data);
	gpio_add_callback(button1.port, &button_cb_data);
	gpio_add_callback(button2.port, &button_cb_data);
	gpio_add_callback(button3.port, &button_cb_data);

	/* ePaper */
	LOG_INF("Setup ePaper pins");
  	DEV_Module_Init();

    LOG_INF("ePaper Init and Clear");
    EPD_2IN9D_Init();
    EPD_2IN9D_Clear();
	k_sleep(K_MSEC(500));

	LOG_INF("Show Golioth logo");
	EPD_2IN9D_Display((void *)gImage_2in9); /* cast because function is not expecting a CONST array) */
    EPD_2IN9D_Sleep(); /* always sleep the ePaper display to avoid damaging it */

	uint8_t epaper_partial_demo_loopcount = 0;
	uint8_t epaper_partial_demo_linecount = 0;

	while (true) {
		if (++lis3dh_loopcount >= 50) {
			lis3dh_loopcount = 0;
			fetch_and_display(sensor);
		}

		if (leds_need_update_flag) {
			ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);
			leds_need_update_flag = 0;
		}

		if (++epaper_partial_demo_loopcount >= 5 && epaper_partial_demo_linecount < 8)
		{
			EPD_2IN9D_Init();
			EPD_2IN9D_LinePart(
				(void *)str_p[epaper_partial_demo_linecount],
				strlen(str_p[epaper_partial_demo_linecount]),
				epaper_partial_demo_linecount,
				0,
				296
				);
			EPD_2IN9D_Sleep();
			++epaper_partial_demo_linecount;
		}

		k_sleep(K_MSEC(200));
	}
}
