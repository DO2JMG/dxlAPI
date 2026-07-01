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

static void rate_table_clear_entry(int i) {
    g_rate_table[i].serial[0] = '\0';
    g_rate_table[i].last_upload = 0;
}

static int rate_table_cleanup_expired(time_t now) {
    int removed = 0;
    for (int i = 0; i < MAX_SONDES; i++) {
        if (g_rate_table[i].serial[0] == '\0') continue;
        if (g_rate_table[i].last_upload <= 0) {
            rate_table_clear_entry(i);
            removed++;
            continue;
        }
        if (difftime(now, g_rate_table[i].last_upload) > RATE_TABLE_ENTRY_TTL_SEC) {
            rate_table_clear_entry(i);
            removed++;
        }
    }
    return removed;
}

int upload_interval_for_altitude(double altitude) {
    if (altitude < ALTITUDE_LOW_LIMIT_M) return UPLOAD_INTERVAL_LOW_ALT_SEC;
    if (altitude <= ALTITUDE_MID_LIMIT_M) return UPLOAD_INTERVAL_MID_ALT_SEC;
    return UPLOAD_INTERVAL_HIGH_ALT_SEC;
}

int rate_limit_allows(const char *serial, double altitude, int *interval_out) {
    time_t now = time(NULL);
    int interval = upload_interval_for_altitude(altitude);
    if (interval_out) *interval_out = interval;

    int removed = rate_table_cleanup_expired(now);
    if (removed > 0) {
        log_msg("DEBUG", "Rate table cleanup removed %d expired entr%s", removed, removed == 1 ? "y" : "ies");
    }

    int free_slot = -1;

    for (int i = 0; i < MAX_SONDES; i++) {
        if (g_rate_table[i].serial[0] == '\0') {
            if (free_slot < 0) free_slot = i;
            continue;
        }

        if (strcmp(g_rate_table[i].serial, serial) == 0) {
            double age = difftime(now, g_rate_table[i].last_upload);
            if (age < interval) return 0;
            g_rate_table[i].last_upload = now;
            return 1;
        }
    }

    if (free_slot >= 0) {
        snprintf(g_rate_table[free_slot].serial, sizeof(g_rate_table[free_slot].serial), "%s", serial);
        g_rate_table[free_slot].last_upload = now;
        return 1;
    }

    log_msg("WARN", "Rate table full, dropping serial=%s; no entries older than %d seconds", serial, RATE_TABLE_ENTRY_TTL_SEC);
    return 0;
}
