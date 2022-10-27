/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MYNAME	"John Hackworth"
#define TITLE	"Nanotech Engineer"
#define HANDLE	"@Kurt_Vonnegut"
#define DEFAULT_FRAME	3

#define NAME_SIZE 64
char _myname[NAME_SIZE] = MYNAME;
char _title[NAME_SIZE] = TITLE;
char _handle[NAME_SIZE] = HANDLE;

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
#include "magtag-common/buttons.h"

/* Images for the epaper screen */
#include "../golioth_nametag.h"
#include "../frame0.h"
#include "../frame1.h"
#include "../frame2.h"
#include "../frame3.h"

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
static K_SEM_DEFINE(user_update_choice, 0, 1);
static K_SEM_DEFINE(wifi_control, 0, 1);
struct k_mutex epaper_mutex;

static void golioth_on_connect(struct golioth_client *client)
{
	k_sem_give(&connected);
}

int fetch_name_from_golioth(uint8_t idx) {
	int err = 0;
	static uint8_t row_idx = 1;

	uint8_t *dest_ptr;
	char endpoint[16];
	if (idx == 0) { dest_ptr = _myname; strcpy(endpoint, "name"); }
	else if (idx == 1) { dest_ptr = _title; strcpy(endpoint, "title"); }
	else if (idx == 2) { dest_ptr = _handle; strcpy(endpoint, "handle"); }
	else { return -1; }

	if (!golioth_is_connected(client)) {
		err = -ENETDOWN;
	}
	else {
		LOG_INF("Fetching %s information from Golioth LightDB State", endpoint);
		uint8_t name_update[NAME_SIZE];
		size_t len = NAME_SIZE;
		int err = golioth_lightdb_get(client,
					endpoint,
					GOLIOTH_CONTENT_FORMAT_APP_JSON,
					name_update,
					&len);

		if (err) {
			LOG_ERR("Unable to fetch name information from Golioth: %d", err);
			epaper_Write("Unable to fetch", 15, (row_idx++)*2, FULL_WIDTH, 2);
			return err;
		}

		if (strncmp(name_update, "null", 4)==0) {
			epaper_Write("Endpoint missing on Golioth", 27, (row_idx++)*2, FULL_WIDTH, 2);
			LOG_INF("Endpoint doesn't exist: %s", endpoint);
		}
		else {
			/* hack to remove quotes received with JSON string */
			name_update[len-1] = '\0';
			memcpy(dest_ptr, name_update+1, NAME_SIZE);
			epaper_Write("Success!", 8, (row_idx++)*2, FULL_WIDTH, 2);
			LOG_INF("Received new name: %s", dest_ptr);
		}

	}

// 	golioth_system_client_stop();
	return err;
}

enum nametag_colors{
	ALLRED,
	ALLGREEN,
	ALLBLUE,
	ALLYELLOW,
	RAINBOW
};

void led_color_changer(enum nametag_colors n_color) {
	if (n_color == RAINBOW) {
		led_states[0].color = RED;
		led_states[1].color = GREEN;
		led_states[2].color = BLUE;
		led_states[3].color = YELLOW;
		ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);
		return;
	}

	uint8_t preset;
	switch(n_color) {
		case ALLRED:
			preset = RED;
			break;
		case ALLGREEN:
			preset = GREEN;
			break;
		case ALLBLUE:
			preset = BLUE;
			break;
		case ALLYELLOW:
			preset = YELLOW;
			break;
		default:
			return;
	}

	for (uint8_t i=0; i<4; i++) {
		led_states[i].color = preset;
	}
	ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);
}

void nametag_blue(void) {
	if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))!=0) {
		/* ePaper already in use, abort */
		return;
	}

	led_color_changer(ALLBLUE);
	epaper_FullClear();
	epaper_ShowFullFrame(frame2);
	epaper_WriteInverted("HELLO", 5, 2, CENTER, 2);
	epaper_WriteInverted("my name is", 10, 4, CENTER, 1);
	epaper_Write(_myname, strlen(_myname), 8, CENTER, 4);

	k_mutex_unlock(&epaper_mutex);
}

void nametag_green(void) {
	if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))!=0) {
		/* ePaper already in use, abort */
		return;
	}

	led_color_changer(ALLGREEN);

	epaper_FullClear();
	epaper_ShowFullFrame(frame1);

	char firstname[20] = " ";
	char lastname[20] = " ";
	char *ptr = firstname;

	uint8_t iter = 0;
	uint8_t idx = 0;
	while(iter<40) {
		if (idx == 19) {
			*(ptr+iter) = '\0';
			iter = 254;
		}
		else if (iter >= strlen(_myname)) {
			*(ptr+idx) = '\0';
			iter = 254;
		}
		else if (_myname[iter] == ' ') {
			*(ptr+idx) = '\0';
			if (ptr == firstname) {
				ptr=lastname;
				idx = 0;
			}
			else {
				iter = 254;
			}
		}
		else {
			*(ptr+idx) = _myname[iter];
			++idx;
		}
		++iter;
	}
	epaper_Write(firstname, strlen(firstname), 5, 216, 4);
	epaper_Write(lastname, strlen(lastname), 10, 216, 4);

	k_mutex_unlock(&epaper_mutex);
}

