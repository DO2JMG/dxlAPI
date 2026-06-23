#ifndef DXLWS_HTTP_CLIENT_H
#define DXLWS_HTTP_CLIENT_H

int http_post_json(const char *url, const char *json, int dry_run);

#endif
