//#include <board-config.h.org>

#define USE_US_TIMER

#include "TickerUsESP32.h"
#include <thread>
#include <chrono>

#define TIMER_DIVIDER   (16)

namespace TimersUS {
    TickerUsESP32::TickerUsESP32() : _timer(nullptr) {
    }

    TickerUsESP32::~TickerUsESP32() {
        detach();
    }

    void TickerUsESP32::_attach_ms(uint32_t milliseconds, bool repeat, callback_with_arg_t callback, uint32_t arg) {

        esp_timer_create_args_t _timerConfig;
        _timerConfig.arg = reinterpret_cast<void *>(arg);
        _timerConfig.callback = callback;
        _timerConfig.dispatch_method = ESP_TIMER_TASK; //ESP_TIMER_ISR; //
        _timerConfig.skip_unhandled_events = false; //true;
        _timerConfig.name = "TickerMsESP32";
        if (_timer) {
            ESP_ERROR_CHECK(esp_timer_stop(_timer));
            ESP_ERROR_CHECK(esp_timer_delete(_timer));
        }
        ESP_ERROR_CHECK(esp_timer_create(&_timerConfig, &_timer));
        if (repeat) {
            //        printf("Packet %u Periodic\n", milliseconds);
            ESP_ERROR_CHECK(esp_timer_start_periodic(_timer, milliseconds * 1000ULL));
        }
        else {
            ESP_ERROR_CHECK(esp_timer_start_once(_timer, milliseconds * 1000ULL));
        }
    }

    // Added delayed task
    void TickerUsESP32::_delay_ms(uint32_t milliseconds, bool repeat, callback_with_arg_t callback, uint32_t arg) {

        esp_timer_create_args_t _timerConfig;
        _timerConfig.arg = reinterpret_cast<void *>(arg);
        _timerConfig.callback = callback;
        _timerConfig.dispatch_method = ESP_TIMER_TASK; //ESP_TIMER_ISR; // But ISR doesnt work with 100ULL
        //    _timerConfig.skip_unhandled_events = true;
        _timerConfig.name = "TickerMsESP32Delay";
        if (_timer_delayed) {
            ESP_ERROR_CHECK(esp_timer_delete(_timer_delayed));
        }
        ESP_ERROR_CHECK(esp_timer_create(&_timerConfig, &_timer_delayed));
        ESP_ERROR_CHECK(esp_timer_start_once(_timer_delayed, milliseconds * 1000ULL));
    }

    // Added as uS
    void TickerUsESP32::_attach_us(uint64_t microseconds, bool repeat, callback_with_arg_t callback, uint32_t arg) {
        esp_timer_create_args_t _timerConfig;
        _timerConfig.arg = reinterpret_cast<void *>(arg);
        _timerConfig.callback = callback;
        _timerConfig.dispatch_method = ESP_TIMER_TASK; //ESP_TIMER_ISR; // But ISR doesnt work with 100ULL
        _timerConfig.skip_unhandled_events = true;
        _timerConfig.name = "TickerUsESP32";

        if (_timer) {
            ESP_ERROR_CHECK(esp_timer_stop(_timer));
            ESP_ERROR_CHECK(esp_timer_delete(_timer));
        }

        ESP_ERROR_CHECK(esp_timer_create(&_timerConfig, &_timer));
        if (repeat) {
            ESP_ERROR_CHECK(esp_timer_start_periodic(_timer, microseconds));
        }
        else {
            ESP_ERROR_CHECK(esp_timer_start_once(_timer, microseconds));
        }
    }

    void TickerUsESP32::detach() {
        if (_timer) {
            ESP_ERROR_CHECK(esp_timer_stop(_timer));
            ESP_ERROR_CHECK(esp_timer_delete(_timer));
            _timer = nullptr;
        }
        //     if (_timer_delayed) {
        //     ESP_ERROR_CHECK(esp_timer_stop(_timer_delayed));
        //     ESP_ERROR_CHECK(esp_timer_delete(_timer_delayed));
        //     _timer = nullptr;
        // }
    }

    bool TickerUsESP32::active() {
        if (!_timer) return false;
        return esp_timer_is_active(_timer);
    }
}

//#endif
