#ifndef DXLWS_TELEMETRY_H
#define DXLWS_TELEMETRY_H

#include "app.h"

int parse_telemetry(const char *packet, telemetry_t *t);
int validate_telemetry(const telemetry_t *t);
int build_telemetry_json(const telemetry_t *t, const config_t *cfg, char *out, unsigned long out_size);
int convert_and_upload_telemetry(const char *packet, const config_t *cfg);

#endif
