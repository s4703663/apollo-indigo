#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include "orientation.h"
#include "message_handler.h"

#ifdef CONFIG_SENSOR

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(orientation, LOG_LEVEL_DBG);

static enum Orientation orientation;

static const struct device *mpu6050 = DEVICE_DT_GET_ONE(invensense_mpu6050);

static void orientation_thread(void *, void *, void *);

K_THREAD_DEFINE(orientation_tid, ORIENTATION_THREAD_STACK_SIZE, orientation_thread, NULL, NULL, NULL,
        ORIENTATION_THREAD_PRIORITY, K_FP_REGS, 0);

int orientation_init(void)
{
    if (!device_is_ready(mpu6050)) {
        LOG_ERR("Accelerometer device %s not read\n", mpu6050->name);
        return 1;
    }

    orientation = ORIENTATION_UP;

    return 0;
}

enum Orientation orientation_get(void)
{
    int ret;
    struct sensor_value accel[3];

    ret = sensor_sample_fetch(mpu6050);

    if (ret != 0) {
        LOG_ERR("Failed to fetch sample for accelerometer");
        return ret;
    }

    ret = sensor_channel_get(mpu6050, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (ret != 0) {
        LOG_ERR("Failed to get accelerometer channel data");
        return ret;
    }

    float y_accel = sensor_value_to_float(&accel[1]);

    // Provide a little bit of hysteresis
    switch (orientation) {
    case ORIENTATION_UP:
        if (y_accel < -4.f) {
            orientation = ORIENTATION_DOWN;
        }
        break;
    case ORIENTATION_DOWN:
        if (y_accel > 4.f) {
            orientation = ORIENTATION_UP;
        }
        break;
    }

    return orientation;
}

static void orientation_thread(void *, void *, void *)
{
    k_thread_suspend(orientation_tid);

    int ret = orientation_init();
    if (ret != 0) {
        return;
    }

    struct Msg msg;
    
    msg.topic = MQTT_MSG_ORIENT_TOPIC;

    for (;;) {
        enum Orientation orientation = orientation_get();
        msg.data.orient_topic_data.orientation = orientation;
        message_handler_send(&msg);
        k_msleep(500);
    }
}

#endif
