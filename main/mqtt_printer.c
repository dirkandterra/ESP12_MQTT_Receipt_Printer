#include "mqtt_printer.h"
#include "printer.h"
#include "secret.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mqtt";

#define MAX_MSG 512

static esp_mqtt_client_handle_t s_client;

extern const char *mqtt_topic;   /* defined in main.c */

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "connected to broker, subscribing to %s", mqtt_topic);
        esp_mqtt_client_subscribe(s_client, mqtt_topic, 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "disconnected from broker");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "subscribed (msg_id=%d)", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        if (event->data_len > 0 && event->data_len <= MAX_MSG) {
            char buf[MAX_MSG + 1];
            memcpy(buf, event->data, event->data_len);
            buf[event->data_len] = '\0';
            ESP_LOGI(TAG, "rx %d bytes on [%.*s]",
                     event->data_len, event->topic_len, event->topic);
            printer_set_size(PRINTER_SIZE_SMALL);
            printer_println(buf);
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error");
        break;

    default:
        break;
    }
    return ESP_OK;
}

void mqtt_publish(const char *topic, const char *payload)
{
    esp_mqtt_client_publish(s_client, topic, payload, 0, 0, 0);
}

void mqtt_printer_start(void)
{
    esp_mqtt_client_config_t cfg = {
        .event_handle = mqtt_event_handler,
        .host         = MQTT_SERVER,
        .port         = 1883,
        .username     = MOSQUITTO_USR,
        .password     = MOSQUITTO_PWD,
    };
    s_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_start(s_client);
    ESP_LOGI(TAG, "MQTT client started → %s:1883", MQTT_SERVER);
}
