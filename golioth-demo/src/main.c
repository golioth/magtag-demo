/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Logging */
#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(golioth_magtag, LOG_LEVEL_DBG);

/* Golioth platform includes */
#include <net/golioth/system_client.h>
#include <samples/common/net_connect.h>
#include <zephyr/net/coap.h>

/* MagTag specific hardware includes */
#include "magtag-common/magtag_epaper.h"
#include "magtag-common/ws2812_control.h"
#include "magtag-common/accel.h"
#include "magtag-common/buttons.h"

#define DEBOUNCE_MS	200
volatile uint64_t debounce = 0;

#define BUTTON_ACTION_ARRAY_SIZE	16
#define BUTTON_ACTION_LEN		1
K_MSGQ_DEFINE(button_action_msgq,
		BUTTON_ACTION_LEN,
		BUTTON_ACTION_ARRAY_SIZE,
		1);

/* Golioth */
static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();
static K_SEM_DEFINE(connected, 0, 1);

static void golioth_on_connect(struct golioth_client *client)
{
	k_sem_give(&connected);
}

static int lightdb_handler(struct golioth_req_rsp *rsp)
{
	if (rsp->err) {
		LOG_WRN("Asnyc Golioth function failed: %d", rsp->err);
		return rsp->err;
	}
	return 0;
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
	int err = golioth_stream_push_cb(client, "accel",
				GOLIOTH_CONTENT_FORMAT_APP_JSON,
				str, strlen(str),
				lightdb_handler, NULL);
	if (err) {
		return err;
	}
	return 0;
}

/* Timers for sound (PWM not yet implemented in ESP32s2 */
#define ACTIVATE_NODE DT_ALIAS(activate)
#define SOUND_NODE DT_ALIAS(sound)
static const struct gpio_dt_spec act = GPIO_DT_SPEC_GET(ACTIVATE_NODE, gpios);
static const struct gpio_dt_spec snd = GPIO_DT_SPEC_GET(SOUND_NODE, gpios);

void make_sound_timer_handler(struct k_timer *dummy)
{
    gpio_pin_toggle_dt(&snd);
}

K_TIMER_DEFINE(make_sound_timer, make_sound_timer_handler, NULL);

void end_note_timer_handler(struct k_timer *dummy)
{
    k_timer_stop(&make_sound_timer);
	gpio_pin_set_dt(&snd, 0);
	gpio_pin_set_dt(&act, 0);
}

K_TIMER_DEFINE(end_note_timer, end_note_timer_handler, NULL);
uint16_t notes[4] = {764,580,470,400};

void button_action_work_handler(struct k_work *work) {
	while (k_msgq_num_used_get(&button_action_msgq)) {
		uint8_t i;
		k_msgq_get(&button_action_msgq, &i, K_NO_WAIT);
		int8_t toggle_val = led_states[i].state > 0 ? 0 : 1;
		led_states[i].state = toggle_val;
		/* update local LED output immediately */
		ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);

		gpio_pin_set_dt(&act, 1);
		k_timer_start(&make_sound_timer, K_USEC(notes[i]), K_USEC(notes[i]));
		k_timer_start(&end_note_timer, K_MSEC(200), K_NO_WAIT);

		/* Build the endpoint with the correct LED number */
		uint8_t endpoint[32];
		snprintk(endpoint, sizeof(endpoint)-1, "Button_%c", 'A'+i);
		/* convert the toggle_val digit into a string */
		char false_buf[6] = "false";
		char true_buf[5] = "true";
		char *state_p = toggle_val==0 ? false_buf : true_buf;

		/* Update the LightDB state endpoint on the Golioth Cloud */
		int err = golioth_lightdb_set_cb(client, endpoint,
				GOLIOTH_CONTENT_FORMAT_APP_JSON,
				state_p, strlen(state_p),
				lightdb_handler, NULL);
		if (err) {
			LOG_WRN("Failed to update Button_%c: %d", 'A'+i, err);
		}
	}
}

K_WORK_DEFINE(button_action_work, button_action_work_handler);

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
		debounce = sys_clock_timeout_end_calc(K_MSEC(DEBOUNCE_MS));
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
			LOG_INF("Button %c pressed", 'A'+i);
			k_msgq_put(&button_action_msgq, &i, K_NO_WAIT);
		}
	}

	k_work_submit(&button_action_work);
}

void main(void)
{
	LOG_DBG("Start MagTag demo");

	#ifdef CONFIG_MAGTAG_NAME
	LOG_INF("Device name: %s", CONFIG_MAGTAG_NAME);
	#else
	#define CONFIG_MAGTAG_NAME "MagTag"
	#endif

	/* Init leds and set two blue pixels to show until we connect to Golioth */
	ws2812_init();
	leds_immediate(BLACK, BLUE, BLUE, BLACK);

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

	/* Accelerometer */
	accelerometer_init();

	/* buttons */
	buttons_init(button_pressed);

	/* Setup pins for sound */
	gpio_pin_configure_dt(&act, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&snd, GPIO_OUTPUT_ACTIVE);
	gpio_pin_set_dt(&act, 0);

	/* ePaper */
	epaper_write(CONFIG_MAGTAG_NAME, strlen(CONFIG_MAGTAG_NAME), 14, -1, 2);

	/* write successful connection message to screen */
	LOG_INF("Connected to Golioth!: %s", CONFIG_MAGTAG_NAME);
	epaper_autowrite("Connected to Golioth!", 21);
	EPD_2IN9D_Sleep();
	
	led_states[0].color = RED; led_states[0].state = 1;
	led_states[1].color = GREEN; led_states[1].state = 1;
	led_states[2].color = BLUE; led_states[2].state = 1;
	led_states[3].color = YELLOW; led_states[3].state = 1;
	ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);

	/* write starting button values to LightDB state */
	uint8_t endpoint[32];
	for (uint8_t i=0; i<4; i++) {
		snprintk(endpoint, sizeof(endpoint)-1, "Button_%c", 'A'+i);
		int err = golioth_lightdb_set(client,
				  endpoint,
				  GOLIOTH_CONTENT_FORMAT_APP_JSON,
				  "true", 4);
		if (err) {
			LOG_WRN("Failed to update Button_%c: %d", 'A'+i, err);
		}
	}

	int err;
	while (true) {
		record_accelerometer(sensor);
		k_sleep(K_SECONDS(5));
	}
}
