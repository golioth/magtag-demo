#include "accel.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(golioth_accel, LOG_LEVEL_DBG);

struct device *sensor;

void accelerometer_init(void)
{
    sensor = (void *)DEVICE_DT_GET_ANY(st_lis2dh);
	if (sensor == NULL) {
		LOG_ERR("No device found");
		return;
	}
	if (!device_is_ready(sensor)) {
		LOG_ERR("Device %s is not ready", sensor->name);
		return;
	}
}

void fetch_and_display(const struct device *sensor, struct sensor_value accel[3])
{
    static unsigned int count;
	//struct sensor_value accel[3];
	const char *overrun = "";
	int rc = sensor_sample_fetch(sensor);

	++count;
	if (rc == -EBADMSG) {
		/* Sample overrun.  Ignore in polled mode. */
		if (IS_ENABLED(CONFIG_LIS2DH_TRIGGER)) {
			overrun = "[OVERRUN] ";
		}
		rc = 0;
	}
	if (rc == 0) {
		rc = sensor_channel_get(sensor,
					SENSOR_CHAN_ACCEL_XYZ,
					accel);
	}
	if (rc < 0) {
		LOG_ERR("ERROR: Update failed: %d", rc);
	}
}