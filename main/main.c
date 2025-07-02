

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <Sensors/gps/gps.h>
#include <Sensors/elm327/elm327.h>
#include <Transmission/transmission.h>
#include <DataPacket.h>
#include <nvs_flash.h>
#include "Config.h"
// #include <esp_log.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"



DataPacket dataPacket;
SemaphoreHandle_t dataMutex;

void gps_task(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    DataPacket* data = (DataPacket *) pvParameters;
    while (1) {
        get_gps_data(data);
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000));
    }
}

void elm327_task(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    DataPacket* data = (DataPacket *) pvParameters;
    while(1) {
        get_elm327_data(data);
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(4000));
    }
}

void transmit_task(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    DataPacket* data = (DataPacket *) pvParameters;
    while(1) {
        if(xSemaphoreTake(dataMutex, portMAX_DELAY))
        {
            transmit_data(data);
            xSemaphoreGive(dataMutex);
        }
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(5000));
    }
}


esp_err_t init_sd_card(void) {
    esp_err_t ret;

    // 1. SPI bus configuration
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = VSPI_HOST;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = 23,
        .miso_io_num = 19,
        .sclk_io_num = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        printf("Failed to initialize SPI bus: %s\n", esp_err_to_name(ret));
        return ret;
    }

    // 2. SD card slot config
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = 5;         // Chip Select pin
    slot_config.host_id = host.slot;

    // 3. VFS mount configuration
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    // 4. Attempt to mount the card
    sdmmc_card_t *card;
    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    vTaskDelay(500 / portTICK_PERIOD_MS); // Add at the start of init_sd_card()


    if (ret == ESP_OK) {
        printf("SD card mounted successfully.\n");
        sdmmc_card_print_info(stdout, card);
    } else {
        printf("Failed to mount SD card: %s\n", esp_err_to_name(ret));
    }

    return ret;
}


void app_main() {

    printf("Initializing SD card...\n");

    if (init_sd_card() != ESP_OK) {
        printf("SD card initialization failed. Exiting...\n");
    }

    printf("SD card is ready to use.\n");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }


    dataMutex = xSemaphoreCreateMutex();
    

    setup_gps();
    setup_elm327();
    // setup_wifi();
    // connect();


    // xTaskCreate(gps_task, "gps_task", 4096, &dataPacket, 4, NULL);
    // xTaskCreate(elm327_task, "elm327_task", 6144, &dataPacket, 3, NULL);
    // xTaskCreate(transmit_task, "transmit_task", 4096, &dataPacket, 5, NULL);


    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("Starting Device:\n\n");

    xTaskCreatePinnedToCore(gps_task, "gps_task", 4096, &dataPacket, 5, NULL, 0);      // Core 0
    xTaskCreatePinnedToCore(elm327_task, "elm327_task", 6144, &dataPacket, 5, NULL, 1); // Core 1
    xTaskCreatePinnedToCore(transmit_task, "transmit_task", 4096, &dataPacket, 6, NULL, 1); // Core 1


    while(1)
    {
        printf("looping \n");
        vTaskDelay(20000 / portTICK_PERIOD_MS);
    }
}




