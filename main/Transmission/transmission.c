
#include "transmission.h"
#include <esp_timer.h>

esp_mqtt_client_handle_t client = NULL;
bool mqtt_connected = false;

int attempt = 10;

// TransmissionState networkStatus = NOT_CONNECTED;

// ===== WiFi Setup =====
void setup_wifi(void) {

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default Wi-Fi STA interface
    esp_netif_create_default_wifi_sta();

    // Initialize Wi-Fi driver
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("WiFi setup completed.\n");

}

// ===== MQTT Event Handler =====
static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            mqtt_connected = true;
            printf("MQTT CONNECTED\n");
            // networkStatus = CONNECTED;
            break;

        case MQTT_EVENT_DISCONNECTED:
            mqtt_connected = false;
            printf("MQTT DISCONNECTED\n");
            // networkStatus = NOT_CONNECTED;
            break;

        default:
            break;
    }
}





// ===== Connect =====
void connect(void) {
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);

    if (ret != ESP_OK) {
        wifi_config_t wifi_config = { 0 };
        strncpy((char*)wifi_config.sta.ssid, SSID, sizeof(wifi_config.sta.ssid));
        strncpy((char*)wifi_config.sta.password, PASSWORD, sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_connect());

        int tries = 0;
        while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK && tries < 20) {
            vTaskDelay(pdMS_TO_TICKS(500));
            tries++;
        }

        if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
            printf("Failed to connect to WiFi.\n");
            return;
        }
    }

    printf("Connected to WiFi: %s\n", (char*)ap_info.ssid);

    if (!mqtt_connected) {
        if (client == NULL) {
            esp_mqtt_client_config_t mqtt_cfg = {
                .broker.address.hostname = "broker.mqtt.cool",
                .broker.address.port = 1883,
                .broker.address.transport = MQTT_TRANSPORT_OVER_TCP,  // or MQTT_TRANSPORT_OVER_SSL for secure
            };

            client = esp_mqtt_client_init(&mqtt_cfg);
            ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, NULL));

            ESP_ERROR_CHECK(esp_mqtt_client_start(client));
        }
        else { esp_mqtt_client_start(client); }

        printf("MQTT client started.\n");
        
    } else {
        printf("MQTT already connected.\n");
    }
}



// ===== Data Transmission Only =====
void transmit_data(DataPacket* data) {
    char message[1024];

    data->uptime_seconds = esp_timer_get_time() / 1000000ULL;

    snprintf(message, sizeof(message),
        "{\"date\":\"%s\",\"time\":\"%s\",\"uptime\":%lu,"
        "\"gps\":{\"lat\":%.6f,\"lon\":%.6f},"
        "\"obd\":{\"rpm\":%d,\"speed\":%d,\"coolant_temp\":%d,"
        "\"throttle_pos\":%d,\"mass_air_flow\":%.2f,\"absolute_engine_load\":%d}}",
        data->date,
        data->time,
        data->uptime_seconds,
        data->gps.lat,
        data->gps.lon,
        data->obd.rpm,
        data->obd.speed,
        data->obd.coolantTemp,
        data->obd.throttlePos,
        data->obd.massAirFlow,
        data->obd.absoluteEngineLoad
    );

    // Save to SD Card
    FILE* f = fopen("/sdcard/data.txt", "a+");
    if (f == NULL) {
        printf("Failed to open file for appending\n");
    } else {
        fprintf(f, "%s\n", message);
        fclose(f);
    }

    printf("message published: %s\n", message);

    // Optionally publish via MQTT
    // int msg_id = esp_mqtt_client_publish(client, TOPIC, message, 0, 1, 0);
    // printf("MQTT message published (msg_id=%d): %s\n", msg_id, message);
}


