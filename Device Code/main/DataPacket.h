
#ifndef DATAPACKET_H
#define DATAPACKET_H


typedef struct
{
    float lat;
    float lon;
} GPSData;


typedef struct 
{
    int speed;  // 0D
    int rpm; // OC
    int throttlePos; // 11
    float massAirFlow; // 10
    int coolantTemp; // 05
    int absoluteEngineLoad; // 43

} OBDData;



typedef struct 
{
    char date[11];          // Format: YYYY-MM-DD
    char time[9];           // Format: HH:MM:SS
    uint32_t uptime_seconds;
    GPSData gps;
    OBDData obd;
} DataPacket;


#endif