#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(screen_display, LOG_LEVEL_INF);

#include "screen_display.h"
#include "synchronisation.h"
#include "images.h"

static const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

// Not enough internal RAM to store the entire image to display
// -> instead, display it in segments of 1/3 at a time
#define DISPLAY_SEG_HEIGHT (DISPLAY_HEIGHT / 3)
#define DISPLAY_SEG_PIXELS (DISPLAY_WIDTH * DISPLAY_SEG_HEIGHT)
#define DISPLAY_SEG_BYTES (DISPLAY_SEG_PIXELS * BYTES_PER_PIXEL)

uint8_t buf[DISPLAY_SEG_HEIGHT][DISPLAY_BYTES_PER_ROW];

static volatile enum DisplayMode display_mode;
// static volatile int frame_num;
static enum Orientation display_orientation;

static const uint8_t *image_buffer = NULL;
static const uint8_t *anim_buffers[MAX_ANIMATION_FRAMES] = {0};
static const uint8_t *reverse_image_buffer = NULL;
static const uint8_t *reverse_anim_buffers[MAX_ANIMATION_FRAMES] = {0};

// static void screen_thread(void *, void *, void *);

// K_THREAD_DEFINE(screen_tid, SCREEN_THREAD_STACK_SIZE, screen_thread, NULL, NULL, NULL,
//         SCREEN_THREAD_PRIORITY, 0, 0);

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

    display_mode = DISPLAY_MODE_OFF;

    // int ret = display_set_orientation(display_dev, DISPLAY_ORIENTATION_ROTATED_180);
    // if (ret != 0) {
    //     LOG_ERR("Faield to set display orientation: %d", ret);
    // }

    int ret = display_blanking_off(display_dev);
    if (ret != 0) {
        return ret;
    }
    return screen_display_welcome();
}

static int screen_display_image(const uint8_t *buffer)
{
    int ret = 0;

    memcpy(buf, buffer, DISPLAY_SEG_BYTES);
    struct display_buffer_descriptor buf_desc = {
        .buf_size = DISPLAY_SEG_BYTES,
        .pitch = DISPLAY_WIDTH,
        .width = DISPLAY_WIDTH,
        .height = DISPLAY_SEG_HEIGHT
    };

    ret = display_write(display_dev, 0, 0, &buf_desc, buf);
    if (ret) {
        return ret;
    }

    memcpy(buf, buffer + DISPLAY_SEG_BYTES, DISPLAY_SEG_BYTES);

    ret = display_write(display_dev, 0, DISPLAY_SEG_HEIGHT, &buf_desc, buf);
    if (ret) {
        return ret;
    }

    memcpy(buf, buffer + DISPLAY_SEG_BYTES * 2, DISPLAY_SEG_BYTES);

    ret = display_write(display_dev, 0, DISPLAY_SEG_HEIGHT * 2, &buf_desc, buf);
    if (ret) {
        return ret;
    }

    return ret;
}

void screen_display_save_image(const void *buffer)
{
    if (image_buffer != NULL) {
        k_free((void *) image_buffer);
    }
    image_buffer = buffer;
}

void screen_display_save_image_reverse(const void *buffer)
{
    if (reverse_image_buffer != NULL) {
        k_free((void *) reverse_image_buffer);
    }
    reverse_image_buffer = buffer;
}

int screen_display_save_animation_frame(int frame_num, const void *buffer)
{
    LOG_INF("Saving upright frame number: %d", frame_num);
    if (frame_num < 0 || frame_num >= MAX_ANIMATION_FRAMES) {
        LOG_ERR("Tried to save invalid frame number: %d", frame_num);
        k_free((uint8_t *) buffer - 1);
        return 1;
    }

    if (anim_buffers[frame_num] != NULL) {
        // subtract 1 because the first byte contains the frame number
        k_free((void *) (anim_buffers[frame_num] - 1));
    }

    anim_buffers[frame_num] = buffer;

    return 0;
}

