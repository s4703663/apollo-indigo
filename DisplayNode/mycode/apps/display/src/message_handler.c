#include <string.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include "message_handler.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(message_handler, LOG_LEVEL_DBG);

const char *const topics[] = {
    [MQTT_MSG_MODE_TOPIC] = MODE_TOPIC,
    [MQTT_MSG_HEARBEAT_TOPIC] = HEARTBEAT_TOPIC,
    [MQTT_MSG_IMG_TOPIC] = IMG_TOPIC,
    [MQTT_MSG_ANIM_TOPIC] = ANIM_TOPIC,
    [MQTT_MSG_DIST_TOPIC] = DIST_TOPIC,
    [MQTT_MSG_ORIENT_TOPIC] = ORIENT_TOPIC
};
size_t topics_count = sizeof(topics) / sizeof(topics[0]);

static enum MQTTTopic decode_mqtt_topic(const char *topic)
{
    switch (topic[0]) {
    case 'm':
        if (strcmp(topic + 1, topics[MQTT_MSG_MODE_TOPIC] + 1) == 0) {
            return MQTT_MSG_MODE_TOPIC;
        }
        return MQTT_MSG_INVALID;
    case 'h':
        if (strcmp(topic + 1, topics[MQTT_MSG_HEARBEAT_TOPIC] + 1) == 0) {
            return MQTT_MSG_HEARBEAT_TOPIC;
        }
        return MQTT_MSG_INVALID;
    case 'i':
        if (strcmp(topic + 1, topics[MQTT_MSG_IMG_TOPIC] + 1) == 0) {
            return MQTT_MSG_IMG_TOPIC;
        }
        return MQTT_MSG_INVALID;
    case 'a':
        if (strcmp(topic + 1, topics[MQTT_MSG_ANIM_TOPIC] + 1) == 0) {
            return MQTT_MSG_ANIM_TOPIC;
        }
        return MQTT_MSG_INVALID;
    case 'd':
        if (strcmp(topic + 1, topics[MQTT_MSG_DIST_TOPIC] + 1) == 0) {
            return MQTT_MSG_DIST_TOPIC;
        }
        return MQTT_MSG_INVALID;
    case 'o':
        if (strcmp(topic + 1, topics[MQTT_MSG_ORIENT_TOPIC] + 1) == 0) {
            return MQTT_MSG_ORIENT_TOPIC;
        }
        return MQTT_MSG_INVALID;
    default:
        return MQTT_MSG_INVALID;
    }
}

int message_handler_init(void)
{
    int ret;

    ret = mqtt_messenger_init();
    if (ret != 0) {
        return ret;
    }

    ret = mqtt_messenger_subscribe(topics, topics_count);

    return ret;
}

int message_handler_receive(struct Msg *msg, k_timeout_t timeout)
{
    int ret;
    struct Message message;

    ret = mqtt_messenger_receive(&message, timeout);
    if (ret != 0) {
        return ret;
    }

    msg->topic = decode_mqtt_topic(message.topic);

    uint32_t data;
    uint8_t *p = message.buffer;

    switch (msg->topic) {
    case MQTT_MSG_MODE_TOPIC:
        if (*p > DISPLAY_MODE_DISTANCE) {
            LOG_ERR("Received invalid display mode: %d", *p);
            msg->topic = MQTT_MSG_INVALID;
            return 1;
        }
        msg->data.mode_topic_data.display_mode = *p;
        k_free(message.buffer);
        break;
    case MQTT_MSG_HEARBEAT_TOPIC:
        data = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3] << 0;
        msg->data.heartbeat_topic_data.tick_num = data;
        k_free(message.buffer);
        break;
    case MQTT_MSG_IMG_TOPIC:
        msg->data.image_topic_data.buffer = message.buffer;
        break;
    case MQTT_MSG_ANIM_TOPIC:
        msg->data.anim_topic_data.frame_num = *p;
        msg->data.anim_topic_data.buffer = p + 1;
        break;
    case MQTT_MSG_DIST_TOPIC:
        char *str = k_malloc(message.size + 1);
        memcpy(str, message.buffer, message.size);
        str[message.size] = '\0';
        msg->data.dist_topic_data.distance = strtof(str, NULL);
        k_free(str);
        break;
    case MQTT_MSG_ORIENT_TOPIC:
        if (p[0] == '1') {
            msg->data.orient_topic_data.orientation = ORIENTATION_DOWN;
        } else {
            msg->data.orient_topic_data.orientation = ORIENTATION_UP;
        }
        break;
    default:
        // break;
        LOG_ERR("Received MQTT message with unknown topic: %s", message.topic);
        return 1;
    }

    k_free(message.topic);

    return 0;
}

int message_handler_send(const struct Msg *msg)
{
    int ret;
    struct Message message;
    static char buf[16];

    switch (msg->topic) {
    case MQTT_MSG_DIST_TOPIC:
        snprintk(buf, 16, "%f", (double) msg->data.dist_topic_data.distance);
        message.topic = DIST_TOPIC;
        message.size = strlen(buf);
        break;
    case MQTT_MSG_ORIENT_TOPIC:
        if (msg->data.orient_topic_data.orientation == ORIENTATION_DOWN) {
            buf[0] = '1';
        } else {
            buf[0] = '0';
        }
        message.topic = ORIENT_TOPIC;
        message.size = 1;
        break;
    default:
        LOG_ERR("Only support sending distance and orientation messages");
        return 1;
    }

    message.buffer = buf;

    return mqtt_messenger_send(&message);

    // message.topic = topics[msg->topic];

    // switch (msg->topic) {
    // case MQTT_MSG_MODE_TOPIC:
    //     buf[0] = msg->data.mode_topic_data.display_mode;
    //     message.buffer = buf;
    //     message.size = 1;
    //     break;
    // // case MQTT_MSG_HEARBEAT_TOPIC:
    // //     // never gonna use this
    // //     return 1;
    // default:
    //     return 1;
    // }

    // return 1;
}
