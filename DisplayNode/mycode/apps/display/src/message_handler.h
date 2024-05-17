#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include "mqtt_messenger.h"
#include "display_mode.h"
#include "orientation_enum.h"

#define STR2(s) #s
#define STR(s) STR2(s)

#define MODE_TOPIC "mode"
#define HEARTBEAT_TOPIC "heartbeat"
#define IMG_TOPIC "img" STR(CONFIG_DISPLAY_DEVICE_NUMBER)
#define ANIM_TOPIC "anim" STR(CONFIG_DISPLAY_DEVICE_NUMBER)
#define DIST_TOPIC "distance"
#define ORIENT_TOPIC "orientation"

enum MQTTTopic {
    MQTT_MSG_MODE_TOPIC,
    MQTT_MSG_HEARBEAT_TOPIC,
    MQTT_MSG_IMG_TOPIC,
    MQTT_MSG_ANIM_TOPIC,
    MQTT_MSG_DIST_TOPIC,
    MQTT_MSG_ORIENT_TOPIC,
    MQTT_MSG_INVALID
};

struct ModeTopicData {
    enum DisplayMode display_mode;
};

struct HeartbeatTopicData {
    uint32_t tick_num;
};

struct ImageTopicData {
    const void *buffer;
};

struct AnimTopicData {
    int frame_num;
    const void *buffer;
};

struct DistTopicData {
    float distance;
};

struct OrientTopicData {
    enum Orientation orientation;
};

union MsgData {
    struct ModeTopicData mode_topic_data;
    struct HeartbeatTopicData heartbeat_topic_data;
    struct ImageTopicData image_topic_data;
    struct AnimTopicData anim_topic_data;
    struct DistTopicData dist_topic_data;
    struct OrientTopicData orient_topic_data;
};

struct Msg {
    enum MQTTTopic topic;
    union MsgData data;
};

int message_handler_init(void);

int message_handler_receive(struct Msg *msg, k_timeout_t timeout);

int message_handler_send(const struct Msg *msg);

#endif
