#ifndef DXLWS_RATE_LIMIT_H
#define DXLWS_RATE_LIMIT_H

int upload_interval_for_altitude(double altitude);
int rate_limit_allows(const char *serial, double altitude, int *interval_out);

#endif
