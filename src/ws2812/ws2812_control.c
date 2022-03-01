#include "ws2812_control.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(golioth_ws2812, LOG_LEVEL_DBG);

/* ws2812 */

/* mosfet control pin for ws2812 power rail */
#define NEOPOWER_NODE 	DT_ALIAS(neopower)
#if DT_NODE_HAS_STATUS(NEOPOWER_NODE, okay)
#define NEOPOWER		DT_GPIO_LABEL(NEOPOWER_NODE, gpios)
#define NEOPOWER_PIN	DT_GPIO_PIN(NEOPOWER_NODE, gpios)
#define NEOPOWER_FLAGS	DT_GPIO_FLAGS(NEOPOWER_NODE, gpios)
#endif

struct led_color_state led_states[STRIP_NUM_PIXELS];
struct led_rgb pixels[STRIP_NUM_PIXELS];

/**
 * @brief Zero out the buffer (all pixels off)
 * 
 */
void clear_pixels(void)
{
    memset(&pixels, 0x00, sizeof(pixels));
}

/**
 * @brief Set a single pixel color
 * 
 * @param pixel_n   the pixel number
 * @param color     the color_rgb value
 */
void set_pixel(struct led_color_state *states, uint8_t pixel_n, uint8_t color_n, int8_t state)
{
    if (pixel_n >= STRIP_NUM_PIXELS) return;
    if (color_n >= sizeof(colors)) return;
	//memcpy(&pixels[pixel_n], &color, sizeof(struct led_rgb));
    states[pixel_n].color = color_n;
    states[pixel_n].state = state;
}

void ws2812_blit(const struct device *dev, struct led_color_state *states, uint8_t pix_count)
{
	struct led_rgb buffer[pix_count];
	for (uint8_t i=0; i<pix_count; i++)
	{
        if (states[i].state == 0)
        {
		    memcpy(&buffer[i], &colors[0], sizeof(struct led_rgb));
        }
        else
        {
            memcpy(&buffer[i], &colors[states[i].color], sizeof(struct led_rgb));
        }
	}
	led_strip_update_rgb(strip, buffer, pix_count);
}

void ws2812_init(void) {
    /* ws2812 */

	/* Turn on power to the ws2812 neopixels */
	#if DT_NODE_HAS_STATUS(NEOPOWER_NODE, okay)
	const struct device *neopower_dev;
	neopower_dev = device_get_binding(NEOPOWER);
	int ret = gpio_pin_configure(neopower_dev, NEOPOWER_PIN, GPIO_OUTPUT_ACTIVE | NEOPOWER_FLAGS);
	if (ret < 0) {
		LOG_ERR("Failed to configure NEOPOWER pin: %d", ret);
	}
	gpio_pin_set(neopower_dev, NEOPOWER_PIN, 0);
	#endif

	#if defined(CONFIG_SOC_ESP32S2)
	/* This is a hack to fix incorrect SPI polarity on ESP32s2-based boards */
	#define SPI_CTRL_REG 0x8
	#define SPI_D_POL 19
	uint32_t* myreg = (uint32_t*)(DT_REG_ADDR(DT_PARENT(DT_ALIAS(led_strip)))+SPI_CTRL_REG);
	*myreg &= ~(1<<SPI_D_POL);
	LOG_ERR("Fixing ESP32s2 Register %p Value: 0x%x", myreg, *myreg);
	#endif

    for (uint8_t i=0; i<STRIP_NUM_PIXELS; i++) {
        led_states[i].color = 0;
        led_states[i].state = -1;
    }
}