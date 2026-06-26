#define _POSIX_C_SOURCE 200809L
#include "telemetry.h"
#include "batch.h"
#include "json_util.h"
#include "log.h"
#include "rate_limit.h"
#include <stdio.h>
#include <string.h>

static int has_number_any(const char *json, const char **names) {
    double dummy;
    return get_number_any(json, names, &dummy);
}

static int has_int_any(const char *json, const char **names) {
    int dummy;
    return get_int_any(json, names, &dummy);
}

static int has_string_any(const char *json, const char **names) {
    char dummy[MAX_SERIAL_LEN];
    return get_string_any(json, names, dummy, sizeof(dummy));
}

static int starts_with_ci(const char *s, const char *prefix) {
    if (!s || !prefix) return 0;
    while (*prefix) {
        char a = *s++;
        char b = *prefix++;
        if (a >= 'a' && a <= 'z') a = (char)(a - 'a' + 'A');
        if (b >= 'a' && b <= 'z') b = (char)(b - 'a' + 'A');
        if (a != b) return 0;
    }
    return 1;
}

static int equals_ci(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        char ca = *a++;
        char cb = *b++;
        if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 'a' + 'A');
        if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 'a' + 'A');
        if (ca != cb) return 0;
    }
    return *a == '\0' && *b == '\0';
}

static int uses_frame_zero_when_missing(const char *type) {
    return starts_with_ci(type, "DFM") ||
           starts_with_ci(type, "IMET") ||
           equals_ci(type, "M10") ||
           equals_ci(type, "M20");
}

static void log_missing_fields(const char *packet,
                               const char **lat_names,
                               const char **lon_names,
                               const char **alt_names,
                               const char **speed_names,
                               const char **dir_names,
                               const char **frame_names,
                               const char **climb_names,
                               const char **type_names,
                               const char **serial_names) {
    char missing[512] = "";
#define ADD_MISSING(cond, name) \
    do { \
        if (!(cond)) { \
            if (missing[0]) strncat(missing, ", ", sizeof(missing) - strlen(missing) - 1); \
            strncat(missing, name, sizeof(missing) - strlen(missing) - 1); \
        } \
    } while (0)

    ADD_MISSING(has_number_any(packet, lat_names), "latitude");
    ADD_MISSING(has_number_any(packet, lon_names), "longitude");
    ADD_MISSING(has_number_any(packet, alt_names), "altitude");
    ADD_MISSING(has_number_any(packet, speed_names), "speed");
    ADD_MISSING(has_number_any(packet, dir_names), "direction");
    if (!has_int_any(packet, frame_names)) {
        char type_tmp[MAX_FIELD_LEN];
        int has_frame_fallback = get_string_any(packet, type_names, type_tmp, sizeof(type_tmp)) &&
                                 uses_frame_zero_when_missing(type_tmp);
        ADD_MISSING(has_frame_fallback, "frame");
    }
    ADD_MISSING(has_number_any(packet, climb_names), "climb");
    ADD_MISSING(has_string_any(packet, type_names), "type");
    ADD_MISSING(has_string_any(packet, serial_names), "serial");

#undef ADD_MISSING

    if (missing[0]) log_msg("DEBUG", "Missing mapped fields: %s", missing);
}

