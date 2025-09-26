#include "syslog_helper.h"
#include <user_config.h>   // provides SYSLOG, syslog_server, syslog_port

#if defined(SYSLOG)

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>
#include <esp_log.h>

// ===== Config (adjust if you like) =====
#ifndef SYSLOG_FACILITY
#define SYSLOG_FACILITY 16           // local0
#endif

#ifndef SYSLOG_APP
#define SYSLOG_APP "MIOPENIO"        // rsyslog will use this as %PROGRAMNAME%
#endif

// Define SYSLOG_RFC5424 to send RFC5424 instead of RFC3164
// #define SYSLOG_RFC5424

namespace {
    WiFiUDP      syslogUdp;
    IPAddress    syslogIP;
    bool         syslogReady = false;
    static const char *TAG   = "SYSLOG";

    inline int pri(int facility, int severity) {
        if (severity < 0) severity = 6;     // default info
        if (severity > 7) severity = 7;
        return facility * 8 + severity;
    }

#ifndef SYSLOG_RFC5424
    // RFC3164 timestamp: "Jan  2 15:04:05" (local time)
    String rfc3164Timestamp() {
        time_t now = time(nullptr);
        struct tm tmnow;
        localtime_r(&now, &tmnow);
        char buf[32];
        strftime(buf, sizeof(buf), "%b %e %T", &tmnow);
        return String(buf);
    }
#else
    // RFC5424 timestamp: "YYYY-MM-DDTHH:MM:SSZ" (UTC)
    String iso8601UTC() {
        time_t now = time(nullptr);
        struct tm tmnow;
        gmtime_r(&now, &tmnow);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tmnow);
        return String(buf);
    }
#endif

    String currentHostIdent() {
        const char *h = WiFi.getHostname();
        if (h && *h) return String(h);
        return WiFi.localIP().toString();
    }
}

// Initialize UDP + resolve syslog IP from user_config.h
void initSyslog() {
    ESP_LOGD(TAG, "Init syslog: server='%s' port=%u", syslog_server.c_str(), syslog_port);
    if (syslogReady) {
        ESP_LOGD(TAG, "Syslog already initialized");
        return;
    }
    if (syslog_server.empty()) {
        ESP_LOGD(TAG, "Syslog server not set");
        return;
    }
    if (!syslogIP.fromString(syslog_server.c_str())) {
        ESP_LOGD(TAG, "Invalid syslog server address: %s", syslog_server.c_str());
        return;
    }
    syslogUdp.begin(0);
    syslogReady = true;
    ESP_LOGD(TAG, "Syslog initialized with IP: %s", syslogIP.toString().c_str());
}

// Real sender with RFC header
void sendSyslog(const String &msg, int severity) {
    if (!syslogReady) {
        ESP_LOGD(TAG, "Syslog not ready, initializing");
        initSyslog();
    }
    if (!syslogReady) {
        ESP_LOGD(TAG, "Syslog initialization failed");
        return;
    }

    const int p     = pri(SYSLOG_FACILITY, severity);
    const String ho = currentHostIdent();

#ifndef SYSLOG_RFC5424
    // RFC3164: <PRI>MMM dd HH:MM:SS HOST APP: message
    const String header = "<" + String(p) + ">" + rfc3164Timestamp() + " " + ho + " " + SYSLOG_APP + ": ";
    const String wire   = header + msg;
#else
    // RFC5424: <PRI>1 TIMESTAMP HOST APP PROCID MSGID - message
    const char* PROCID = "-";   // e.g., chip ID if you want
    const char* MSGID  = "-";
    const String header = "<" + String(p) + ">1 " + iso8601UTC() + " " + ho + " " + SYSLOG_APP
                        + " " + PROCID + " " + MSGID + " - ";
    const String wire   = header + msg;
#endif

    ESP_LOGD(TAG, "Sending syslog (len=%u): %s", wire.length(), wire.c_str());
    syslogUdp.beginPacket(syslogIP, syslog_port);
    syslogUdp.write(reinterpret_cast<const uint8_t*>(wire.c_str()), wire.length());
    int result = syslogUdp.endPacket();
    ESP_LOGD(TAG, "Message send result: %d", result);
}

// Legacy overload without severity (defaults to info)
void sendSyslog(const String &msg) {
    sendSyslog(msg, 6);
}

#else  // !SYSLOG

// No-op definitions so you can build without SYSLOG
void initSyslog() {}
void sendSyslog(const String &) {}
void sendSyslog(const String &, int) {}


#endif // SYSLOG
