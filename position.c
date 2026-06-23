#include "position.h"
#include "http_client.h"
#include "json_util.h"
#include <stdio.h>

int upload_receiver_position(const config_t *cfg) {
    char esc_call[MAX_CALLSIGN_LEN * 2];
    char json[MAX_JSON_OUT];
    json_escape(cfg->callsign, esc_call, sizeof(esc_call));

    snprintf(json, sizeof(json),
        "{\"latitude\":%.7f,\"longitude\":%.7f,\"callsign\":\"%s\","
        "\"software\":\"%s\",\"version\":\"%s\"}",
        cfg->station_lat, cfg->station_lon, esc_call, HTTP_SOFTWARE_NAME, PROGRAM_VERSION);

    return http_post_json(POSITION_URL, json, cfg->dry_run);
}