int parse_telemetry(const char *packet, telemetry_t *t) {
    static const char *lat_names[]    = {"latitude", "lat", "latit", "la", "y", NULL};
    static const char *lon_names[]    = {"longitude", "long", "lon", "lng", "longit", "lo", "x", NULL};
    static const char *alt_names[]    = {"altitude", "alt", "alti", "height", "heigth", NULL};
    static const char *speed_names[]  = {"speed", "spd", "vel", "velocity", "vel_h", "hspeed", "hvel", "h_speed", "ground_speed", NULL};
    static const char *dir_names[]    = {"direction", "dir", "heading", "course", "azimuth", "bearing", NULL};
    static const char *freq_names[]   = {"frequency", "freq", "mhz", "rx_frequency", "rxfreq", "f", NULL};
    static const char *frame_names[]  = {"frame", "frameno", "framenumber", "frame_number", "frnr", "framenr", "frn", "num", "number", "n", "up", NULL};
    static const char *climb_names[]  = {"climb", "clb", "vel_v", "vspeed", "vvel", "vs", "v_speed", "vertical_speed", NULL};
    static const char *type_names[]   = {"type", "typ", "sonde_type", "sondetype", "model", "sonde", NULL};
    static const char *serial_names[] = {"id", "serial", "sonde_id", "sondeid", "ident", "name", "ser", NULL};
    static const char *temp_names[]   = {"temperature", "temp", "t", NULL};
    static const char *hum_names[]    = {"humidity", "hum", "h", NULL};
    static const char *pres_names[]   = {"pressure", "pres", "p", NULL};
    static const char *volt_names[]   = {"voltage", "batt", "battery", "ub", NULL};
    static const char *rssi_names[]   = {"rssi", NULL};
    static const char *sat_names[]    = {"sat", "sats", "used", "used_sats", NULL};
    static const char *bursttx_names[] = {"bursttx", NULL};
    static const char *txoff_names[]   = {"txoff", NULL};

    memset(t, 0, sizeof(*t));

    if (!(get_number_any(packet, lat_names, &t->latitude) &&
          get_number_any(packet, lon_names, &t->longitude) &&
          get_number_any(packet, alt_names, &t->altitude) &&
          get_number_any(packet, speed_names, &t->speed) &&
          get_number_any(packet, dir_names, &t->direction) &&
          get_number_any(packet, climb_names, &t->climb) &&
          get_string_any(packet, type_names, t->type, sizeof(t->type)) &&
          get_string_any(packet, serial_names, t->serial, sizeof(t->serial)))) {
        return 0;
    }

    int has_frame = get_int_any(packet, frame_names, &t->frame);
    if (!has_frame && uses_frame_zero_when_missing(t->type)) {
        t->frame = 0;
        has_frame = 1;
    }
    if (!has_frame) {
        t->frame = -1;
        return 0;
    }

    if (!get_number_any(packet, freq_names, &t->frequency)) {
        if (!json_get_nested_number(packet, "sdr", "rx", &t->frequency)) {
            t->frequency = 404.500;
        }
    }

    if (strcmp(t->type, "RS41") == 0) {
        char rs41_variant[MAX_FIELD_LEN];
        if (json_get_string(packet, "ser", rs41_variant, sizeof(rs41_variant))) {
            snprintf(t->type, sizeof(t->type), "%s", rs41_variant);
        }
    }

    t->has_temperature = get_number_any(packet, temp_names, &t->temperature);
    t->has_humidity = get_number_any(packet, hum_names, &t->humidity);
    t->has_pressure = get_number_any(packet, pres_names, &t->pressure);
    t->has_voltage = get_number_any(packet, volt_names, &t->voltage);
    t->has_rssi = get_number_any(packet, rssi_names, &t->rssi);
    t->has_sat = get_int_any(packet, sat_names, &t->sat);
    if (t->has_sat && t->sat <= 0) t->has_sat = 0;
    t->has_burstkilltimer = get_int_any(packet, bursttx_names, &t->burstkilltimer);
    t->has_killtimer = get_int_any(packet, txoff_names, &t->killtimer);

    t->has_xdata = json_get_raw_array(packet, "xdata", t->xdata, sizeof(t->xdata));
    t->has_xdata1 = json_get_raw_array(packet, "xdata1", t->xdata1, sizeof(t->xdata1));
    t->has_xdata2 = json_get_raw_array(packet, "xdata2", t->xdata2, sizeof(t->xdata2));
    t->has_xdata3 = json_get_raw_array(packet, "xdata3", t->xdata3, sizeof(t->xdata3));
    return 1;
}

static int serial_starts_with_letter(const char *serial) {
    if (!serial || serial[0] == '\0') return 0;
    return (serial[0] >= 'A' && serial[0] <= 'Z') ||
           (serial[0] >= 'a' && serial[0] <= 'z');
}

