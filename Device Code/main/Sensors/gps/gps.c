#include "gps.h"

uart_config_t uart_config = {
    .baud_rate = GPS_BAUD_RATE, // GPS baud rate
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
};


const uint8_t UBX_ENABLE_ZDA[] = {
  0xB5, 0x62,             // UBX sync chars
  0x06, 0x01,             // CFG-MSG class and ID
  0x08, 0x00,             // Payload length = 8 bytes
  0xF0, 0x08,             // Message: NMEA ZDA
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, // Enable ZDA on all ports
  0x3A, 0x2C              // Checksum
};




void setup_gps() {
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, 1024, 0, 0, NULL, 0);
}

// Helper to parse 2-digit substring to int
static inline int atoi_n(const char *str, int n) {
    char buf[4] = {0};
    strncpy(buf, str, n);
    return atoi(buf);
}

int fast_split_nmea(const char *line, const char **fields, int max_fields) {
    int count = 0;
    const char *start = line;
    while (*line && count < max_fields) {
        if (*line == ',') {
            fields[count++] = start;
            start = line + 1;
        }
        line++;
    }
    if (count < max_fields && *start != '\0') {
        fields[count++] = start;
    }
    return count;
}

void parse_gga(const char *line, DataPacket *data) {
    const char *fields[GGA_FIELD_COUNT];
    int count = fast_split_nmea(line, fields, GGA_FIELD_COUNT);
    if (count < 10) goto invalid;

    // Time: hhmmss.sss
    const char *time = fields[1];
    if (time && strlen(time) >= 6) {
        snprintf(data->time, sizeof(data->time), "%02d:%02d:%02d",
                 atoi_n(time, 2),
                 atoi_n(time + 2, 2),
                 atoi_n(time + 4, 2));
    }

    int fix_quality = atoi(fields[6]);
    if (fix_quality == 0 || !fields[2] || !fields[3] || !fields[4] || !fields[5]) goto invalid;
    if (fields[2][0] == '\0' || fields[4][0] == '\0') goto invalid;

    // Latitude
    float raw_lat = atof(fields[2]);
    char lat_dir = fields[3][0];
    int lat_deg = (int)(raw_lat / 100);
    float lat_min = raw_lat - (lat_deg * 100);
    float lat = lat_deg + lat_min / 60.0f;
    if (lat_dir == 'S') lat *= -1;

    // Longitude
    float raw_lon = atof(fields[4]);
    char lon_dir = fields[5][0];
    int lon_deg = (int)(raw_lon / 100);
    float lon_min = raw_lon - (lon_deg * 100);
    float lon = lon_deg + lon_min / 60.0f;
    if (lon_dir == 'W') lon *= -1;

    data->gps.lat = lat;
    data->gps.lon = lon;
    return;

invalid:
    data->gps.lat = GPS_DEFAULT;
    data->gps.lon = GPS_DEFAULT;
    strncpy(data->time, "INVALID", sizeof(data->time));
}




void parse_zda(const char *line, DataPacket *data) {
    const char *fields[ZDA_FIELD_COUNT];
    int count = fast_split_nmea(line, fields, ZDA_FIELD_COUNT);
    if (count < 5) goto invalid;

    const char *day  = fields[2];
    const char *mon  = fields[3];
    const char *year = fields[4];

    if (!day || !mon || !year || strlen(day) == 0 || strlen(mon) == 0 || strlen(year) < 4) {
        goto invalid;
    }

    snprintf(data->date, sizeof(data->date), "%s-%02d-%02d", year, atoi(mon), atoi(day));
    return;

invalid:
    strncpy(data->date, "INVALID", sizeof(data->date));
}



void get_gps_data(DataPacket* data) {
    uint8_t dataline[1024];
    memset(dataline, 0, sizeof(dataline));

    int len = uart_read_bytes(UART_NUM, dataline, sizeof(dataline), pdMS_TO_TICKS(200));
    if (len <= 0) return;  // no data received

    dataline[len < sizeof(dataline) ? len : sizeof(dataline) - 1] = '\0';

    char *line = strtok((char *)dataline, "\r\n");

    while (line != NULL) {
        if (strstr(line, "$GNGGA") || strstr(line, "$GPGGA")) {
            parse_gga(line, data);
        } else if (strstr(line, "$GNZDA") || strstr(line, "$GPZDA")) {
            parse_zda(line, data);
        } else if (strstr(line, "$GPTXT")) {
        }
        line = strtok(NULL, "\r\n");
    }
}
