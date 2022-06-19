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
#include <samples/common/wifi.h>

/* MagTag specific hardware includes */
#include "epaper/EPD_2in9d.h"
#include "ws2812/ws2812_control.h"
#include "accelerometer/accel.h"
#include "buttons/buttons.h"

volatile uint64_t debounce = 0;

/* Golioth */
static struct golioth_client *client = GOLIOTH_SYSTEM_CLIENT_GET();

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
		debounce = sys_clock_timeout_end_calc(K_MSEC(200));
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
			int8_t toggle_val = led_states[i].state > 0 ? 0 : 1;
			led_states[i].state = toggle_val;
			/* update local LED output immediately */
			ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);

			gpio_pin_set_dt(&act, 1);
			k_timer_start(&make_sound_timer, K_USEC(notes[i]), K_USEC(notes[i]));
			k_timer_start(&end_note_timer, K_MSEC(200), K_NO_WAIT);

			/* Build the endpoint with the correct LED number */
			uint8_t endpoint[32];
			snprintk(endpoint, sizeof(endpoint)-1, ".d/Button_%c", 'A'+i);
			/* convert the toggle_val digit into a string */
			char false_buf[6] = "false";
			char true_buf[5] = "true";
			char *state_p = toggle_val==0 ? false_buf : true_buf;

			/* Update the LightDB state endpoint on the Golioth Cloud */
			int err = golioth_lightdb_set(client,
				  endpoint,
				  COAP_CONTENT_FORMAT_TEXT_PLAIN,
				  state_p, strlen(state_p));
			if (err) {
				LOG_WRN("Failed to update Button_%c: %d", 'A'+i, err);
			}
		}
	}
}

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
	LOG_DBG("Start MagTag demo");

	#ifdef CONFIG_MAGTAG_NAME
	LOG_INF("Device name: %s", CONFIG_MAGTAG_NAME);
	#else
	#define CONFIG_MAGTAG_NAME "MagTag"
	#endif

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

	/* Setup pins for sound */
	gpio_pin_configure_dt(&act, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&snd, GPIO_OUTPUT_ACTIVE);
	gpio_pin_set_dt(&act, 0);

	/* Start Golioth */
	client->on_message = golioth_on_message;
	golioth_system_client_start();

	/* ePaper */
	epaper_init();
	epaper_WriteDoubleLine(CONFIG_MAGTAG_NAME, strlen(CONFIG_MAGTAG_NAME), 7);

	/* wait until we've connected to golioth */
	while (golioth_ping(client) != 0)
	{
		k_msleep(1000);
	}
	/* write successful connection message to screen */
	LOG_INF("Connected to Golioth!: %s", CONFIG_MAGTAG_NAME);
	epaper_autowrite("Connected to Golioth!", 21);
	led_states[0].color = RED; led_states[0].state = 1;
	led_states[1].color = GREEN; led_states[1].state = 1;
	led_states[2].color = BLUE; led_states[2].state = 1;
	led_states[3].color = YELLOW; led_states[3].state = 1;
	ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);

	/* write starting button values to LightDB state */
	uint8_t endpoint[32];
	for (uint8_t i=0; i<4; i++) {
		snprintk(endpoint, sizeof(endpoint)-1, ".d/Button_%c", 'A'+i);
		int err = golioth_lightdb_set(client,
				  endpoint,
				  COAP_CONTENT_FORMAT_TEXT_PLAIN,
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
