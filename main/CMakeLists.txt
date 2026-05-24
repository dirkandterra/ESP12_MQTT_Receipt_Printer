idf_component_register(
    SRCS
        "main.c"
        "printer.c"
        "mqtt_printer.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp8266
        esp_event
        tcpip_adapter
        mqtt
        pthread
)
