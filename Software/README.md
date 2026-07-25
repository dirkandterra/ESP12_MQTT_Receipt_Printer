# ESP-12 MQTT Receipt Printer

Controls a generic 58mm ESC/POS thermal printer over UART from an ESP-12 (ESP8266) module. Subscribes to an MQTT topic and prints any message received.

## Hardware

| Signal | ESP-12 pin | Notes |
|--------|-----------|-------|
| UART TX | GPIO2 (UART1) | Connect to printer RX |
| GND | GND | Common ground with printer |

UART1 on ESP8266 is TX-only (GPIO2), which is sufficient — ESC/POS printers do not require RX for normal printing.

Printer power: most 58mm printers need 5–9 V at up to 2 A. Do **not** power from the ESP module.

## Configuration

Copy `main/secret.h.example` to `main/secret.h` and fill in your credentials:

```c
#define WIFI_SSID      "your_ssid"
#define WIFI_PWD       "your_password"
#define MQTT_SERVER    "your.broker.ip"
#define MOSQUITTO_USR  "username"
#define MOSQUITTO_PWD  "password"
```

The MQTT topic is defined in `main/main.c`:
```c
const char *mqtt_topic = "ESP12_Receipt/print";
```

Printer settings (UART port, baud rate) are in `idf.py menuconfig` → Receipt Printer.

## Build

Requires [ESP8266_RTOS_SDK](https://github.com/espressif/ESP8266_RTOS_SDK) v3.4 or later.

```bash
export IDF_PATH=/path/to/ESP8266_RTOS_SDK
idf.py build
idf.py -p /dev/ttyUSB0 flash
```

## Behaviour

- On boot, the IP address is printed on the receipt paper.
- Any message published to the MQTT topic is printed in Font B (condensed).
- WiFi: retries 5 times on disconnect, then pauses 5 minutes before trying again.
- MQTT: auto-reconnects indefinitely; re-subscribes on each reconnect.

## Project structure

```
main/
  main.c            WiFi init, MQTT topic, app entry
  printer.c/.h      ESC/POS UART driver
  mqtt_printer.c/.h MQTT subscriber → printer
  secret.h          WiFi & MQTT credentials (not in repo)
```
