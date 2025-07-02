

#ifndef GPS_H
#define GPS_H

// Include Libraries
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>
#include <driver/uart.h>
#include <DataPacket.h>


// Pins
#define UART_NUM UART_NUM_1
#define GPS_TX_PIN GPIO_NUM_17 
#define GPS_RX_PIN GPIO_NUM_16
#define GPS_BAUD_RATE 115200

// Defaults
#define GPS_DEFAULT 999.9999f;

// Field Counts
#define GGA_FIELD_COUNT 15
#define ZDA_FIELD_COUNT 10

#undef pdMS_TO_TICKS
#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(((uint64_t)(xTimeInMs) * configTICK_RATE_HZ) / 1000U))


// Functions
void setup_gps();
void get_gps_data(DataPacket* data);

#endif