int screen_display_save_animation_frame_reverse(int frame_num, const void *buffer)
{
    LOG_INF("Saving reverse frame number: %d", frame_num);
    if (frame_num < 0 || frame_num >= MAX_ANIMATION_FRAMES) {
        LOG_ERR("Tried to save invalid frame number: %d", frame_num);
        k_free((uint8_t *) buffer - 1);
        return 1;
    }

    if (reverse_anim_buffers[frame_num] != NULL) {
        // subtract 1 because the first byte contains the frame number
        k_free((void *) (reverse_anim_buffers[frame_num] - 1));
    }

    reverse_anim_buffers[frame_num] = buffer;

    return 0;
}

int screen_display_set_mode(enum DisplayMode mode)
{
    int ret;

    // if (display_mode == mode) {
    //     return 0;
    // }

    display_mode = mode;

    switch (mode) {
    case DISPLAY_MODE_OFF:
        ret = display_blanking_on(display_dev);
        if (ret) {
            return ret;
        }
        break;
    case DISPLAY_MODE_IMAGE:
        if (display_orientation == ORIENTATION_DOWN) {
            if (reverse_image_buffer == NULL) {
                LOG_ERR("Tried to display reverse image when none received");
                return 1;
            }
            ret = screen_display_image(reverse_image_buffer);
            if (ret) {
                return ret;
            }
        } else {
            if (image_buffer == NULL) {
                LOG_ERR("Tried to display upright image when none received");
                return 1;
            }
            ret = screen_display_image(image_buffer);
            if (ret) {
                return ret;
            }
        }
        // deliberately fall through
    case DISPLAY_MODE_DISTANCE:
        ret = display_blanking_off(display_dev);
        if (ret) {
            return ret;
        }
        break;
    case DISPLAY_MODE_ANIMATION:
        break;
    }

    return 0;
}

int screen_display_set_animation_frame(int frame)
{
    int ret;

    if (display_mode != DISPLAY_MODE_ANIMATION) {
        return 0;
    }

    // if (frame == frame_num) {
    //     return 0;
    // }

    // if (frame_num < 0 || frame_num >= MAX_ANIMATION_FRAMES) {
    //     LOG_ERR("Tried to set invalid frame number: %d", frame_num);
    //     return 1;
    // }

    frame = frame % MAX_ANIMATION_FRAMES;

    if (display_orientation == ORIENTATION_DOWN) {
        if (reverse_anim_buffers[frame] == NULL) {
            LOG_ERR("Tried to display a frame that has not been received yet");
            return 2;
        }

        ret = screen_display_image(reverse_anim_buffers[frame]);
    } else {
        if (anim_buffers[frame] == NULL) {
            LOG_ERR("Tried to display a frame that has not been received yet");
            return 2;
        }

        ret = screen_display_image(anim_buffers[frame]);
    }

    return ret;
}


// static void screen_thread(void *, void *, void *)
// {
//     int ret;
    
//     for (;;) {
//         ret = k_event_wait(&tick_event, 1, true, K_FOREVER);
//         printk("RECEIVED TICK ========================================\n");

//         uint32_t frame_num = tick % MAX_ANIMATION_FRAMES;
//         screen_display_set_animation_frame(frame_num);
//     }
// }

int screen_display_set_orientation(enum Orientation orientation)
{
    int ret;
    if (orientation == ORIENTATION_DOWN) {
        ret = display_set_orientation(display_dev, DISPLAY_ORIENTATION_ROTATED_180);
        if (ret != 0) {
            LOG_ERR("Faield to set display orientation: %d", ret);
            return ret;
        }
        // if (display_mode == DISPLAY_MODE_IMAGE) {
        //     // Display the reverse image
        //     ret = screen_display_image(reverse_image_buffer);
        //     if (ret) {
        //         return ret;
        //     }
        // }
    } else {
        ret = display_set_orientation(display_dev, DISPLAY_ORIENTATION_NORMAL);
        if (ret != 0) {
            LOG_ERR("Faield to set display orientation: %d", ret);
            return ret;
        }
        // if (display_mode == DISPLAY_MODE_IMAGE) {
        //     // Display the right-side up image
        //     ret = screen_display_image(image_buffer);
        //     if (ret) {
        //         return ret;
        //     }
        // }
    }

    display_orientation = orientation;
    // Use this to refresh screen.
    ret = screen_display_set_mode(display_mode);

    return 0;
}

