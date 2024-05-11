#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
// #include <zephyr/random/random.h>
#include <zephyr/net/wifi_mgmt.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mqtt_messenger, LOG_LEVEL_DBG);

#include "mqtt_messenger.h"

#define SERVER_PORT 1883
#define APP_MQTT_BUFFER_SIZE 128
#define APP_CONNECT_TIMEOUT_MS 2000
#define APP_SLEEP_MSECS 500

#define RC_STR(rc) ((rc) == 0 ? "OK" : "ERROR")
#define PRINT_RESULT(func, rc) LOG_INF("%s: %d <%s>", (func), rc, RC_STR(rc))

// The mqtt client struct
static struct mqtt_client client;

// MQTT broker details
static struct sockaddr_storage broker;

// Buffers for MQTT client
static uint8_t rx_buffer[APP_MQTT_BUFFER_SIZE];
static uint8_t tx_buffer[APP_MQTT_BUFFER_SIZE];

static volatile bool connected;

static struct zsock_pollfd fds[1];
static int nfds;

// For wifi connection callback
struct net_mgmt_event_callback wifi_cb;

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

        break;

    case MQTT_EVT_DISCONNECT:
        LOG_INF("MQTT client disconnected %d", evt->result);

        connected = false;
        clear_fds();

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

    case MQTT_EVT_PUBCOMP:
        if (evt->result != 0) {
            LOG_ERR("MQTT PUBCOMP error %d", evt->result);
            break;
        }

        LOG_INF("PUBCOMP packet id: %u",
            evt->param.pubcomp.message_id);

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
    client.rx_buf_size = sizeof(rx_buffer);
    client.tx_buf = tx_buffer;
    client.tx_buf_size = sizeof(tx_buffer);

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

    return 0;
}
