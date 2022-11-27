/*
 * Copyright (c) 2022 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MYNAME	"John Hackworth"
#define TITLE	"Nanotech Engineer"
#define HANDLE	"@Kurt_Vonnegut"
#define DEFAULT_FRAME	3

/* Logging */
#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(golioth_magtag, LOG_LEVEL_DBG);

/* Persistent settings */
#include <zephyr/settings/settings.h>
#define SETTINGS_ROOT	"nametag"
#define SETTINGS_NAME	"name"
#define SETTINGS_TITLE	"title"
#define SETTINGS_HANDLE	"handle"
#define GET_ENDP(i)	SETTINGS_ROOT "/" i

#define NAME_SIZE 64
static char _myname[NAME_SIZE] = MYNAME;
static char _title[NAME_SIZE] = TITLE;
static char _handle[NAME_SIZE] = HANDLE;

struct nametag_ctx{
	char *data;
	char *key;
	char *end_p;
};

struct nametag_ctx nametag_ctx_arr[3] = {
	{ _myname, SETTINGS_NAME, GET_ENDP(SETTINGS_NAME) },
	{ _title, SETTINGS_TITLE, GET_ENDP(SETTINGS_TITLE) },
	{ _handle, SETTINGS_HANDLE, GET_ENDP(SETTINGS_HANDLE) }
};

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
static K_SEM_DEFINE(manual_get_complete, 0, 1);
struct k_mutex epaper_mutex;

/* Prototypes */
bool process_update_and_store(char *new_data,
		uint8_t len,
		struct nametag_ctx ctx,
		bool show_msgs);
void refresh_delay_timer_handler(struct k_timer *dummy);

/* Timers */
K_TIMER_DEFINE(refresh_delay_timer, refresh_delay_timer_handler, NULL);

static int nametag_settings_set(const char *name, size_t len,
		settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	size_t name_len;
	int rc;

	name_len = settings_name_next(name, &next);

	if (!next) {
		if (!strncmp(name, SETTINGS_NAME, name_len)) {
			rc = read_cb(cb_arg, &_myname, NAME_SIZE);
			return 0;
		}
		if (!strncmp(name, SETTINGS_TITLE, name_len)) {
			rc = read_cb(cb_arg, &_title, NAME_SIZE);
			return 0;
		}
		if (!strncmp(name, SETTINGS_HANDLE, name_len)) {
			rc = read_cb(cb_arg, &_handle, NAME_SIZE);
			return 0;
		}
	}
	return -ENOENT;
}

struct settings_handler my_conf = {
	.name = SETTINGS_ROOT,
	.h_set = nametag_settings_set,
};

void remove_quotes(char *str_d, const char *str_s, uint8_t len_with_quotes) {
	strcpy(str_d, str_s+1);
	*(str_d+len_with_quotes-2) = '\0';
}

static int update_handler(struct golioth_req_rsp *rsp)
{
	if (!k_sem_count_get(&manual_get_complete)) {
		/* Ignore until a manual get if finished */
		return 0;
	}

	if (rsp->err) {
		LOG_ERR("Error receiving LightDB State update: %d", rsp->err);
		return rsp->err;
	}

	struct nametag_ctx *ctx = (struct nametag_ctx*)rsp->user_data;
	LOG_HEXDUMP_INF(rsp->data, rsp->len, "Data");
	LOG_DBG("Userdata: %s", ctx->key);

	/* hacks to deal with rsp->data being wrapped in quotes */
	char nametag_data[NAME_SIZE];
	remove_quotes(nametag_data, rsp->data, rsp->len);

	if (process_update_and_store(nametag_data, rsp->len, *ctx, false)) {
		LOG_DBG("Successfully updated");
		k_timer_start(&refresh_delay_timer, K_MSEC(1500), K_NO_WAIT);
	}
	return 0;
}

static void golioth_on_connect(struct golioth_client *client)
{
	k_sem_give(&connected);

	int err;

	for (uint8_t i=0; i<ARRAY_SIZE(nametag_ctx_arr); i++) {
		err = golioth_lightdb_observe_cb(client, nametag_ctx_arr[i].key,
					 GOLIOTH_CONTENT_FORMAT_APP_JSON,
					 update_handler, &nametag_ctx_arr[i]);

		if (err) {
			LOG_WRN("failed to observe lightdb path: %d", err);
		}
	}
}

bool process_update_and_store(char *new_data,
		uint8_t len,
		struct nametag_ctx ctx,
		bool show_msgs) {

	int err = 0;
	char line_buf[32];
	snprintk(line_buf, sizeof(line_buf), "Fetching %s... Success!", ctx.key);
	if (show_msgs) {
		epaper_autowrite(line_buf, strlen(line_buf));
	}

	if (strcmp(new_data, ctx.data)==0) {
		LOG_DBG("Local data already up-to-date");
		if (show_msgs) {
			epaper_autowrite("No change", 9);
		}
	}
	else {
		memcpy(ctx.data, new_data, NAME_SIZE);

		LOG_DBG("Saving: %s", ctx.end_p);
		err = settings_save_one(ctx.end_p, (const void *)ctx.data, NAME_SIZE);
		if (err) {
			LOG_DBG("failed to write: %s %d", ctx.data, strlen(ctx.data));
		}
		else {
			LOG_DBG("Saved to flash");
			if (show_msgs) {
				epaper_autowrite("Saved to flash", 14);
			}
		}
		return true;
	}
	return false;
}

