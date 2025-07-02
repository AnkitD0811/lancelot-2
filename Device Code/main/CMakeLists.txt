idf_component_register(
    SRCS 
        "main.c"
        "Sensors/gps/gps.c"
        "Sensors/elm327/elm327.c"
        "Transmission/transmission.c"

    INCLUDE_DIRS 
        "."
        "Sensors/gps"
        "Sensors/elm327"
        "Transmission"

    REQUIRES 
        esp_wifi
        esp_netif
        esp_event
        mqtt
        bt
        driver
        nvs_flash
        json
        fatfs
)
