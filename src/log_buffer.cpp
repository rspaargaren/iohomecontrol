#include <deque>
#include <vector>
#include <Arduino.h>
#include <log_buffer.h>
#if defined(WEBSERVER)
#include <web_server_handler.h>
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
}

std::vector<String> getLogMessages() {
    return std::vector<String>(logDeque.begin(), logDeque.end());
}
