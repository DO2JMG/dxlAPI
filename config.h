#ifndef DXLWS_CONFIG_H
#define DXLWS_CONFIG_H

#include "app.h"
#include <stdio.h>

void usage(FILE *out);
int parse_args(int argc, char **argv, config_t *cfg);

#endif
