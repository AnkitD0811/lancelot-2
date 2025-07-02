

#ifndef TRANSMISSION_H
#define TRANSMISSION_H

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_err.h"
#include "mqtt_client.h"
#include <DataPacket.h>
#include <cJSON.h>
    #include<stdio.h>

#define SSID "YmatsWifi"
#define PASSWORD "yash5678"
#define BROKER "mqtt://broker.mqtt.cool"
#define PORT 1883
#define TOPIC "esp32/yash/test"

#undef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(((uint64_t)(xTimeInMs) * configTICK_RATE_HZ) / 1000U))

// extern esp_mqtt_client_handle_t client;
// extern bool mqtt_connected;
// typedef enum  {
//     CONNECTED,
//     NOT_CONNECTED
// } TransmissionState;

// extern TransmissionState networkStatus;


typedef enum {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTED
} wifi_status_t;


// Functions
void setup_wifi();
void connect();
void transmit_data(DataPacket *data);


#endif