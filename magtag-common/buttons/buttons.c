#include "buttons.h"

/**
 * @brief set up buttons and interrupts
 *
 * @param handler the function to call when button interrupt occurs
 */
void buttons_init(gpio_callback_handler_t handler)
{
    gpio_pin_configure_dt(&button0, GPIO_INPUT);
	gpio_pin_configure_dt(&button1, GPIO_INPUT);
	gpio_pin_configure_dt(&button2, GPIO_INPUT);
	gpio_pin_configure_dt(&button3, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button0, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&button2, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_pin_interrupt_configure_dt(&button3, GPIO_INT_EDGE_TO_ACTIVE);
	uint32_t button_mask = BIT(button0.pin) | BIT(button1.pin) | BIT(button2.pin) | BIT(button3.pin);
	gpio_init_callback(&button_cb_data, handler, button_mask);
	gpio_add_callback(button0.port, &button_cb_data);
	gpio_add_callback(button1.port, &button_cb_data);
	gpio_add_callback(button2.port, &button_cb_data);
	gpio_add_callback(button3.port, &button_cb_data);
}