int validate_telemetry(const telemetry_t *t) {
    if (t->altitude > 45000.0) {
        log_msg("WARN", "Frame rejected: altitude %.1f m is greater than 45000 m for serial=%s frame=%d", t->altitude, t->serial, t->frame);
        return 0;
    }
    if (t->latitude == 0.0) {
        log_msg("WARN", "Frame rejected: latitude is 0 for serial=%s frame=%d", t->serial, t->frame);
        return 0;
    }
    if (t->longitude == 0.0) {
        log_msg("WARN", "Frame rejected: longitude is 0 for serial=%s frame=%d", t->serial, t->frame);
        return 0;
    }
    if (t->type[0] == '\0') {
        log_msg("WARN", "Frame rejected: type is missing for serial=%s frame=%d", t->serial, t->frame);
        return 0;
    }
    if (t->frame < 0) {
        log_msg("WARN", "Frame rejected: frame number is invalid for serial=%s frame=%d", t->serial, t->frame);
        return 0;
    }
    if (!serial_starts_with_letter(t->serial)) {
        log_msg("WARN", "Frame rejected: serial must start with a letter, got serial=%s frame=%d", t->serial[0] ? t->serial : "<missing>", t->frame);
        return 0;
    }
    return 1;
}

int build_telemetry_json(const telemetry_t *t, const config_t *cfg, char *out, unsigned long out_size) {
    char timestamp[7];
    char esc_type[MAX_FIELD_LEN * 2];
    char esc_serial[MAX_SERIAL_LEN * 2];
    char esc_call[MAX_CALLSIGN_LEN * 2];
    int n;

    utc_hhmmss(timestamp);
    json_escape(t->type, esc_type, sizeof(esc_type));
    json_escape(t->serial, esc_serial, sizeof(esc_serial));
    json_escape(cfg->callsign, esc_call, sizeof(esc_call));

    n = snprintf(out, out_size,
        "{\"timestamp\":\"%s\",\"frame\":%d,\"latitude\":%.6f,\"longitude\":%.6f,"
        "\"altitude\":%.1f,\"speed\":%.1f,\"direction\":%d,\"frequency\":%.3f,"
        "\"type\":\"%s\",\"serial\":\"%s\",\"callsign\":\"%s\",\"climb\":%.1f",
        timestamp, t->frame, t->latitude, t->longitude, t->altitude, t->speed,
        (int)t->direction, t->frequency, esc_type, esc_serial, esc_call, t->climb);

    if (n < 0 || (unsigned long)n >= out_size) return 0;

#define APPEND_OPTIONAL(flag, name, value, fmt) \
    do { \
        if (flag) { \
            int m = snprintf(out + n, out_size - (unsigned long)n, ",\"%s\":" fmt, name, value); \
            if (m < 0 || (unsigned long)m >= out_size - (unsigned long)n) return 0; \
            n += m; \
        } \
    } while (0)

    APPEND_OPTIONAL(t->has_temperature, "temperature", t->temperature, "%.1f");
    APPEND_OPTIONAL(t->has_humidity, "humidity", t->humidity, "%.1f");
    APPEND_OPTIONAL(t->has_pressure, "pressure", t->pressure, "%.1f");
    APPEND_OPTIONAL(t->has_voltage, "voltage", t->voltage, "%.1f");
    APPEND_OPTIONAL(t->has_rssi, "rssi", t->rssi, "%.1f");

#define APPEND_OPTIONAL_INT(flag, name, value) \
    do { \
        if (flag) { \
            int m = snprintf(out + n, out_size - (unsigned long)n, ",\"%s\":%d", name, value); \
            if (m < 0 || (unsigned long)m >= out_size - (unsigned long)n) return 0; \
            n += m; \
        } \
    } while (0)

    APPEND_OPTIONAL_INT(t->has_sat, "sat", t->sat);
    APPEND_OPTIONAL_INT(t->has_burstkilltimer, "burstkilltimer", t->burstkilltimer);
    APPEND_OPTIONAL_INT(t->has_killtimer, "killtimer", t->killtimer);

#define APPEND_OPTIONAL_ARRAY(flag, name, value) \
    do { \
        if (flag) { \
            int m = snprintf(out + n, out_size - (unsigned long)n, ",\"%s\":%s", name, value); \
            if (m < 0 || (unsigned long)m >= out_size - (unsigned long)n) return 0; \
            n += m; \
        } \
    } while (0)

    APPEND_OPTIONAL_ARRAY(t->has_xdata, "xdata", t->xdata);
    APPEND_OPTIONAL_ARRAY(t->has_xdata1, "xdata1", t->xdata1);
    APPEND_OPTIONAL_ARRAY(t->has_xdata2, "xdata2", t->xdata2);
    APPEND_OPTIONAL_ARRAY(t->has_xdata3, "xdata3", t->xdata3);

#undef APPEND_OPTIONAL_ARRAY
#undef APPEND_OPTIONAL_INT
#undef APPEND_OPTIONAL

    if ((unsigned long)n + 2 > out_size) return 0;
    out[n++] = '}';
    out[n] = '\0';
    return 1;
}

