#ifndef DXLWS_BATCH_H
#define DXLWS_BATCH_H

#include "app.h"

typedef struct {
    char frames[BATCH_MAX_FRAMES][MAX_JSON_OUT];
    int count;
    time_t first_queued;
} batch_t;

void batch_init(batch_t *batch);
int batch_add_frame(batch_t *batch, const char *frame_json);
int batch_count(const batch_t *batch);
int batch_should_flush(const batch_t *batch, time_t now);
int batch_flush(batch_t *batch, const config_t *cfg);

#endif
