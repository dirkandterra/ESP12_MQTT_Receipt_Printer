#include "printer.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "printer";

#define UART_NUM    CONFIG_PRINTER_UART_NUM
#define BAUD        CONFIG_PRINTER_BAUD_RATE

static void send(const uint8_t *buf, size_t len)
{
    uart_write_bytes(UART_NUM, (const char *)buf, len);
}

void printer_init(void)
{
    uart_config_t cfg = {
        .baud_rate  = BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &cfg));
    /* ESP8266 UART pins are hardware-fixed: UART1 TX is GPIO2, no software remap needed */
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, 0, 256, 0, NULL, 0));

    printer_reset();
    ESP_LOGI(TAG, "ready on UART%d (GPIO2 TX) %d baud", UART_NUM, BAUD);
}

void printer_reset(void)
{
    const uint8_t cmd[] = {0x1B, 0x40}; /* ESC @ */
    send(cmd, sizeof(cmd));
}

void printer_write(const char *data, size_t len)
{
    uart_write_bytes(UART_NUM, data, len);
}

void printer_println(const char *text)
{
    uart_write_bytes(UART_NUM, text, strlen(text));
    uart_write_bytes(UART_NUM, "\n", 1);
}

void printer_set_align(printer_align_t align)
{
    const uint8_t cmd[] = {0x1B, 0x61, (uint8_t)align}; /* ESC a n */
    send(cmd, sizeof(cmd));
}

void printer_set_bold(bool bold)
{
    const uint8_t cmd[] = {0x1B, 0x45, bold ? 0x01 : 0x00}; /* ESC E n */
    send(cmd, sizeof(cmd));
}

void printer_set_size(printer_size_t size)
{
    if (size == PRINTER_SIZE_SMALL) {
        const uint8_t cmd[] = {0x1B, 0x4D, 0x01}; /* ESC M 1 — Font B */
        send(cmd, sizeof(cmd));
    } else {
        const uint8_t font_a[] = {0x1B, 0x4D, 0x00}; /* ESC M 0 — Font A */
        send(font_a, sizeof(font_a));
        const uint8_t cmd[] = {0x1D, 0x21, (uint8_t)size}; /* GS ! n */
        send(cmd, sizeof(cmd));
    }
}

void printer_set_underline(uint8_t weight)
{
    const uint8_t cmd[] = {0x1B, 0x2D, weight}; /* ESC - n */
    send(cmd, sizeof(cmd));
}

void printer_feed(uint8_t lines)
{
    const uint8_t cmd[] = {0x1B, 0x64, lines}; /* ESC d n */
    send(cmd, sizeof(cmd));
}

void printer_cut(void)
{
    printer_feed(4);
    const uint8_t cmd[] = {0x1D, 0x56, 0x41, 0x03}; /* GS V A 3 — partial cut */
    send(cmd, sizeof(cmd));
}
