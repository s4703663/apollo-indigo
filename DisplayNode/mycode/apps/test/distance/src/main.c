/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
// #include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
// #define LED0_NODE DT_ALIAS(led0)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

// static const struct gpio_dt_spec pulse_out = DT_ALIAS(gpio0);
// static const struct gpio_dt_spec pulse_in = DT_ALIAS(gpio1);

// float get_distance() {
// 	gpio_pin_set_dt(&pulse_out, 1);
// 	gpio_pin_set_dt(&pulse_out, 0);
// 	int64_t start = k_uptime_get();
// 	// Wait for response from pulse in:
// 	while (1) {
// 		int result = gpio_pin_get_dt(&pulse_out);
// 		if (result) {
// 			break;
// 		}
// 	}
// 	int64_t stop = k_uptime_get();
// 	int64_t duration = stop - start;
// 	// Speed of sound is 343 m/s
// 	// sound travels there and back (twice the distance) therefore divide by 2
// 	float distance = (float) duration * 0.343f / 2.f;
// 	return distance;

// }

int main(void)
{
	int ret;
	// bool led_state = true;

	// if (!(gpio_is_ready_dt(&pulse_out) && gpio_is_ready_dt(&pulse_in))) {
	// 	return 0;
	// }

	// ret = gpio_pin_configure_dt(&pulse_out, GPIO_INPUT);
	// if (ret < 0) {
	// 	return 0;
	// }

	// ret = gpio_pin_configure_dt(&pulse_in, GPIO_OUTPUT_ACTIVE);
	// if (ret < 0) {
	// 	return 0;
	// }

	// while (1) {
	// 	float distance = get_distance();
	// 	printk("Distance: %f\n", (double) distance);
	// 	k_msleep(SLEEP_TIME_MS);
	// }
	// return 0;

	const struct device *sensor;

	printk("Distance Example Application\n");

	sensor = DEVICE_DT_GET(DT_NODELABEL(us0));
	if (!device_is_ready(sensor)) {
		LOG_ERR("Sensor not ready");
		return 0;
	}

	while (1) {
		struct sensor_value val;

		ret = sensor_sample_fetch(sensor);
		if (ret < 0) {
			LOG_ERR("Could not fetch sample (%d)", ret);
			// return 0;
			continue;
		}

		ret = sensor_channel_get(sensor, SENSOR_CHAN_DISTANCE, &val);
		if (ret < 0) {
			LOG_ERR("Could not get sample (%d)", ret);
			return 0;
		}

		float distance = sensor_value_to_float(&val);
		printk("Raw sensor value: %d.%06d\n", val.val1, val.val2);
		printk("Sensor value: %f\n", (double) distance);

		k_sleep(K_MSEC(1000));
	}
}
