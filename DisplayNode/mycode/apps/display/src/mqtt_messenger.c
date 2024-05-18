#include <stdalign.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/random/random.h>
#include <zephyr/net/wifi_mgmt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mqtt_messenger, LOG_LEVEL_DBG);

#include "mqtt_messenger.h"

#define SERVER_PORT 1883
#define MQTT_TX_BUFFER_SIZE 128
// #define MQTT_RX_BUFFER_SIZE 262114 // 256 KB
// #define MQTT_RX_BUFFER_SIZE 128 // 256 KB
#define MQTT_RX_BUFFER_SIZE 16384 // 256 KB
#define APP_CONNECT_TIMEOUT_MS 2000
#define APP_SLEEP_MSECS 500

#define HEARTBEAT_THREAD_STACK_SIZE 2048
#define HEARTBEAT_THREAD_PRIORITY 4
#define HEARTBEAT_SLEEP_SEC 15

#define RECEIVE_THREAD_STACK_SIZE 2048
#define RECEIVE_THREAD_PRIORITY 5
#define RECEIVE_POLL_TIMEOUT_MS 1000


#define RC_STR(rc) ((rc) == 0 ? "OK" : "ERROR")
#define PRINT_RESULT(func, rc) LOG_INF("%s: %d <%s>", (func), rc, RC_STR(rc))

// The mqtt client struct
static struct mqtt_client client;

// MQTT broker details
static struct sockaddr_storage broker;

// Buffers for MQTT client
// static uint8_t rx_buffer[APP_MQTT_BUFFER_SIZE];
// static uint8_t tx_buffer[APP_MQTT_BUFFER_SIZE];
static uint8_t rx_buffer[MQTT_RX_BUFFER_SIZE];
static uint8_t tx_buffer[MQTT_TX_BUFFER_SIZE];

// static uint8_t *rx_buffer = NULL;
// static uint8_t *tx_buffer = NULL;

// Buffer for application:
// static char payload_buffer[APP_MQTT_BUFFER_SIZE];

static volatile bool connected;

static struct zsock_pollfd fds[1] = {};
static int nfds = 0;

// For wifi connection callback
struct net_mgmt_event_callback wifi_cb;

// Periodically send MQTT heartbeat
static void mqtt_heartbeat_thread(void *, void *, void *);
static void mqtt_receive_thread(void *, void *, void *);

K_THREAD_DEFINE(mqtt_heartbeat_tid, HEARTBEAT_THREAD_STACK_SIZE, mqtt_heartbeat_thread, NULL, NULL, NULL,
        HEARTBEAT_THREAD_PRIORITY, 0, 0);

K_THREAD_DEFINE(mqtt_receive_tid, RECEIVE_THREAD_STACK_SIZE, mqtt_receive_thread, NULL, NULL, NULL,
        RECEIVE_THREAD_PRIORITY, 0, 0);

// Queue for incoming messages
K_MSGQ_DEFINE(receive_msgq, sizeof(struct Message), 4, alignof(struct Message));
// Queue for outgoing messages
K_MSGQ_DEFINE(send_msgq, sizeof(struct Message), 4, alignof(struct Message));

K_SEM_DEFINE(connected_sem, 0, 1);

// static int wait(int timeout)
// {
//     int ret = 0;

//     if (nfds > 0) {
//         ret = zsock_poll(fds, nfds, timeout);
//         if (ret < 0) {
//             LOG_ERR("poll error: %d", errno);
//         }
//     }

//     return ret;
// }

static void mqtt_heartbeat_thread(void *, void *, void *)
{
    int ret;

    for (;;) {
        if (connected) {
            ret = mqtt_live(&client);
            if (ret != 0 && ret != -EAGAIN) {
                PRINT_RESULT("mqtt_live", ret);
                // return ret;
            }
        }
        k_sleep(K_SECONDS(HEARTBEAT_SLEEP_SEC));
    }
}

