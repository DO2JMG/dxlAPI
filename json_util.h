#ifndef DXLWS_JSON_UTIL_H
#define DXLWS_JSON_UTIL_H

#include <stddef.h>

const char *skip_ws(const char *p);
int json_get_number(const char *json, const char *key, double *out);
int json_get_int(const char *json, const char *key, int *out);
int json_get_nested_number(const char *json, const char *object_key, const char *nested_key, double *out);
int json_get_string(const char *json, const char *key, char *out, size_t out_size);
int get_number_any(const char *json, const char **names, double *out);
int get_int_any(const char *json, const char **names, int *out);
int get_string_any(const char *json, const char **names, char *out, size_t out_size);
void json_escape(const char *in, char *out, size_t out_size);
void utc_hhmmss(char out[7]);
int packet_looks_like_json(const char *packet);
void log_packet_preview(const char *packet);

#endif
