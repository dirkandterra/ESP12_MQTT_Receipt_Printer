#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "driver/adc.h"

#include "secret.h"
#include "printer.h"
#include "mqtt_printer.h"

static const char *TAG = "receipt";

const char *mqtt_topic = "ESP12_Receipt/print";

static EventGroupHandle_t wifi_event_group;

static int retry_count;
static float batteryVoltage;
static TimerHandle_t s_retry_timer;
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_MAX_RETRY      5
#define WIFI_PAUSE_MS       (5 * 60 * 1000)
#define VOLTAGE_DIVIDER_RATIO (7.48/(74.7+7.48))  // R1=75k, R2=7.5k

/* ---------------------------------------------------------- wifi callbacks */

static void wifi_retry_timer_cb(TimerHandle_t t)
{
    retry_count = 0;
    ESP_LOGI(TAG, "WiFi retry pause over, restarting reconnect cycle");
    esp_wifi_connect();
}

static esp_err_t on_wifi_event(void *ctx, system_event_t *ev)
{
    switch (ev->event_id) {

    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP: {
        char ip_str[32];
        snprintf(ip_str, sizeof(ip_str), "IP: " IPSTR "\n",
                 IP2STR(&ev->event_info.got_ip.ip_info.ip));
        ESP_LOGI(TAG, "%s", ip_str);
        printer_println(ip_str);
        retry_count = 0;
        xTimerStop(s_retry_timer, 0);
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    }

    case SYSTEM_EVENT_STA_DISCONNECTED:
        if (retry_count < WIFI_MAX_RETRY) {
            retry_count++;
            ESP_LOGW(TAG, "WiFi disconnected, reconnecting (%d/%d)...",
                     retry_count, WIFI_MAX_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "WiFi failed after %d attempts, pausing 5 min", WIFI_MAX_RETRY);
            xTimerStart(s_retry_timer, 0);
        }
        break;

    default:
        break;
    }
    return ESP_OK;
}

/* ---------------------------------------------------------- wifi init */

static void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();
    s_retry_timer = xTimerCreate("wifi_retry", pdMS_TO_TICKS(WIFI_PAUSE_MS),
                                 pdFALSE, NULL, wifi_retry_timer_cb);

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(on_wifi_event, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PWD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi connected to \"%s\"", WIFI_SSID);
}

/*----------------------------------------------------------- read battery level */
static void battery_timer_cb(TimerHandle_t t)
{
    uint16_t raw;
    if(adc_read(&raw) != ESP_OK) {
        ESP_LOGE(TAG, "ADC read failed");
        return;
    }
    batteryVoltage = (float)raw / 1023.0f / VOLTAGE_DIVIDER_RATIO;
    int v_whole = (int)batteryVoltage;
    int v_frac  = (int)((batteryVoltage - v_whole) * 100);
    ESP_LOGI(TAG, "Battery voltage: %d.%02dV (raw=%u)", v_whole, v_frac, raw);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d.%02dV", v_whole, v_frac);
    mqtt_publish("ESP12_Receipt/BattVoltage", buf);
}

/* ---------------------------------------------------------- entry point */

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    printer_init();
    wifi_init_sta();

    adc_config_t adc_cfg = {
        .mode = ADC_READ_TOUT_MODE,
        .clk_div = 8,
    };
    if(adc_init(&adc_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "ADC init failed");
        return;
    }
    TimerHandle_t battery_timer = xTimerCreate("battery", pdMS_TO_TICKS(60 * 1000), pdTRUE, NULL, battery_timer_cb);
    xTimerStart(battery_timer, 0);

    mqtt_printer_start();
    ESP_LOGI(TAG, "Receipt printer ready, listening on %s", mqtt_topic);
}
