// elm327.c

#include "elm327.h"

#define OBD_RESPONSE_MAX_LEN 128
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static uint32_t handle = 0;
static bool connected = false;
static char latest_obd_response[OBD_RESPONSE_MAX_LEN] = {0};
static bool response_ready = false;

// Discovery state
static bool discovery_started = false;
static bool elm327_found = false;
static uint8_t discovered_elm327_addr[6];

#define TARGET_BT_NAME "OBDII"  // Or "ELM327" or whatever your device is called
// #define TARGET_BT_NAME "Yash's M14"  // phone
// #define TARGET_BT_NAME "yashmathur-IdeaPad-3-15IIL05"


void elm327_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
        case ESP_BT_GAP_DISC_RES_EVT: {
            esp_bd_addr_t bda;
            memcpy(bda, param->disc_res.bda, sizeof(esp_bd_addr_t));

            char bda_str[18];
            snprintf(bda_str, sizeof(bda_str),
                     "%02x:%02x:%02x:%02x:%02x:%02x",
                     bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

            printf("Found BT device: %s\n", bda_str);

            // Request device name — triggers ESP_BT_GAP_READ_REMOTE_NAME_EVT
            esp_bt_gap_read_remote_name(bda);
            break;
        }

        case ESP_BT_GAP_READ_REMOTE_NAME_EVT: {
            if (param->read_rmt_name.stat == ESP_BT_STATUS_SUCCESS) {
                char *dev_name = (char *)param->read_rmt_name.rmt_name;

                char bda_str[18];
                snprintf(bda_str, sizeof(bda_str),
                         "%02x:%02x:%02x:%02x:%02x:%02x",
                         param->read_rmt_name.bda[0], param->read_rmt_name.bda[1],
                         param->read_rmt_name.bda[2], param->read_rmt_name.bda[3],
                         param->read_rmt_name.bda[4], param->read_rmt_name.bda[5]);

                printf("Remote device name: %s [%s]\n", dev_name, bda_str);

                if (strcmp(dev_name, TARGET_BT_NAME) == 0 && !elm327_found) {
                    printf("Target ELM327 matched by name. Saving address...\n");
                    memcpy(discovered_elm327_addr, param->read_rmt_name.bda, 6);
                    elm327_found = true;
                    esp_bt_gap_cancel_discovery(); // Will trigger DISC_STATE_CHANGED_EVT
                }
            } else {
                printf("Failed to get remote name.\n");
            }
            break;
        }

        case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
            if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
                if (elm327_found) {
                    printf("Discovery complete. Requesting remote services...\n");
                    esp_spp_start_discovery(discovered_elm327_addr);  // 🚀 Dynamic SCN discovery
                } else {
                    printf("Discovery complete. No matching device found.\n");
                }
            }
            break;
        }

        default:
            break;
    }
}



// SPP Callback
void spp_cb(esp_spp_cb_event_t e, esp_spp_cb_param_t *p) {
    switch (e) {
        case ESP_SPP_INIT_EVT:
            printf("SPP Init done, starting discovery...\n");
            if (!discovery_started) {
                discovery_started = true;
                esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
            }
            break;
        case ESP_SPP_OPEN_EVT:
            printf("SPP connection established!\n");
            handle = p->open.handle;
            connected = true;
            vTaskDelay(pdMS_TO_TICKS(1500));
            const char* init_cmds[] = {"ATZ\r", "ATE0\r", "ATL0\r", "ATS0\r"};
            for (int i = 0; i < 4; i++) {
                esp_spp_write(handle, strlen(init_cmds[i]), (uint8_t *)init_cmds[i]);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            break;
        case ESP_SPP_DATA_IND_EVT:
            memset(latest_obd_response, 0, sizeof(latest_obd_response));
            memcpy(latest_obd_response, p->data_ind.data, MIN(p->data_ind.len, OBD_RESPONSE_MAX_LEN - 1));
            latest_obd_response[MIN(p->data_ind.len, OBD_RESPONSE_MAX_LEN - 1)] = '\0';
            response_ready = true;
            // printf("OBD Response: %s\n", latest_obd_response);
            break;
        case ESP_SPP_CLOSE_EVT:
            printf("SPP connection closed.\n");
            connected = false;
            break;
        case ESP_SPP_DISCOVERY_COMP_EVT:
            printf("SPP service discovery complete. Found SCN: %d\n", *(p->disc_comp.scn));
            if (p->disc_comp.status == ESP_SPP_SUCCESS) {
                esp_spp_connect(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_MASTER, *(p->disc_comp.scn), discovered_elm327_addr);
            } else {
                printf("SPP service discovery failed!\n");
            }
            break;


        default:
            break;
    }
}

void setup_elm327() {
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_bt_gap_register_callback(elm327_gap_cb));

    esp_bt_pin_code_t pin_code;
    memcpy(pin_code, "1234", 4);
    ESP_ERROR_CHECK(esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_FIXED, 4, pin_code));

    ESP_ERROR_CHECK(esp_spp_register_callback(spp_cb));
    ESP_ERROR_CHECK(esp_spp_enhanced_init(&(esp_spp_cfg_t){ .mode = ESP_SPP_MODE_CB }));

    ESP_ERROR_CHECK(esp_bt_gap_set_device_name("ESP32_OBD"));
}