int fetch_name_from_golioth(struct nametag_ctx ctx) {
	int err = 0;
	static uint8_t row_idx = 1;

	if (!golioth_is_connected(client)) {
		err = -ENETDOWN;
	}
	else {
		LOG_INF("Fetching %s information from Golioth LightDB State", ctx.key);
		uint8_t name_update[NAME_SIZE];
		size_t len = NAME_SIZE;
		int err = golioth_lightdb_get(client,
					ctx.key,
					GOLIOTH_CONTENT_FORMAT_APP_JSON,
					name_update,
					&len);

		if (err) {
			LOG_ERR("Unable to fetch name information from Golioth: %d", err);
			epaper_write("Unable to fetch", 15, (row_idx++)*2, FULL_WIDTH, 2);
			return err;
		}

		if (strncmp(name_update, "null", 4)==0) {
			epaper_autowrite("Endpoint missing on Golioth", 27);
			LOG_INF("Endpoint doesn't exist: %s", ctx.key);
		}
		else {
			/* hack to remove quotes received with JSON string */
			char nametag_data[NAME_SIZE];
			remove_quotes(nametag_data, name_update, len);

			LOG_INF("Received %s: %s", ctx.key, nametag_data);
			process_update_and_store(nametag_data,
					strlen(nametag_data),
					ctx,
					true);
		}
	}
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
	epaper_full_clear();
	epaper_show_full_frame(frame2);
	epaper_write_inverted("HELLO", 5, 2, CENTER, 2);
	epaper_write_inverted("my name is", 10, 4, CENTER, 1);
	epaper_write(_myname, strlen(_myname), 8, CENTER, 4);

	k_mutex_unlock(&epaper_mutex);
}

void nametag_green(void) {
	if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))!=0) {
		/* ePaper already in use, abort */
		return;
	}

	led_color_changer(ALLGREEN);

	epaper_full_clear();
	epaper_show_full_frame(frame1);

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
	epaper_write(firstname, strlen(firstname), 5, 216, 4);
	epaper_write(lastname, strlen(lastname), 10, 216, 4);

	k_mutex_unlock(&epaper_mutex);
}

void nametag_red(void) {
	if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))!=0) {
		/* ePaper already in use, abort */
		return;
	}

	led_color_changer(ALLRED);

	epaper_full_clear();
	epaper_show_full_frame(frame0);
	epaper_write(_title, strlen(_title), 1, 284, 2);
	epaper_write(_myname, strlen(_myname), 6, CENTER, 4);
	epaper_write(_handle, strlen(_handle), 13, 204, 2);

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
	epaper_full_clear();

	/* Use a partial write to draw the background */
	/* Change frame3 to the name of your array */
	epaper_show_full_frame(frame3);

	/* Write text on top of the background */
	epaper_write(_myname, strlen(_myname), 2, CENTER, 4);
	epaper_write_inverted(_title, strlen(_title), 11, CENTER, 2);
	epaper_write_inverted(_handle, strlen(_handle), 13, CENTER, 2);

	k_mutex_unlock(&epaper_mutex);
}

void nametag_yellow(void) {
	if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))!=0) {
		/* ePaper already in use, abort */
		return;
	}

	led_color_changer(ALLYELLOW);
	epaper_full_clear();
	epaper_show_full_frame(frame3);
	epaper_write(_myname, strlen(_myname), 2, CENTER, 4);
	epaper_write_inverted(_title, strlen(_title), 11, CENTER, 2);
	epaper_write_inverted(_handle, strlen(_handle), 13, CENTER, 2);

	k_mutex_unlock(&epaper_mutex);
}

void nametag_rainbow(void) {
	if (k_mutex_lock(&epaper_mutex, K_SECONDS(1))!=0) {
		/* ePaper already in use, abort */
		return;
	}

	LOG_INF("Fetching data");

	led_color_changer(RAINBOW);

	epaper_full_clear();
	epaper_autowrite("Fetching name from Golioth", 26);

	int err;
	for (uint8_t i=0; i<ARRAY_SIZE(nametag_ctx_arr); i++) {
		err = fetch_name_from_golioth(nametag_ctx_arr[i]);
		if (err != 0) {
			if (err == -ENETDOWN) {
				epaper_autowrite("Err: Not connected to Golioth", 29);
			}
			else {
				epaper_autowrite("Unknown error", 13);
			}
			k_mutex_unlock(&epaper_mutex);
			return;
		}
	}
	k_mutex_unlock(&epaper_mutex);
	k_sem_give(&manual_get_complete);
	nametag_yellow();
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
					epaper_write("Starting WiFi...", 16, 0, 160, 2);
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

void refresh_delay_timer_handler(struct k_timer *dummy)
{
	uint8_t new_frame = DEFAULT_FRAME;
	k_msgq_put(&button_action_msgq, &new_frame, K_NO_WAIT);
	k_work_submit(&button_action_work);
}

void main(void)
{
	int err;

	LOG_DBG("Start MagTag demo");

	#ifdef CONFIG_MAGTAG_NAME
	LOG_INF("Device name: %s", CONFIG_MAGTAG_NAME);
	#else
	#define CONFIG_MAGTAG_NAME "MagTag"
	#endif

	k_mutex_init(&epaper_mutex);

	/* Persistent settings */
	settings_subsys_init();
	settings_register(&my_conf);
	settings_load();

	ws2812_init();
	led_states[0].color = BLUE; led_states[0].state = 1;
	led_states[1].color = BLUE; led_states[1].state = 1;
	led_states[2].color = BLACK; led_states[2].state = 1;
	led_states[3].color = BLACK; led_states[3].state = 1;
	ws2812_blit(strip, led_states, STRIP_NUM_PIXELS);

	/* buttons */
	buttons_init(button_pressed);

	epaper_init();
	epaper_full_clear();
	epaper_show_full_frame(golioth_nametag);
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
	k_work_submit(&button_action_work);

	/* No need for loop, threads will handle program flow */
}
