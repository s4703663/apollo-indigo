#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "distance.h"
#include "message_handler.h"

#ifdef CONFIG_SENSOR

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(distance, LOG_LEVEL_DBG);

const struct device *sensor = DEVICE_DT_GET(DT_NODELABEL(us0));

static void distance_thread(void *, void *, void *);

K_THREAD_DEFINE(distance_tid, DISTANCE_THREAD_STACK_SIZE, distance_thread, NULL, NULL, NULL,
        DISTANCE_THREAD_PRIORITY, K_FP_REGS, 0);

int distance_init(void)
{
    if (!device_is_ready(sensor)) {
        LOG_ERR("Sensor not ready");
        return 1;
    }

    return 0;
}

float distance_get(void)
{
    int ret;
    struct sensor_value val;

    ret = sensor_sample_fetch(sensor);
    if (ret < 0) {
        LOG_ERR("Could not fetch sample (%d)", ret);
        return 1;
    }
    ret = sensor_channel_get(sensor, SENSOR_CHAN_DISTANCE, &val);
    if (ret < 0) {
        LOG_ERR("Could not get sample (%d)", ret);
        return 0;
    }
    float distance = sensor_value_to_float(&val);

    return distance;
}

// Defined in main.c
extern volatile enum DisplayMode current_display_mode;

static void distance_thread(void *, void *, void *)
{
    k_thread_suspend(distance_tid);

    int ret = distance_init();
    if (ret != 0) {
        return;
    }

    struct Msg msg;
    msg.topic = MQTT_MSG_DIST_TOPIC;

    for (;;) {
        float distance = distance_get();
        msg.data.dist_topic_data.distance = distance;
        if (current_display_mode == DISPLAY_MODE_DISTANCE) {
            message_handler_send(&msg);
        }
        // printk("%f\n", distance);
        k_msleep(500);
    }
}
#endif