int screen_display_welcome(void)
{
    int ret = 0;

    memcpy(buf, welcome, sizeof(welcome) / 2);

    struct display_buffer_descriptor buf_desc = {
        .buf_size = sizeof(welcome) / 2,
        .pitch = WELCOME_WIDTH,
        .width = WELCOME_WIDTH,
        .height = WELCOME_HEIGHT / 2
    };

    ret = display_write(display_dev, WELCOME_X_POS, WELCOME_Y_POS, &buf_desc, buf);
    if (ret) {
        return ret;
    }

    memcpy(buf, (uint8_t *) welcome + sizeof(welcome) / 2, sizeof(welcome) / 2);

    ret = display_write(display_dev, WELCOME_X_POS, WELCOME_Y_POS + WELCOME_HEIGHT / 2, &buf_desc, buf);

    return ret;
}

int screen_display_distance(float dist)
{
    int ret = 0;
    static char dist_text[32];

    if (display_mode != DISPLAY_MODE_DISTANCE) {
        return 0;
    }

    snprintk(dist_text, 32, "%f", (double) dist);

    ret = screen_display_image((uint8_t *) black);
    if (ret != 0) {
        return ret;
    }
    
    memcpy(buf, distance, sizeof(distance));

    struct display_buffer_descriptor buf_desc = {
        .buf_size = sizeof(distance),
        .pitch = DISTANCE_WIDTH,
        .width = DISTANCE_WIDTH,
        .height = DISTANCE_HEIGHT
    };

    ret = display_write(display_dev, DISTANCE_X_POS, DISTANCE_Y_POS, &buf_desc, buf);
    if (ret != 0) {
        return ret;
    }

    // Calculate distance text images

    int x = TEXT_X_POS;
    buf_desc.buf_size = sizeof(zero);
    buf_desc.pitch = TEXT_WIDTH;
    buf_desc.width = TEXT_WIDTH;
    buf_desc.height = TEXT_HEIGHT;

    for (char *p = dist_text; *p; ++p) {
        switch (*p) {
        case '0':
            memcpy(buf, zero, sizeof(zero));
            ret = display_write(display_dev, x, TEXT_Y_POS, &buf_desc, buf);
            break;
        case '1':
            memcpy(buf, one, sizeof(one));
            ret = display_write(display_dev, x, TEXT_Y_POS, &buf_desc, buf);
            break;
        case '2':
            memcpy(buf, two, sizeof(two));
            ret = display_write(display_dev, x, TEXT_Y_POS, &buf_desc, buf);
            break;
        case '3':
            memcpy(buf, three, sizeof(three));
            ret = display_write(display_dev, x, TEXT_Y_POS, &buf_desc, buf);
            break;
        case '4':
            memcpy(buf, four, sizeof(four));
            ret = display_write(display_dev, x, TEXT_Y_POS, &buf_desc, buf);
            break;
        case '5':
            memcpy(buf, five, sizeof(five));
            ret = display_write(display_dev, x, TEXT_Y_POS, &buf_desc, buf);
            break;
        case '6':
            memcpy(buf, six, sizeof(six));
            ret = display_write(display_dev, x, TEXT_Y_POS, &buf_desc, buf);
            break;
        case '7':
            memcpy(buf, seven, sizeof(seven));
            ret = display_write(display_dev, x, TEXT_Y_POS, &buf_desc, buf);
            break;
        case '8':
            memcpy(buf, eight, sizeof(eight));
            ret = display_write(display_dev, x, TEXT_Y_POS, &buf_desc, buf);
            break;
        case '9':
            memcpy(buf, nine, sizeof(nine));
            ret = display_write(display_dev, x, TEXT_Y_POS, &buf_desc, buf);
            break;
        case '.':
            memcpy(buf, dot, sizeof(dot));
            ret = display_write(display_dev, x, TEXT_Y_POS, &buf_desc, buf);
            break;
        default:
            LOG_ERR("Unexpected character '%c' in distance string", *p);
            break;
        }
        if (ret != 0) {
            return ret;
        }
        x += TEXT_WIDTH;
    }

    return ret;
}
