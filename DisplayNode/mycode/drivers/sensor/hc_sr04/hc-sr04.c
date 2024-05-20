
#define DT_DRV_COMPAT elecfreaks_hc_sr04

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hc_sr04, CONFIG_SENSOR_LOG_LEVEL);

// Timings defined by spec
#define T_TRIG_PULSE_US       11
#define T_INVALID_PULSE_US    25000
#define T_MAX_WAIT_MS         130
// #define T_MAX_WAIT_MS         500
// #define T_MAX_WAIT_MS         2000
#define T_SPURIOS_WAIT_US     145
#define METERS_PER_SEC        340

enum hc_sr04_state {
    HC_SR04_STATE_IDLE,
    HC_SR04_STATE_TRIG_SENT,
    HC_SR04_STATE_ECHO_BEGUN,
    HC_SR04_STATE_ERROR
};

struct hc_sr04_data {
    struct sensor_value sensor_value;
    struct gpio_callback echo_cb_data;
    enum hc_sr04_state state;
    uint32_t start_time;
    uint32_t end_time;
    struct k_sem semaphore;
};

struct hc_sr04_config {
    struct gpio_dt_spec trigger;
    struct gpio_dt_spec echo;
};

static void echo_callback(const struct device *dev, struct gpio_callback *cb,
        gpio_port_pins_t pins)
{
    // struct hc_sr04_data *data = dev->data; // <- bad
    struct hc_sr04_data *data = CONTAINER_OF(cb, struct hc_sr04_data, echo_cb_data);
    switch (data->state) {
    case HC_SR04_STATE_TRIG_SENT:
        data->start_time = k_cycle_get_32();
        data->state = HC_SR04_STATE_ECHO_BEGUN;
        break;
    case HC_SR04_STATE_ECHO_BEGUN:
        data->end_time = k_cycle_get_32();
        data->state = HC_SR04_STATE_IDLE;
        k_sem_give(&data->semaphore);
        break;
    default:
        data->state = HC_SR04_STATE_ERROR;
        break;
    }
}

static int hc_sr04_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    const struct hc_sr04_config *config = dev->config;
    struct hc_sr04_data *data = dev->data;

    // Send trigger pulse
    gpio_pin_set_dt(&config->trigger, 1);
    k_busy_wait(T_TRIG_PULSE_US);
    gpio_pin_set_dt(&config->trigger, 0);

    data->state = HC_SR04_STATE_TRIG_SENT;

    // Wait for echo pulse to be received
    if (k_sem_take(&data->semaphore, K_MSEC(T_MAX_WAIT_MS)) != 0) {
        LOG_WRN("No response from HC-SR04");
        return -EIO;
    }

    // assert in correct state
    __ASSERT(data->state == HC_SR04_STATE_IDLE, "Invalid state");

    // Calculate duration (subtraction may underflow but is ok)
    uint32_t duration = data->end_time - data->start_time;
    duration = k_cyc_to_us_near32(duration);

    if (duration < T_INVALID_PULSE_US && duration > T_TRIG_PULSE_US) {
        uint32_t distance = duration * METERS_PER_SEC / 2;
        data->sensor_value.val1 = distance / 1000000;
        data->sensor_value.val2 = distance % 1000000;
    } else {
        LOG_INF("Invalid ultrasonic measurement");
        data->sensor_value.val1 = 0;
        data->sensor_value.val2 = 0;
        k_usleep(T_SPURIOS_WAIT_US);
    }

    return 0;
}

static int hc_sr04_channel_get(const struct device *dev,
        enum sensor_channel chan, struct sensor_value *val)
{
    const struct hc_sr04_data *data = dev->data;

    switch (chan) {
    case SENSOR_CHAN_DISTANCE:
        // copy sensor value
        *val = data->sensor_value;
        break;
    default:
        return -ENOTSUP;
    }

    return 0;
}

static const struct sensor_driver_api hc_sr04_driver_api = {
    .sample_fetch = hc_sr04_sample_fetch,
    .channel_get = hc_sr04_channel_get
};

static int hc_sr04_init(const struct device *dev)
{
    const struct hc_sr04_config *config = dev->config;
    struct hc_sr04_data *data = dev->data;

    int ret;

    if (!device_is_ready(config->trigger.port)) {
        LOG_ERR("Trigger GPIO not ready");
        return -ENODEV;
    }

    if (!device_is_ready(config->echo.port)) {
        LOG_ERR("Echo GPIO not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&config->trigger, GPIO_OUTPUT);
    if (ret < 0) {
        LOG_ERR("Could not configure trigger GPIO (%d)", ret);
        return ret;
    }

    ret = gpio_pin_configure_dt(&config->echo, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Could not configure echo GPIO (%d)", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&config->echo, GPIO_INT_EDGE_BOTH);
    if (ret < 0) {
        LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
                ret, config->echo.port->name, config->echo.pin);
        return ret;
    }

    gpio_init_callback(&data->echo_cb_data, echo_callback, BIT(config->echo.pin));
    ret = gpio_add_callback_dt(&config->echo, &data->echo_cb_data);
    if (ret < 0) {
        LOG_ERR("Error %d: failed to add callback for echo pin", ret);
        return ret;
    }

    ret = k_sem_init(&data->semaphore, 0, 1);
    if (ret < 0) {
        LOG_ERR("Error %d: failed to initialise semaphore for HC-SR04", ret);
        return ret;
    }

    data->state = HC_SR04_STATE_IDLE;

    return 0;
}

#define HC_SR04_INIT(i) \
    static struct hc_sr04_data hc_sr04_data_##i; \
    \
    static struct hc_sr04_config hc_sr04_config_##i = { \
        .trigger = GPIO_DT_SPEC_INST_GET(i, trig_gpios), \
        .echo = GPIO_DT_SPEC_INST_GET(i, echo_gpios) \
    }; \
    \
    DEVICE_DT_INST_DEFINE(i, hc_sr04_init, NULL, &hc_sr04_data_##i, \
            &hc_sr04_config_##i, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
            &hc_sr04_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HC_SR04_INIT)

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "HC_SR04 driver enabled without any devices"
#endif
