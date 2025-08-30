#include <deque>
#include <vector>
#include <Arduino.h>
#include <log_buffer.h>
#if defined(WEBSERVER)
#include <web_server_handler.h>
#endif
#if defined(SYSLOG)
#include <syslog_helper.h>
#endif

namespace {
    std::deque<String> logDeque;
    const size_t MAX_LOG_ENTRIES = 50;
}

void addLogMessage(const String &msg) {
    if (logDeque.size() >= MAX_LOG_ENTRIES) {
        logDeque.pop_front();
    }
    logDeque.push_back(msg);
#if defined(WEBSERVER)
    broadcastLog(msg);
#endif
#if defined(SYSLOG)
    sendSyslog(msg);
#endif
}

std::vector<String> getLogMessages() {
    return std::vector<String>(logDeque.begin(), logDeque.end());
}
