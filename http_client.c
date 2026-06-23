#define _POSIX_C_SOURCE 200809L
#include "http_client.h"
#include "app.h"
#include "log.h"
#include <curl/curl.h>
#include <stdio.h>

int http_post_json(const char *url, const char *json, int dry_run) {
    if (dry_run) {
        printf("DRY-RUN POST %s %s\n", url, json);
        return 0;
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        log_msg("ERROR", "curl_easy_init failed");
        return -1;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, PROGRAM_NAME "/" PROGRAM_VERSION);

    CURLcode res = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        log_msg("ERROR", "HTTP POST failed: %s", curl_easy_strerror(res));
        return -1;
    }
    if (status < 200 || status >= 300) {
        log_msg("ERROR", "Server returned HTTP %ld", status);
        return -1;
    }
    return 0;
}
