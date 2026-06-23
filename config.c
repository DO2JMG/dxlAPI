#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include "log.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void usage(FILE *out) {
    fprintf(out,
        PROGRAM_NAME " " PROGRAM_VERSION "\n"
        "\n"
        "Receives dxlAPRS / sondemod UDP JSON telemetry and uploads it to wettersonde.net.\n"
        "\n"
        "Usage:\n"
        "  " PROGRAM_NAME " -callsign CALLSIGN -lat LAT -lon LON [options]\n"
        "\n"
        "Required:\n"
        "  -callsign CALLSIGN       Your receiver callsign, e.g. DL1XYZ-11\n"
        "  -lat LAT                 Receiver latitude; required for position upload\n"
        "  -lon LON                 Receiver longitude; required for position upload\n"
        "\n"
        "Options:\n"
        "  -bind ADDR               UDP bind address, default 127.0.0.1\n"
        "  -port PORT               UDP listen port, default 18001\n"
        "  -dry                     Print converted JSON but do not upload\n"
        "  -v                       Print received JSON and more diagnostics\n"
        "  -h                       Show this help\n"
        "\n"
    );
}

static int parse_int(const char *s, int *out) {
    char *end = NULL;
    long v;
    if (!s || *s == '\0') return 0;
    errno = 0;
    v = strtol(s, &end, 10);
    if (errno || !end || *end != '\0' || v < 1 || v > 65535) return 0;
    *out = (int)v;
    return 1;
}

static int parse_double_arg(const char *s, double *out) {
    char *end = NULL;
    double v;
    if (!s || *s == '\0') return 0;
    errno = 0;
    v = strtod(s, &end);
    if (errno || !end || *end != '\0') return 0;
    *out = v;
    return 1;
}

static int looks_like_callsign(const char *s) {
    size_t len;
    if (!s) return 0;
    len = strlen(s);
    if (len < 3 || len > MAX_CALLSIGN_CHARS) return 0;
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-')) return 0;
    }
    return 1;
}

static int is_opt(const char *arg, const char *preferred, const char *legacy) {
    return !strcmp(arg, preferred) || (legacy && !strcmp(arg, legacy));
}

int parse_args(int argc, char **argv, config_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    snprintf(cfg->bind_addr, sizeof(cfg->bind_addr), "%s", DEFAULT_BIND_ADDR);
    cfg->port = DEFAULT_PORT;

    for (int i = 1; i < argc; i++) {
        if (is_opt(argv[i], "-h", "--help")) {
            usage(stdout);
            exit(0);
        } else if (is_opt(argv[i], "-bind", "--bind") && i + 1 < argc) {
            snprintf(cfg->bind_addr, sizeof(cfg->bind_addr), "%s", argv[++i]);
        } else if (is_opt(argv[i], "-port", "--port") && i + 1 < argc) {
            if (!parse_int(argv[++i], &cfg->port)) {
                log_msg("ERROR", "Invalid -port value");
                return 0;
            }
        } else if (is_opt(argv[i], "-callsign", "--callsign") && i + 1 < argc) {
            snprintf(cfg->callsign, sizeof(cfg->callsign), "%s", argv[++i]);
        } else if (is_opt(argv[i], "-lat", "--station-lat") && i + 1 < argc) {
            if (!parse_double_arg(argv[++i], &cfg->station_lat)) {
                log_msg("ERROR", "Invalid -lat value");
                return 0;
            }
            cfg->has_station_lat = 1;
        } else if (is_opt(argv[i], "-lon", "--station-lon") && i + 1 < argc) {
            if (!parse_double_arg(argv[++i], &cfg->station_lon)) {
                log_msg("ERROR", "Invalid -lon value");
                return 0;
            }
            cfg->has_station_lon = 1;
        } else if (is_opt(argv[i], "-dry", "--dry-run")) {
            cfg->dry_run = 1;
        } else if (is_opt(argv[i], "-v", "--verbose")) {
            cfg->verbose = 1;
        } else {
            log_msg("ERROR", "Unknown or incomplete option: %s", argv[i]);
            return 0;
        }
    }

    if (!looks_like_callsign(cfg->callsign)) {
        log_msg("ERROR", "Missing or invalid -callsign");
        return 0;
    }
    if (!cfg->has_station_lat || !cfg->has_station_lon) {
        log_msg("ERROR", "Missing required -lat and/or -lon; position upload is mandatory");
        return 0;
    }
    if (cfg->station_lat < -90.0 || cfg->station_lat > 90.0 ||
        cfg->station_lon < -180.0 || cfg->station_lon > 180.0) {
        log_msg("ERROR", "Invalid receiver position; latitude must be -90..90 and longitude -180..180");
        return 0;
    }
    return 1;
}
