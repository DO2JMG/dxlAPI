#define _POSIX_C_SOURCE 200809L
#include "json_util.h"
#include "log.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *skip_ws(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
}

static int json_key_equals(const char *key_start, const char *key_end, const char *key) {
    size_t want = strlen(key);
    if ((size_t)(key_end - key_start) != want) return 0;
    return strncmp(key_start, key, want) == 0;
}

static const char *json_find_value(const char *json, const char *key) {
    const char *p = json;
    while ((p = strchr(p, '"')) != NULL) {
        const char *key_start = p + 1;
        const char *key_end = key_start;
        int escaped = 0;

        while (*key_end) {
            if (escaped) escaped = 0;
            else if (*key_end == '\\') escaped = 1;
            else if (*key_end == '"') break;
            key_end++;
        }
        if (*key_end != '"') return NULL;

        p = skip_ws(key_end + 1);
        if (*p == ':' && json_key_equals(key_start, key_end, key)) return skip_ws(p + 1);
        p = key_end + 1;
    }
    return NULL;
}

int json_get_number(const char *json, const char *key, double *out) {
    const char *p = json_find_value(json, key);
    char *end = NULL;
    if (!p) return 0;

    if (*p == '"') {
        char tmp[96];
        const char *q = p + 1;
        size_t i = 0;
        while (*q && *q != '"' && i + 1 < sizeof(tmp)) tmp[i++] = *q++;
        if (*q != '"') return 0;
        tmp[i] = '\0';
        errno = 0;
        *out = strtod(tmp, &end);
        return !errno && end && *end == '\0';
    }

    errno = 0;
    *out = strtod(p, &end);
    return !errno && end && end != p;
}

int json_get_int(const char *json, const char *key, int *out) {
    double v;
    if (!json_get_number(json, key, &v)) return 0;
    *out = (int)v;
    return 1;
}

int json_get_nested_number(const char *json, const char *object_key, const char *nested_key, double *out) {
    const char *p = json_find_value(json, object_key);
    if (!p) return 0;
    p = skip_ws(p);
    if (*p != '{') return 0;
    return json_get_number(p, nested_key, out);
}

int json_get_string(const char *json, const char *key, char *out, size_t out_size) {
    const char *p = json_find_value(json, key);
    size_t i = 0;
    if (!p || *p != '"' || out_size == 0) return 0;
    p++;
    while (*p && *p != '"' && i + 1 < out_size) {
        if (*p == '\\' && p[1]) {
            p++;
            switch (*p) {
                case 'n': out[i++] = '\n'; break;
                case 'r': out[i++] = '\r'; break;
                case 't': out[i++] = '\t'; break;
                case '"': out[i++] = '"'; break;
                case '\\': out[i++] = '\\'; break;
                case '/': out[i++] = '/'; break;
                default: out[i++] = *p; break;
            }
            p++;
        } else {
            out[i++] = *p++;
        }
    }
    if (*p != '"') return 0;
    out[i] = '\0';
    return i > 0;
}


int json_get_raw_array(const char *json, const char *key, char *out, size_t out_size) {
    const char *p = json_find_value(json, key);
    const char *start;
    int depth = 0;
    int in_string = 0;
    int escaped = 0;
    size_t len;

    if (!p || out_size == 0) return 0;
    p = skip_ws(p);
    if (*p != '[') return 0;

    start = p;
    while (*p) {
        char c = *p;
        if (in_string) {
            if (escaped) escaped = 0;
            else if (c == '\\') escaped = 1;
            else if (c == '"') in_string = 0;
        } else {
            if (c == '"') in_string = 1;
            else if (c == '[') depth++;
            else if (c == ']') {
                depth--;
                if (depth == 0) {
                    p++;
                    len = (size_t)(p - start);
                    if (len >= out_size) return 0;
                    memcpy(out, start, len);
                    out[len] = '\0';
                    return 1;
                }
            }
        }
        p++;
    }
    return 0;
}

int get_number_any(const char *json, const char **names, double *out) {
    for (int i = 0; names[i]; i++) if (json_get_number(json, names[i], out)) return 1;
    return 0;
}

int get_int_any(const char *json, const char **names, int *out) {
    for (int i = 0; names[i]; i++) if (json_get_int(json, names[i], out)) return 1;
    return 0;
}

int get_string_any(const char *json, const char **names, char *out, size_t out_size) {
    for (int i = 0; names[i]; i++) if (json_get_string(json, names[i], out, out_size)) return 1;
    return 0;
}

void json_escape(const char *in, char *out, size_t out_size) {
    size_t j = 0;
    if (out_size == 0) return;
    for (size_t i = 0; in[i] && j + 2 < out_size; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c == '"' || c == '\\') {
            if (j + 3 >= out_size) break;
            out[j++] = '\\';
            out[j++] = (char)c;
        } else if (c >= 32 && c < 127) {
            out[j++] = (char)c;
        }
    }
    out[j] = '\0';
}

void utc_hhmmss(char out[7]) {
    time_t now = time(NULL);
    struct tm tm_utc;
    gmtime_r(&now, &tm_utc);
    snprintf(out, 7, "%02d%02d%02d", tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec);
}

int packet_looks_like_json(const char *packet) {
    const char *p = skip_ws(packet);
    return *p == '{' || *p == '[';
}

void log_packet_preview(const char *packet) {
    char ascii[257];
    char hex[3 * 32 + 1];
    size_t len = strlen(packet);
    size_t n_ascii = len < sizeof(ascii) - 1 ? len : sizeof(ascii) - 1;
    size_t n_hex = len < 32 ? len : 32;

    for (size_t i = 0; i < n_ascii; i++) {
        unsigned char c = (unsigned char)packet[i];
        ascii[i] = (c >= 32 && c < 127) ? (char)c : '.';
    }
    ascii[n_ascii] = '\0';

    hex[0] = '\0';
    for (size_t i = 0; i < n_hex; i++) {
        char tmp[4];
        snprintf(tmp, sizeof(tmp), "%02X ", (unsigned char)packet[i]);
        strncat(hex, tmp, sizeof(hex) - strlen(hex) - 1);
    }

    log_msg("DEBUG", "Packet preview : %s%s", ascii, len > n_ascii ? "..." : "");
  
}
