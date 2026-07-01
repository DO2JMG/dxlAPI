#ifndef DXLWS_APP_H
#define DXLWS_APP_H

#include <stddef.h>
#include <time.h>

#define PROGRAM_NAME "dxlapi"
#define HTTP_SOFTWARE_NAME "dxlAPI"
#define PROGRAM_VERSION "0.10.2"

#define DEFAULT_BIND_ADDR "127.0.0.1"
#define DEFAULT_PORT 18001
#define MAX_PACKET 16384
#define MAX_JSON_OUT 8192
#define BATCH_MAX_FRAMES 20
#define BATCH_FLUSH_INTERVAL_SEC 5
#define MAX_BATCH_JSON_OUT 131072
#define MAX_SONDES 256
#define RATE_TABLE_ENTRY_TTL_SEC 10800
#define MAX_SERIAL_LEN 96
#define MAX_CALLSIGN_CHARS 10
#define MAX_CALLSIGN_LEN (MAX_CALLSIGN_CHARS + 1)
#define MAX_FIELD_LEN 160
#define MAX_XDATA_JSON_LEN 2048

#define TELEMETRY_URL "http://api.wettersonde.net/telemetrie.php"
#define POSITION_URL  "http://api.wettersonde.net/position.php"

#define UPLOAD_INTERVAL_LOW_ALT_SEC 2
#define UPLOAD_INTERVAL_MID_ALT_SEC 5
#define UPLOAD_INTERVAL_HIGH_ALT_SEC 10
#define ALTITUDE_LOW_LIMIT_M 2000.0
#define ALTITUDE_MID_LIMIT_M 5000.0

#define POSITION_UPLOAD_INTERVAL_SEC 1800
#define POSITION_RETRY_INTERVAL_SEC 60

typedef struct {
    char bind_addr[64];
    int port;
    char callsign[MAX_CALLSIGN_LEN];
    int verbose;
    int dry_run;
    int has_station_lat;
    int has_station_lon;
    double station_lat;
    double station_lon;
} config_t;

typedef struct {
    double latitude;
    double longitude;
    double altitude;
    double speed;
    double direction;
    double frequency;
    double climb;
    int frame;
    char type[MAX_FIELD_LEN];
    char serial[MAX_SERIAL_LEN];
    int has_temperature;
    int has_humidity;
    int has_pressure;
    int has_voltage;
    int has_rssi;
    int has_sat;
    int has_burstkilltimer;
    int has_killtimer;
    int has_xdata;
    int has_xdata1;
    int has_xdata2;
    int has_xdata3;
    char xdata[MAX_XDATA_JSON_LEN];
    char xdata1[MAX_XDATA_JSON_LEN];
    char xdata2[MAX_XDATA_JSON_LEN];
    char xdata3[MAX_XDATA_JSON_LEN];
    double temperature;
    double humidity;
    double pressure;
    double voltage;
    double rssi;
    int sat;
    int burstkilltimer;
    int killtimer;
} telemetry_t;

#endif
