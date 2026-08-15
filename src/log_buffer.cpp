#include <deque>
#include <vector>
#include <Arduino.h>
#include <log_buffer.h>
#include <user_config.h>

#if defined(WEBSERVER)
#include <web_server_handler.h>
#endif
#if defined(SYSLOG)
#include <syslog_helper.h>
#endif

namespace {
    std::deque<String> logDeque;
    const size_t MAX_LOG_ENTRIES = 50;

#if defined(WEBSERVER)
    // Keeps the WS log stream from flooding: always let through the rare,
    // genuinely one-off events (boot/crash diagnostics), always drop
    // debug/verbose chatter, and rate-limit everything else - including
    // packet decode traces - to at most one broadcast per 50ms. Packet
    // traces always contain the literal string "1W"/"2W" as their protocol
    // marker, so they must NOT be in the always-allow bucket: during a
    // burst of real RF traffic every trace line matches, defeating the
    // rate limit entirely and flooding/crashing WS clients (confirmed via
    // direct WS capture: 14 messages in 0.3s during live traffic, followed
    // by a corrupted frame and a dropped connection).
    bool shouldBroadcastLog(const String &msg) {
        if (msg.length() == 0) {
            return false;
        }

        if (msg.indexOf("Boot reset reason") >= 0 ||
            msg.indexOf("Last crash marker") >= 0) {
            return true;
        }

#if CONFIG_LOG_COLORS
        const size_t logLevelPosition = 7;
#else
        const size_t logLevelPosition = 0;
#endif /* CONFIG_LOG_COLORS */

        if (msg[logLevelPosition] == 'D' || msg[logLevelPosition] == 'V') {
            return false;
        }
        
        static unsigned long lastBroadcastMs = 0;
        const unsigned long now = millis();
        if (now - lastBroadcastMs < 50) {
            return false;
        }
        lastBroadcastMs = now;
        return true;
    }
#endif
}

void addLogMessage(const String &msg) {
    if (logDeque.size() >= MAX_LOG_ENTRIES) {
        logDeque.pop_front();
    }
    logDeque.push_back(msg);
#if defined(WEBSERVER)
    if (shouldBroadcastLog(msg)) {
        broadcastLog(msg);
    }
#endif
#if defined(SYSLOG)
    sendSyslog(msg);
#endif
}

std::vector<String> getLogMessages() {
    return std::vector<String>(logDeque.begin(), logDeque.end());
}