int convert_and_queue_telemetry(const char *packet, const config_t *cfg, batch_t *batch) {
    telemetry_t t;
    char json[MAX_JSON_OUT];

    if (!packet_looks_like_json(packet)) {
        log_msg("WARN", "Received non-JSON UDP packet; packet skipped. Use sondemod -J <ip>:<port>, not -r/AXUDP/APRS.");
        if (cfg->verbose) log_packet_preview(packet);
        return -1;
    }

    if (!parse_telemetry(packet, &t)) {
        log_msg("WARN", "Missing required telemetry field or unsupported sondemod JSON; packet skipped");
        if (cfg->verbose) {
            static const char *lat_names[]    = {"latitude", "lat", "latit", "la", "y", NULL};
            static const char *lon_names[]    = {"longitude", "long", "lon", "lng", "longit", "lo", "x", NULL};
            static const char *alt_names[]    = {"altitude", "alt", "alti", "height", "heigth", NULL};
            static const char *speed_names[]  = {"speed", "spd", "vel", "velocity", "vel_h", "hspeed", "hvel", "h_speed", "ground_speed", NULL};
            static const char *dir_names[]    = {"direction", "dir", "heading", "course", "azimuth", "bearing", NULL};
            static const char *frame_names[]  = {"frame", "frameno", "framenumber", "frame_number", "frnr", "framenr", "frn", "num", "number", "n", "up", NULL};
            static const char *climb_names[]  = {"climb", "clb", "vel_v", "vspeed", "vvel", "vs", "v_speed", "vertical_speed", NULL};
            static const char *type_names[]   = {"type", "typ", "sonde_type", "sondetype", "model", "sonde", NULL};
            static const char *serial_names[] = {"id", "serial", "sonde_id", "sondeid", "ident", "name", "ser", NULL};
            log_missing_fields(packet, lat_names, lon_names, alt_names, speed_names, dir_names, frame_names, climb_names, type_names, serial_names);
            log_packet_preview(packet);
        }
        return -1;
    }

    if (!validate_telemetry(&t)) return -1;

    if (t.latitude < -90.0 || t.latitude > 90.0 ||
        t.longitude < -180.0 || t.longitude > 180.0) {
        log_msg("WARN", "Invalid coordinates for serial=%s", t.serial);
        return -1;
    }

    if (t.frequency < 400.0 || t.frequency > 406.0) {
        log_msg("WARN", "Frequency %.3f MHz out of wettersonde.net range for serial=%s", t.frequency, t.serial);
        return -1;
    }

    int upload_interval = upload_interval_for_altitude(t.altitude);
    if (!rate_limit_allows(t.serial, t.altitude, &upload_interval)) {
        if (cfg->verbose) {
            log_msg("INFO", "Rate limit: skipped serial=%s altitude=%.1fm interval=%ds", t.serial, t.altitude, upload_interval);
        }
        return 0;
    }

    if (!build_telemetry_json(&t, cfg, json, sizeof(json))) {
        log_msg("ERROR", "Could not build output JSON for serial=%s", t.serial);
        return -1;
    }

    if (cfg->verbose) {
        log_msg("INFO", "Queueing serial=%s frame=%d altitude=%.1fm interval=%ds", t.serial, t.frame, t.altitude, upload_interval);
    }
    if (!batch_add_frame(batch, json)) {
        log_msg("ERROR", "Telemetry batch is full; could not queue serial=%s", t.serial);
        return -1;
    }

    if (cfg->verbose) {
        log_msg("INFO", "Queued serial=%s frame=%d for batch upload", t.serial, t.frame);
    }
    return 1;
}
