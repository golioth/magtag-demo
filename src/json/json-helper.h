#ifndef __JSON_HELPER_H_
#define __JSON_HELPER_H_

#include <data/json.h>

struct atomic_led {
	const char *color;
	int8_t state;
};

static const struct json_obj_descr atomic_led_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct atomic_led, color, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct atomic_led, state, JSON_TOK_NUMBER)
};

#endif
