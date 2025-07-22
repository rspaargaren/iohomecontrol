#include <deque>
#include <vector>
#include <Arduino.h>
#include <log_buffer.h>

namespace {
    std::deque<String> logDeque;
    const size_t MAX_LOG_ENTRIES = 50;
}

void addLogMessage(const String &msg) {
    if (logDeque.size() >= MAX_LOG_ENTRIES) {
        logDeque.pop_front();
    }
    logDeque.push_back(msg);
}

std::vector<String> getLogMessages() {
    return std::vector<String>(logDeque.begin(), logDeque.end());
}
