#ifndef ELM327_H
#define ELM327_H

#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "esp_bt_defs.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include <ctype.h>
#include "DataPacket.h"


// PID
typedef struct {
    const char *pid_cmd;
    uint8_t pid_code;
} PIDCommand;


// Functions
void setup_elm327();
void get_elm327_data(DataPacket* data);

#endif // ELM327_H
