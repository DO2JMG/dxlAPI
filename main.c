#define _POSIX_C_SOURCE 200809L
#include "app.h"
#include "config.h"
#include "log.h"
#include "position.h"
#include "rate_limit.h"
#include "telemetry.h"
#include "udp.h"
#include <curl/curl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t g_running = 1;

static void on_signal(int signo) {
    (void)signo;
    g_running = 0;
}

int main(int argc, char **argv) {
    config_t cfg;

    if (!parse_args(argc, argv, &cfg)) {
        usage(stderr);
        return 2;
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        log_msg("ERROR", "curl_global_init failed");
        return 1;
    }

    if (upload_receiver_position(&cfg) == 0) {
        log_msg("INFO", "Receiver position uploaded");
    } else {
        log_msg("ERROR", "Mandatory receiver position upload failed; stopping");
        curl_global_cleanup();
        return 1;
    }

    time_t next_position_upload = time(NULL) + POSITION_UPLOAD_INTERVAL_SEC;

    int sock = create_udp_socket(cfg.bind_addr, cfg.port);
    if (sock < 0) {
        curl_global_cleanup();
        return 1;
    }

    log_msg("INFO", "Listening on udp://%s:%d as %s", cfg.bind_addr, cfg.port, cfg.callsign);
    log_msg("INFO", "Altitude-dependent telemetry rate limit: <2000m=%ds, 2000-5000m=%ds, >5000m=%ds per sonde",
            UPLOAD_INTERVAL_LOW_ALT_SEC, UPLOAD_INTERVAL_MID_ALT_SEC, UPLOAD_INTERVAL_HIGH_ALT_SEC);
    log_msg("INFO", "Receiver position upload interval: every %d seconds", POSITION_UPLOAD_INTERVAL_SEC);

    while (g_running) {
        time_t now = time(NULL);

        if (now >= next_position_upload) {
            if (upload_receiver_position(&cfg) == 0) {
                log_msg("INFO", "Receiver position uploaded");
                next_position_upload = now + POSITION_UPLOAD_INTERVAL_SEC;
            } else {
                log_msg("ERROR", "Receiver position upload failed; retrying in %d seconds", POSITION_RETRY_INTERVAL_SEC);
                next_position_upload = now + POSITION_RETRY_INTERVAL_SEC;
            }
        }

        now = time(NULL);
        long wait_sec = (long)difftime(next_position_upload, now);
        if (wait_sec < 1) wait_sec = 1;
        if (wait_sec > 5) wait_sec = 5;

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        struct timeval tv;
        tv.tv_sec = wait_sec;
        tv.tv_usec = 0;

        int ready = select(sock + 1, &readfds, NULL, NULL, &tv);
        if (ready < 0) {
            if (errno == EINTR) continue;
            log_msg("ERROR", "select failed: %s", strerror(errno));
            continue;
        }
        if (ready == 0) continue;

        if (FD_ISSET(sock, &readfds)) {
            char buf[MAX_PACKET];
            ssize_t n = recvfrom(sock, buf, sizeof(buf) - 1, 0, NULL, NULL);
            if (n < 0) {
                if (errno == EINTR) continue;
                log_msg("ERROR", "recvfrom failed: %s", strerror(errno));
                continue;
            }
            buf[n] = '\0';
            if (cfg.verbose) log_msg("DEBUG", "Received: %s", buf);
            convert_and_upload_telemetry(buf, &cfg);
        }
    }

    log_msg("INFO", "Stopping");
    close(sock);
    curl_global_cleanup();
    return 0;
}
