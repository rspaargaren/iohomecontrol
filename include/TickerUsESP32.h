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

/* This is the code of this library https://github.com/espressif/arduino-esp32/blob/master/libraries/Ticker/src/Ticker.h */
#ifndef TICKERUSESP32_H
#define TICKERUSESP32_H

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "esp_timer.h"
}

void system_timer_reinit();
#define INIT_US system_timer_reinit();

namespace TimersUS {
    class TickerUsESP32 {
    public:
    TickerUsESP32();
    ~TickerUsESP32();
    typedef void (*callback_t)();
    typedef void (*callback_with_arg_t)(void*);

    void setMaxIterations(uint32_t maxIterations) {
        _maxIterations = maxIterations;
    }

    void attach(uint32_t seconds, callback_t callback) {
        _attach_ms(seconds * 1000, true, reinterpret_cast<callback_with_arg_t>(callback), 0);
    }
    void attach_ms(uint32_t milliseconds, callback_t callback) {
        _attach_ms(milliseconds, true, reinterpret_cast<callback_with_arg_t>(callback), 0);
    }
    // Added delayed task
    void delay_ms(uint32_t milliseconds, callback_t callback) {
        _delay_ms(milliseconds, false, reinterpret_cast<callback_with_arg_t>(callback), 0);
    }
    // Added as uS
    void attach_us(uint64_t microseconds, callback_t callback) {
        _attach_us(microseconds, true, reinterpret_cast<callback_with_arg_t>(callback), 0);
    }
    template<typename TArg>
    void attach(uint32_t seconds, void (*callback)(TArg), TArg arg) {
        static_assert(sizeof(TArg) <= sizeof(uint32_t), "attach() callback argument size must be <= 4 bytes");
        // C-cast serves two purposes:
        // static_cast for smaller integer types,
        // reinterpret_cast + const_cast for pointer types
        auto arg32 = (uint32_t)arg;
        _attach_ms(seconds * 1000, true, reinterpret_cast<callback_with_arg_t>(callback), arg32);
    }
    template<typename TArg>
    void attach_ms(uint32_t milliseconds, void (*callback)(TArg), TArg arg) {
        static_assert(sizeof(TArg) <= sizeof(uint32_t), "attach_ms() callback argument size must be <= 4 bytes");
        auto arg32 = (uint32_t)arg;
        _attach_ms(milliseconds, true, reinterpret_cast<callback_with_arg_t>(callback), arg32);
    }
    // Added delayed task
    template<typename TArg>
    void delay_ms(uint32_t milliseconds, void (*callback)(TArg), TArg arg) {
        static_assert(sizeof(TArg) <= sizeof(uint32_t), "delay_ms() callback argument size must be <= 4 bytes");
        auto arg32 = (uint32_t)arg;
        _delay_ms(milliseconds, false, reinterpret_cast<callback_with_arg_t>(callback), arg32);
    }
    // Added as uS
    template<typename TArg>
    void attach_us(uint64_t microseconds, void (*callback)(TArg), TArg arg) {
        static_assert(sizeof(TArg) <= sizeof(uint32_t), "attach_us() callback argument size must be <= 4 bytes");
        auto arg32 = (uint32_t)arg;
        _attach_us(microseconds, true, reinterpret_cast<callback_with_arg_t>(callback), arg32);
    }

    void detach();
    bool active();

    private:
        uint32_t _iterationCount = 0;
        uint32_t _maxIterations = UINT32_MAX;  // Par défaut, pas de limite d'itérations

    protected:
    void _attach_ms(uint32_t milliseconds, bool repeat, callback_with_arg_t callback, uint32_t arg);
    // Added delayed task
    void _delay_ms(uint32_t milliseconds, bool repeat, callback_with_arg_t callback, uint32_t arg);
    // Added as uS
    void _attach_us(uint64_t microseconds, bool repeat, callback_with_arg_t callback, uint32_t arg);

    esp_timer_handle_t _timer;
    esp_timer_handle_t _timer_delayed{};
    };
}

#endif // TICKERUSESP32_H
