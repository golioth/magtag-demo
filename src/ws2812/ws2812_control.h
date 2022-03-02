#ifndef __WS2812_CONTROL_H_
#define __WS2812_CONTROL_H_

#include <zephyr.h>
#include <drivers/led_strip.h>
#include <device.h>
#include <drivers/spi.h>
#include <sys/util.h>
#include <string.h>

#define STRIP_NODE		    DT_ALIAS(led_strip)
#define STRIP_NUM_PIXELS	DT_PROP(DT_ALIAS(led_strip), chain_length)
#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

static const struct led_rgb colors[] = {
	RGB(0x00, 0x00, 0x00), /* off */
	RGB(0x0F, 0x00, 0x00), /* red */
	RGB(0x00, 0x0F, 0x00), /* green */
	RGB(0x00, 0x00, 0x0F), /* blue */
};

/* Color name definitions match colors[] index */
#define BLACK	0
#define RED		1
#define GREEN	2
#define BLUE	3

struct led_color_state {
  uint8_t color;
  int8_t state;  
};

extern struct led_color_state led_states[STRIP_NUM_PIXELS];

extern struct led_rgb pixels[STRIP_NUM_PIXELS];
static const struct device *strip = DEVICE_DT_GET(STRIP_NODE);

/* ws2812 prototypes*/
void clear_pixels(void);
void set_pixel(struct led_color_state *states, uint8_t pixel_n, uint8_t color_n, int8_t state);
void ws2812_blit(const struct device *dev, struct led_color_state *states, uint8_t pix_count);
void ws2812_init(void);
void leds_immediate(uint8_t led3, uint8_t led2, uint8_t led1, uint8_t led0);
void set_leds(uint8_t led_num, const char * l_color, int8_t l_state);

#endif