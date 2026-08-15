#include <blind_position.h>
#include <esp_timer.h>
#include <Arduino.h>
#include <algorithm>

#define LOG_TAG "blind_position"

namespace IOHC {
    BlindPosition::BlindPosition(uint32_t travelTimeSec)
            : state(State::Idle), travelTime(travelTimeSec), lastUpdateUs(0), position(0.0f) {}

    void BlindPosition::setTravelTime(uint32_t sec) { travelTime = sec; }

    uint32_t BlindPosition::getTravelTime() const { return travelTime; }

    void BlindPosition::startOpening() {
        update();
        ESP_LOGI(LOG_TAG, "start opening (pos=%.1f%%)", position);
        state = State::Opening;
        lastUpdateUs = esp_timer_get_time();
    }

    void BlindPosition::startClosing() {
        update();
        ESP_LOGI(LOG_TAG, "start closing (pos=%.1f%%)", position);
        state = State::Closing;
        lastUpdateUs = esp_timer_get_time();
    }

    void BlindPosition::stop() {
        update();
        ESP_LOGI(LOG_TAG, "stop (pos=%.1f%%)", position);
        state = State::Idle;
    }

    void BlindPosition::update() {
    if (state == State::Idle || travelTime == 0) {
        lastUpdateUs = esp_timer_get_time();
        return;
    }

    uint64_t now = esp_timer_get_time();
    uint64_t elapsed = now - lastUpdateUs;
    float delta = static_cast<float>(elapsed) * 100.0f /
                  (static_cast<float>(travelTime) * 1000000.0f);

    if (state == State::Opening) {
        position += delta;
        if (position >= 99.5f) {  // margin to avoid floating point rounding issue
            position = 100.0f;
            state = State::Idle;
        }
    } else if (state == State::Closing) {
        position -= delta;
        if (position <= 0.5f) {  // margin for rounding
            position = 0.0f;
            state = State::Idle;
        }
    }

    // Clamp position to [0, 100]
    position = std::clamp(position, 0.0f, 100.0f);

    lastUpdateUs = now;
    ESP_LOGI(LOG_TAG, "update (state=%d pos=%.1f%%)", static_cast<int>(state), position);
}

    float BlindPosition::getPosition() const { return position; }

    bool BlindPosition::isMoving() const { return state != State::Idle; }

    void BlindPosition::setPosition(float pos) {
        position = std::clamp(pos, 0.0f, 100.0f);
        lastUpdateUs = esp_timer_get_time();
    }
}
