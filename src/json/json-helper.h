#ifndef __JSON_HELPER_H_
#define __JSON_HELPER_H_

#include <data/json.h>

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

#endif
