#ifndef __ACCEL_H_
#define __ACCEL_H_

#include <zephyr/drivers/sensor.h>

extern struct device *sensor;

/* prototypes */
void accelerometer_init(void);
void fetch_and_display(const struct device *sensor, struct sensor_value accel[3]);

#endif