static void mqtt_receive_thread(void *, void *, void *)
{
    int ret;
    
    for (;;) {
        if (connected) {
            // negative timeout means wait forever
            // Returns -1 on error, or no. of events occurred
            ret = zsock_poll(fds, nfds, RECEIVE_POLL_TIMEOUT_MS);
            if (ret < 0) {
                LOG_ERR("poll error: %d", errno);
                // return ret;
            // } else if (ret == 0) {
            //     continue;
            }
            ret = mqtt_input(&client);
            if (ret != 0) {
                PRINT_RESULT("mqtt_input", ret);
                // return ret;
            }
            // ret = mqtt_input(&client);
            // if (ret != 0) {
            //     PRINT_RESULT("mqtt_input", ret);
            //     // return ret;
            // }
            // k_msleep(10);
        } else {
            k_msleep(RECEIVE_POLL_TIMEOUT_MS);
        }
    }
}

static void prepare_fds(void)
{
    fds[0].fd = client.transport.tcp.sock;
    fds[0].events = ZSOCK_POLLIN;
    nfds = 1;
}

static void clear_fds(void)
{
    nfds = 0;
}

static void mqtt_evt_handler(struct mqtt_client *const client, const struct mqtt_evt *evt)
{
    int err;

    switch (evt->type) {
    case MQTT_EVT_CONNACK:
        if (evt->result != 0) {
            LOG_ERR("MQTT connect failed %d", evt->result);
            break;
        }

        connected = true;
        LOG_INF("MQTT client connected!");
        k_sem_give(&connected_sem);

        break;

    case MQTT_EVT_DISCONNECT:
        LOG_INF("MQTT client disconnected %d", evt->result);

        connected = false;
        clear_fds();

        break;

    case MQTT_EVT_PUBLISH:
        if (evt->result != 0) {
            LOG_ERR("MQTT PUBLISH error %d", evt->result);
            break;
        }

        printk("Inside MQTT_EVT_PUBLISH\n");

        // LOG_INF("PUBLISH packet id: %u", evt->param.publish.message_id);

        // evt->param.publish.message.payload.len
        size_t topic_size = evt->param.publish.message.topic.topic.size;
        size_t payload_size = evt->param.publish.message.payload.len;
        // printk("PUBLISH payload length: %u\n", payload_size);
        
        struct Message message = {
            .topic = k_malloc(topic_size + 1),
            .buffer = k_malloc(payload_size),
            .size = payload_size
        };

        if (message.topic == NULL || message.buffer == NULL) {
            LOG_ERR("Failed to allocate enough memory for published message");
            k_free(message.topic);
            k_free(message.buffer);
            break;
        }

        memcpy(message.topic, evt->param.publish.message.topic.topic.utf8, topic_size);
        message.topic[topic_size] = '\0';

        LOG_INF("PUBLISH packet id: %u, payload length: %u, topic: %s", evt->param.publish.message_id,
                payload_size, message.topic);
        printk("Before readall\n");
        err = mqtt_readall_publish_payload(client, message.buffer, payload_size);
        if (err < 0) {
            LOG_ERR("Failed to read publish payload: %d", err);
        }
        printk("After readall\n");

        // LOG_INF("PUBLISH payload read length: %u", err);
        // printk("PUBLISH payload read length: %u\n", err);

        if (evt->param.publish.message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
            struct mqtt_puback_param puback_param = {
                .message_id = evt->param.publish.message_id
            };
            err = mqtt_publish_qos1_ack(client, &puback_param);
            if (err != 0) {
                LOG_ERR("Failed to send MQTT PUBACK: %d", err);
            }
        } else if (evt->param.publish.message.topic.qos == MQTT_QOS_2_EXACTLY_ONCE) {
            struct mqtt_pubrec_param pubrec_param = {
                .message_id = evt->param.publish.message_id
            };
            err = mqtt_publish_qos2_receive(client, &pubrec_param);
            if (err != 0) {
                LOG_ERR("Failed to send MQTT PUBREC: %d", err);
            }
        }

        printk("Before msgq_put\n");
        err = k_msgq_put(&receive_msgq, &message, K_NO_WAIT);
        if (err != 0) {
            k_free(message.topic);
            k_free(message.buffer);
            LOG_ERR("Failed to add received MQTT message to queue");
            break;
        }
        printk("After msgq_put\n");
        
        // LOG_INF("Received message from topic %s:\n%s\n", evt->param.publish.message.topic.topic.utf8,
        //         payload_buffer);
        
        break;

    case MQTT_EVT_PUBACK:
        if (evt->result != 0) {
            LOG_ERR("MQTT PUBACK error %d", evt->result);
            break;
        }

        LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);

        break;

    case MQTT_EVT_PUBREC:
        if (evt->result != 0) {
            LOG_ERR("MQTT PUBREC error %d", evt->result);
            break;
        }

        LOG_INF("PUBREC packet id: %u", evt->param.pubrec.message_id);

        const struct mqtt_pubrel_param rel_param = {
            .message_id = evt->param.pubrec.message_id
        };

        err = mqtt_publish_qos2_release(client, &rel_param);
        if (err != 0) {
            LOG_ERR("Failed to send MQTT PUBREL: %d", err);
        }

        break;

    case MQTT_EVT_PUBREL:
        if (evt->result != 0) {
            LOG_ERR("MQTT PUBREL error %d", evt->result);
            break;
        }

        LOG_INF("PUBREL packet id: %u", evt->param.pubrel.message_id);

        const struct mqtt_pubcomp_param comp_param = {
            .message_id = evt->param.pubrel.message_id
        };

        err = mqtt_publish_qos2_complete(client, &comp_param);
        if (err != 0) {
            LOG_ERR("Failed to send MQTT PUBCOMP: %d", err);
        }

        break;

    case MQTT_EVT_PUBCOMP:
        if (evt->result != 0) {
            LOG_ERR("MQTT PUBCOMP error %d", evt->result);
            break;
        }

        LOG_INF("PUBCOMP packet id: %u",
            evt->param.pubcomp.message_id);

        break;

    case MQTT_EVT_SUBACK:
        if (evt->result != 0) {
            LOG_ERR("MQTT SUBACK error %d", evt->result);
            break;
        }

        LOG_INF("SUBACK packet id: %u", evt->param.suback.message_id);

        break;

    case MQTT_EVT_UNSUBACK:
        if (evt->result != 0) {
            LOG_ERR("MQTT UNSUBACK error %d", evt->result);
            break;
        }

        LOG_INF("UNSUBACK packet id: %u", evt->param.unsuback.message_id);

        break;

    case MQTT_EVT_PINGRESP:
        LOG_INF("PINGRESP packet");
        break;

    default:
        break;
    }
}

