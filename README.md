# VehicleTracker 


Data Packet from Device : ( Not actual packet, just a temporary structure)


Example: {"date":"2025,00,00","time":"INVALID","uptime":123,"gps":{"lat":999.999878,"lon":999.999878},"obd":{"rpm":0,"speed":0,"coolant_temp":0,"throttle_pos":0,"mass_air_flow":0.00,"absolute_engine_load":0}}


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