static int convert_hexToInt(char hex) {
    if (hex >= '0' && hex <= '9') return hex - '0';
    if (hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
    if (hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
    return 0;
}

static inline int get_byte(const char *hex) {
    return (convert_hexToInt(hex[0]) << 4) | convert_hexToInt(hex[1]);
}

void sanitize_obd_response(const char *in, char *out, size_t out_size) {
    size_t j = 0;
    for (size_t i = 0; in[i] != '\0' && j < out_size - 1; ++i) {
        if (in[i] != ' ' && in[i] != '\r' && in[i] != '\n') {
            if (isxdigit((unsigned char)in[i])) {
                out[j++] = toupper(in[i]);  // uppercase hex
            }
        }
    }
    out[j] = '\0';
}

#define MAX_OBD_CLEANED_LEN 64

static void parse_pid_response(const char *response, OBDData *obd) {
    printf("Raw Response: %s\n", response);

    char cleaned[MAX_OBD_CLEANED_LEN];
    sanitize_obd_response(response, cleaned, sizeof(cleaned));

    if (strlen(cleaned) < 6 || strncmp(cleaned, "41", 2) != 0) {
        printf("Invalid or corrupted OBD response\n");
        return;
    }

    char pid_str[3] = { cleaned[2], cleaned[3], '\0' };
    printf("Parsed PID: %s\n", pid_str);

    const char *data = &cleaned[4];

    if (strcmp(pid_str, "0C") == 0 && strlen(data) >= 4) {
        int A = get_byte(&data[0]);
        int B = get_byte(&data[2]);
        obd->rpm = ((A * 256) + B) / 4;
        printf("RPM = %d\n", obd->rpm);

    } else if (strcmp(pid_str, "0D") == 0 && strlen(data) >= 2) {
        int A = get_byte(&data[0]);
        obd->speed = A;
        printf("Speed = %d km/h\n", obd->speed);

    } else if (strcmp(pid_str, "05") == 0 && strlen(data) >= 2) {
        int A = get_byte(&data[0]);
        obd->coolantTemp = A - 40;
        printf("Coolant Temp = %d°C\n", obd->coolantTemp);

    } else if (strcmp(pid_str, "11") == 0 && strlen(data) >= 2) {
        int A = get_byte(&data[0]);
        obd->throttlePos = (A * 100) / 255;
        printf("Throttle Position = %d%%\n", obd->throttlePos);

    } else if (strcmp(pid_str, "10") == 0 && strlen(data) >= 4) {
        int A = get_byte(&data[0]);
        int B = get_byte(&data[2]);
        obd->massAirFlow = ((A * 256) + B) / 100.0f;
        printf("MAF = %.2f g/s\n", obd->massAirFlow);

    } else if (strcmp(pid_str, "43") == 0 && strlen(data) >= 2) {
        int A = get_byte(&data[0]);
        obd->absoluteEngineLoad = (A * 100) / 255;
        printf("Abs. Engine Load = %d%%\n", obd->absoluteEngineLoad);

    } else {
        printf("Unsupported or malformed PID: %s\n", pid_str);
    }
}



void get_elm327_data(DataPacket *data) {
    if (!connected || handle == 0) {
        printf("ELM327 not connected. Skipping OBD query.\n");
        return;
    }

    const PIDCommand commands[] = {
        { "010C\r", 0x0C },  // RPM
        { "010D\r", 0x0D },  // Speed
        { "0105\r", 0x05 },  // Coolant Temp
        { "0111\r", 0x11 },  // Throttle Position
        { "0110\r", 0x10 },  // Mass Air Flow
        { "0143\r", 0x43 },  // Absolute Engine Load
    };
    const int num_commands = sizeof(commands) / sizeof(commands[0]);

    // Reset fields
    data->obd.speed = -1;
    data->obd.rpm = -1;
    data->obd.coolantTemp = -999;
    data->obd.throttlePos = -1;
    data->obd.massAirFlow = -1;
    data->obd.absoluteEngineLoad = -1;

    for (int i = 0; i < num_commands; i++) {
        response_ready = false;
        memset(latest_obd_response, 0, sizeof(latest_obd_response));

        esp_spp_write(handle, strlen(commands[i].pid_cmd), (uint8_t *)commands[i].pid_cmd);

        int attempts = 20;
        while (!response_ready && attempts-- > 0) {
            vTaskDelay(pdMS_TO_TICKS(30));
        }

        if (response_ready) {
            parse_pid_response(latest_obd_response, &data->obd);
        } else {
            printf("Timeout: No response for PID 0x%02X\n", commands[i].pid_code);
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Short delay between commands
    }
}
