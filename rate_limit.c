#define _POSIX_C_SOURCE 200809L
#include "rate_limit.h"
#include "app.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
    char serial[MAX_SERIAL_LEN];
    time_t last_upload;
} sonde_rate_t;

static sonde_rate_t g_rate_table[MAX_SONDES];

int upload_interval_for_altitude(double altitude) {
    if (altitude < ALTITUDE_LOW_LIMIT_M) return UPLOAD_INTERVAL_LOW_ALT_SEC;
    if (altitude <= ALTITUDE_MID_LIMIT_M) return UPLOAD_INTERVAL_MID_ALT_SEC;
    return UPLOAD_INTERVAL_HIGH_ALT_SEC;
}

int rate_limit_allows(const char *serial, double altitude, int *interval_out) {
    time_t now = time(NULL);
    int interval = upload_interval_for_altitude(altitude);
    if (interval_out) *interval_out = interval;

    for (int i = 0; i < MAX_SONDES; i++) {
        if (g_rate_table[i].serial[0] == '\0') {
            snprintf(g_rate_table[i].serial, sizeof(g_rate_table[i].serial), "%s", serial);
            g_rate_table[i].last_upload = now;
            return 1;
        }
        if (strcmp(g_rate_table[i].serial, serial) == 0) {
            double age = difftime(now, g_rate_table[i].last_upload);
            if (age < interval) return 0;
            g_rate_table[i].last_upload = now;
            return 1;
        }
    }
    log_msg("WARN", "Rate table full, dropping serial=%s", serial);
    return 0;
}