static int broker_init(void)
{
    int ret;

    struct sockaddr_in *broker4 = (struct sockaddr_in *) &broker;

    broker4->sin_family = AF_INET;
    broker4->sin_port = htons(SERVER_PORT);
    // returns 1 on success
    ret = zsock_inet_pton(AF_INET, CONFIG_MQTT_BROKER_ADDRESS, &broker4->sin_addr);
    if (ret != 1) {
        LOG_ERR("Failed to process MQTT broker address: \"%s\"", CONFIG_MQTT_BROKER_ADDRESS);
        return ret;
    }

    return 0;
}

static int client_init(void)
{
    int ret;

    mqtt_client_init(&client);
    ret = broker_init();
    if (ret) {
        return ret;
    }

    // MQTT client configuration
    client.broker = &broker;
    client.evt_cb = mqtt_evt_handler;
    client.client_id.utf8 = (uint8_t *) CONFIG_MQTT_CLIENT_ID;
    client.client_id.size = strlen(CONFIG_MQTT_CLIENT_ID);
    client.password = NULL;
    client.user_name = NULL;
    client.protocol_version = MQTT_VERSION_3_1_1;

    // MQTT buffers configuration
    client.rx_buf = rx_buffer;
    client.rx_buf_size = MQTT_RX_BUFFER_SIZE; // sizeof(rx_buffer);
    client.tx_buf = tx_buffer;
    client.tx_buf_size = MQTT_TX_BUFFER_SIZE; // sizeof(tx_buffer);

    client.transport.type = MQTT_TRANSPORT_NON_SECURE;

    return 0;
}

static int wait(int timeout)
{
    int ret = 0;

    if (nfds > 0) {
        ret = zsock_poll(fds, nfds, timeout);
        if (ret < 0) {
            LOG_ERR("poll error: %d", errno);
        }
    }

    return ret;
}

static int start_mqtt(void)
{
    int ret;

    do {
        ret = client_init();
        if (ret) {
            return ret;
        }
        ret = mqtt_connect(&client);
        if (ret) {
            PRINT_RESULT("mqtt_connect", ret);
            k_msleep(APP_SLEEP_MSECS);
            continue;
        }

        prepare_fds();

        if (wait(APP_CONNECT_TIMEOUT_MS)) {
            mqtt_input(&client);
        }

        if (!connected) {
            mqtt_abort(&client);
        }

    } while (!connected);

    return 0;
}

