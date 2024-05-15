#ifndef MQTT_MESSENGER_H
#define MQTT_MESSENGER_H

struct Message {
    char *topic;
    void *buffer;
    size_t size;
};

int mqtt_messenger_init(void);

int mqtt_messenger_subscribe(char **topics, size_t topics_count);

int mqtt_messenger_receive(struct Message *message, k_timeout_t timeout);

int mqtt_messenger_send(const struct Message *message);

#endif