void nametag_red(void) {
	if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))!=0) {
		/* ePaper already in use, abort */
		return;
	}

	led_color_changer(ALLRED);

	epaper_FullClear();
	epaper_ShowFullFrame(frame0);
	epaper_Write(_title, strlen(_title), 1, 284, 2);
	epaper_Write(_myname, strlen(_myname), 6, CENTER, 4);
	epaper_Write(_handle, strlen(_handle), 13, 204, 2);

	k_mutex_unlock(&epaper_mutex);
}

/**
 * @brief Unused function awaiting workshop user customization
 */
void nametag_training_challenge(void) {
	if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))!=0) {
		/* ePaper already in use, abort */
		return;
	}

	/* Perform a full-refresh on the display */
	epaper_FullClear();

	/* Use a partial write to draw the background */
	/* Change frame3 to the name of your array */
	epaper_ShowFullFrame(frame3);

	/* Write text on top of the background */
	epaper_Write(_myname, strlen(_myname), 2, CENTER, 4);
	epaper_WriteInverted(_title, strlen(_title), 11, CENTER, 2);
	epaper_WriteInverted(_handle, strlen(_handle), 13, CENTER, 2);

	k_mutex_unlock(&epaper_mutex);
}

void nametag_yellow(void) {
	if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))!=0) {
		/* ePaper already in use, abort */
		return;
	}

	led_color_changer(ALLYELLOW);
	epaper_FullClear();
	epaper_ShowFullFrame(frame3);
	epaper_Write(_myname, strlen(_myname), 2, CENTER, 4);
	epaper_WriteInverted(_title, strlen(_title), 11, CENTER, 2);
	epaper_WriteInverted(_handle, strlen(_handle), 13, CENTER, 2);

	k_mutex_unlock(&epaper_mutex);
}

void nametag_rainbow(void) {
	if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))!=0) {
		/* ePaper already in use, abort */
		return;
	}

	LOG_INF("Fetching data");

	led_color_changer(RAINBOW);

	epaper_FullClear();
	epaper_Write("Fetching name from Golioth", 26, 0, FULL_WIDTH, 2);

	int err;
	for (uint8_t i=0; i<3; i++) {
		err = fetch_name_from_golioth(i);
		if (err != 0) {
			if (err == -ENETDOWN) {
				epaper_Write("Err: Not connected to Golioth", 29, 14, FULL_WIDTH, 2);
			}
			else {
				epaper_Write("Unknown error", 13, 14, FULL_WIDTH, 2);
			}
			k_mutex_unlock(&epaper_mutex);
			return;
		}
	}
	k_mutex_unlock(&epaper_mutex);
	nametag_red();
}

void button_action_work_handler(struct k_work *work) {
	while (k_msgq_num_used_get(&button_action_msgq)) {
		uint8_t i;
		k_msgq_get(&button_action_msgq, &i, K_NO_WAIT);

		if (k_sem_count_get(&user_update_choice) == 0) {
			/* Device await user response at boot time */
			if (i==0) {
				LOG_INF("User chose: No");
				k_sem_give(&user_update_choice);
			}
			else if (i==1) {
				/* User said yes to WiFi update */
				LOG_INF("User chose: Yes");
				led_color_changer(RAINBOW);
				if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))==0) {
					epaper_Write("Starting WiFi...", 16, 0, 160, 2);
					k_mutex_unlock(&epaper_mutex);
				}
				k_sem_give(&wifi_control);
				k_sem_give(&user_update_choice);
			}
			else {
				LOG_INF("Invalid user choice: %d", i);
			}
			return;
		}

		/* Update the ePaper frame */
		if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))==0) {
			switch(i) {
				case 1:
					nametag_green();
					break;
				case 2:
					nametag_blue();
					break;
				case 3:
					nametag_yellow();
					break;
				case 4:
					nametag_rainbow();
					break;
				default:
					nametag_red();
			}
			k_mutex_unlock(&epaper_mutex);
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
	else if (epaper_mutex.lock_count > 0) {
		/* ePaper write is in progress, disable button presses */
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

	k_mutex_init(&epaper_mutex);

	ws2812_init();
	led_states[0].color = BLUE; led_states[0].state = 1;
	led_states[1].color = BLUE; led_states[1].state = 1;
	led_states[2].color = BLACK; led_states[2].state = 1;
	led_states[3].color = BLACK; led_states[3].state = 1;
	ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);

	/* buttons */
	buttons_init(button_pressed);

	epaper_hardware_init();
	epaper_FullClear();
	epaper_ShowFullFrame(golioth_nametag);
	uint8_t default_screen = (DEFAULT_FRAME < 4 ? DEFAULT_FRAME : 0);

	LOG_INF("Awaiting user choice...");
	k_sem_take(&user_update_choice, K_FOREVER);
	k_sem_give(&user_update_choice); /* Don't block elsewhere */

	if (k_sem_count_get(&wifi_control)) {
		if (IS_ENABLED(CONFIG_GOLIOTH_SAMPLES_COMMON)) {
			net_connect();
		}

		client->on_connect = golioth_on_connect;
		golioth_system_client_start();

		/* wait until we've connected to golioth */
		k_sem_take(&connected, K_FOREVER);
		LOG_INF("Connecting established!");
		default_screen = 4;
	}

	k_msgq_put(&button_action_msgq, &default_screen, K_FOREVER);
	LOG_INF("Submitting work from main");
	k_work_submit(&button_action_work);

	/* No need for loop, threads will handle program flow */
}
