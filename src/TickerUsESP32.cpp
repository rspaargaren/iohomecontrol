/*
   Copyright (c) 2024. CRIDP https://github.com/cridp

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

           http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

//#include <board-config.h>

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

    /**
    * @brief Check if ESP timer is active. This is used to prevent an attacker from trying to re - start the timer when it's time to do something other than reset it to the default value.
    * @return True if timer is active false otherwise ( no timer
    */
    bool TickerUsESP32::active() {
        // Returns true if the timer is not currently running.
        if (!_timer) return false;
        return esp_timer_is_active(_timer);
    }
}

//#endif
