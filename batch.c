#define _POSIX_C_SOURCE 200809L
#include "batch.h"
#include "http_client.h"
#include "json_util.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

void batch_init(batch_t *batch) {
    memset(batch, 0, sizeof(*batch));
}

int batch_add_frame(batch_t *batch, const char *frame_json) {
    if (batch->count >= BATCH_MAX_FRAMES) return 0;
    snprintf(batch->frames[batch->count], sizeof(batch->frames[batch->count]), "%s", frame_json);
    if (batch->count == 0) batch->first_queued = time(NULL);
    batch->count++;
    return 1;
}

int batch_count(const batch_t *batch) {
    return batch->count;
}

int batch_should_flush(const batch_t *batch, time_t now) {
    if (batch->count <= 0) return 0;
    if (batch->count >= BATCH_MAX_FRAMES) return 1;
    return difftime(now, batch->first_queued) >= BATCH_FLUSH_INTERVAL_SEC;
}

int batch_flush(batch_t *batch, const config_t *cfg) {
    char payload[MAX_BATCH_JSON_OUT];
    char esc_client[MAX_CALLSIGN_LEN * 2];
    int n;

    if (batch->count <= 0) return 0;

    json_escape(cfg->callsign, esc_client, sizeof(esc_client));
    n = snprintf(payload, sizeof(payload), "{\"client\":\"%s\",\"frames\":[", esc_client);
    if (n < 0 || (unsigned long)n >= sizeof(payload)) {
        log_msg("ERROR", "Could not build batch JSON header");
        return -1;
    }

    for (int i = 0; i < batch->count; i++) {
        int m = snprintf(payload + n, sizeof(payload) - (unsigned long)n,
                         "%s%s", i ? "," : "", batch->frames[i]);
        if (m < 0 || (unsigned long)m >= sizeof(payload) - (unsigned long)n) {
            log_msg("ERROR", "Batch JSON too large; dropping queued batch");
            batch_init(batch);
            return -1;
        }
        n += m;
    }

    if ((unsigned long)n + 3 > sizeof(payload)) {
        log_msg("ERROR", "Batch JSON too large at closing");
        batch_init(batch);
        return -1;
    }
    payload[n++] = ']';
    payload[n++] = '}';
    payload[n] = '\0';

    if (cfg->verbose) log_msg("INFO", "Uploading telemetry batch with %d frame(s)", batch->count);

    int rc = http_post_json(TELEMETRY_URL, payload, cfg->dry_run);
    if (rc == 0) {
        batch_init(batch);
    } else {
        log_msg("ERROR", "Telemetry batch upload failed; keeping %d frame(s) queued", batch->count);
    }
    return rc;
}
