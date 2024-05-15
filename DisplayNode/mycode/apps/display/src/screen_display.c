#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#include "screen_display.h"

static const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

// uint8_t *buf;
uint8_t buf[80][960];

#define TEST_SIZE 20

int screen_display_init(void)
{
    struct display_capabilities capabilities;
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Device %s not found. Aborting sample.", display_dev->name);
        return 0;
    }

    LOG_INF("Display sample for %s", display_dev->name);

    display_set_pixel_format(display_dev, PIXEL_FORMAT_RGB_888);

    display_get_capabilities(display_dev, &capabilities);

    LOG_INF("capabilities.screen_info: %d", capabilities.screen_info);
    LOG_INF("capabilities.current_pixel_format: %d", capabilities.current_pixel_format);
    LOG_INF("capabilities.supported_pixel_formats: %d", capabilities.supported_pixel_formats);
    LOG_INF("capabilities.x_resolution: %d", capabilities.x_resolution);
    LOG_INF("capabilities.y_resolution: %d", capabilities.y_resolution);

    // buf = k_malloc(TEST_SIZE * TEST_SIZE * 3);
    // buf = k_malloc(230400); //320 * 240 * 3,
    // buf = malloc(60000); //320 * 240 * 3,
    // buf = k_malloc(230400);
    // if (buf == NULL) {
    //     return -1;
    // }

    return 0;
}

int screen_display_image(const void *buffer)
{
    int ret = 0;

    memcpy(buf, buffer, 76800);
    struct display_buffer_descriptor buf_desc = {
        .buf_size = 76800, //230400, //320 * 240 * 3,
        .pitch = 320,
        .width = 320,
        .height = 80
    };
    // struct display_buffer_descriptor buf_desc = {
    //     .buf_size = 100 * 100 * 3, //1200, //320 * 240 * 3,
    //     .pitch = 100, // 20,
    //     .width = 100, // 20,
    //     .height = 100 // 20
    // };

    ret = display_blanking_off(display_dev);
    if (ret) {
        return ret;
    }

    // return display_write(display_dev, 0, 0, &buf_desc, buffer);
    ret = display_write(display_dev, 0, 0, &buf_desc, buf);
    if (ret) {
        return ret;
    }

    memcpy(buf, (uint8_t *) buffer + 76800, 76800);

    ret = display_write(display_dev, 0, 80, &buf_desc, buf);
    if (ret) {
        return ret;
    }

    memcpy(buf, (uint8_t *) buffer + 76800 * 2, 76800);

    ret = display_write(display_dev, 0, 80 * 2, &buf_desc, buf);
    if (ret) {
        return ret;
    }

    return ret;

    // int ret;
    // struct display_buffer_descriptor buf_desc = {
    //     .buf_size = TEST_SIZE * TEST_SIZE * 3,
    //     .pitch = TEST_SIZE,
    //     .width = TEST_SIZE,
    //     .height = TEST_SIZE
    // };

    // unsigned i = 0;
    // for (int y = 0; y < TEST_SIZE; y++) {
    //     for (int x = 0; x < TEST_SIZE; x++) {
    //         buf[i++] = 0xff; // red
    //         buf[i++] = 0x00; // green
    //         buf[i++] = 0x00; // blue
    //     }
    // }

    // ret = display_blanking_off(display_dev);
    // if (ret) {
    //     return ret;
    // }

    // return display_write(display_dev, 0, 0, &buf_desc, buf);
}
