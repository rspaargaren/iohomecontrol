#include <blind_position.h>
#include <esp_timer.h>
#include <Arduino.h>

namespace IOHC {
    BlindPosition::BlindPosition(uint32_t travelTimeMs)
            : state(State::Idle), travelTime(travelTimeMs), lastUpdateUs(0), position(0.0f) {}

    void BlindPosition::setTravelTime(uint32_t ms) { travelTime = ms; }

    uint32_t BlindPosition::getTravelTime() const { return travelTime; }

    void BlindPosition::startOpening() {
        update();
        Serial.printf("[BlindPosition] start opening (pos=%.1f%%)\n", position);
        state = State::Opening;
        lastUpdateUs = esp_timer_get_time();
    }

    void BlindPosition::startClosing() {
        update();
        Serial.printf("[BlindPosition] start closing (pos=%.1f%%)\n", position);
        state = State::Closing;
        lastUpdateUs = esp_timer_get_time();
    }

    void BlindPosition::stop() {
        update();
        Serial.printf("[BlindPosition] stop (pos=%.1f%%)\n", position);
        state = State::Idle;
    }

    void BlindPosition::update() {
        if (state == State::Idle || travelTime == 0) {
            lastUpdateUs = esp_timer_get_time();
            return;
        }
        uint64_t now = esp_timer_get_time();
        uint64_t elapsed = now - lastUpdateUs;
        float ratio = static_cast<float>(elapsed) / (static_cast<float>(travelTime) * 1000.0f);
        if (state == State::Opening) {
            position += ratio * (100.0f - position);
            if (position >= 100.0f || elapsed >= static_cast<uint64_t>(travelTime) * 1000ULL) {
                position = 100.0f;
                state = State::Idle;
            }
        } else if (state == State::Closing) {
            position -= ratio * position;
            if (position <= 0.0f || elapsed >= static_cast<uint64_t>(travelTime) * 1000ULL) {
                position = 0.0f;
                state = State::Idle;
            }
        }
        lastUpdateUs = now;
        Serial.printf("[BlindPosition] update (state=%d pos=%.1f%%)\n", static_cast<int>(state), position);
    }

    float BlindPosition::getPosition() const { return position; }
}