static void wifi_connect_cb(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
    printk("Inside callback\n");
    if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
    // if (mgmt_event != NET_EVENT_L4_CONNECTED) {
        return;
    }
    // WiFi connected, initialize MQTT client here
    LOG_INF("Wifi connected");
    start_mqtt();
}

int mqtt_messenger_init(void)
{
    // rx_buffer = k_malloc(MQTT_RX_BUFFER_SIZE);
    // if (rx_buffer == NULL) {
    //     LOG_ERR("Failed to allocate enough space for RX buffer for MQTT");
    //     return 1;
    // }
    // tx_buffer = k_malloc(MQTT_TX_BUFFER_SIZE);
    // if (tx_buffer == NULL) {
    //     LOG_ERR("Failed to allocate enough space for TX buffer for MQTT");
    //     return 1;
    // }
    // Connect to WiFi
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params params = {
        .security = WIFI_SECURITY_TYPE_PSK,
        .ssid = CONFIG_WIFI_SSID,
        .ssid_length = strlen(CONFIG_WIFI_SSID),
        .psk = CONFIG_WIFI_PSK,
        .psk_length = strlen(CONFIG_WIFI_PSK)
    };
    while (1) {
        int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, (void *)&params, sizeof(params));
        if (ret == 0) {
            break;
        }

        LOG_INF("Connect request failed %d. Waiting iface be up...", ret);
        k_msleep(500);
    }

    // net_mgmt_init_event_callback(&wifi_cb, wifi_connect_cb, NET_EVENT_L4_CONNECTED);
    net_mgmt_init_event_callback(&wifi_cb, wifi_connect_cb, NET_EVENT_IPV4_ADDR_ADD);
    // net_mgmt_init_event_callback(&wifi_cb, wifi_connect_cb, NET_EVENT_WIFI_CMD_CONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);

    for (;;) {
        int ret = k_sem_take(&connected_sem, K_MSEC(2000));
        if (ret == 0) {
            break;
        }
        LOG_INF("MQTT Connecting...");
    }

    LOG_INF("MQTT Connected.");

    return 0;
}

int mqtt_messenger_subscribe(const char *const topics[], size_t topics_count)
{
    // // subscrite to topics here
    // struct mqtt_topic topic = {
    //     .topic = {
    //         .utf8 = "test_topic",
    //         .size = strlen("test_topic")
    //     },
    //     .qos = MQTT_QOS_2_EXACTLY_ONCE
    // };
    // struct mqtt_subscription_list subscription_list = {
    //     .list = &topic,
    //     .list_count = 1,
    //     .message_id = sys_rand32_get()
    // };
    // mqtt_subscribe(client, &subscription_list);

    struct mqtt_topic mqtt_topics[topics_count];

    for (size_t i = 0; i < topics_count; ++i) {
        mqtt_topics[i] = (struct mqtt_topic) {
            .topic = {
                .utf8 = (uint8_t *) topics[i], // no need to copy string
                .size = strlen(topics[i])
            },
            // .qos = MQTT_QOS_2_EXACTLY_ONCE
            .qos = MQTT_QOS_0_AT_MOST_ONCE
        };
    }
    struct mqtt_subscription_list subscription_list = {
        .list = mqtt_topics,
        .list_count = topics_count,
        .message_id = sys_rand32_get()
    };

    return mqtt_subscribe(&client, &subscription_list);
}

int mqtt_messenger_receive(struct Message *message, k_timeout_t timeout)
{
    return k_msgq_get(&receive_msgq, message, timeout);
}

int mqtt_messenger_send(const struct Message *message)
{
    // return k_msgq_put(&send_msgq, message, timeout);
    struct mqtt_publish_param param = {
        .message = {
            .topic = {
                // .qos = MQTT_QOS_2_EXACTLY_ONCE,
                .qos = MQTT_QOS_0_AT_MOST_ONCE,
                .topic = {
                    .utf8 = (uint8_t *) message->topic,
                    .size = strlen(message->topic)
                }
            },
            .payload = {
                .data = message->buffer,
                .len = message->size
            }
        },
        .message_id = sys_rand32_get(),
        .dup_flag = 0U,
        .retain_flag = 0U
    };

    return mqtt_publish(&client, &param);
    return 0;
}
