#define USE_US_TIMER

#if defined(ESP8266)
#include "c_types.h"
#include "eagle_soc.h"
#include "osapi.h"

#include "TickerUs.h"

namespace Timers
    {
    TickerUs::TickerUs()
        : _timer(nullptr) {}

    TickerUs::~TickerUs()
    {
        detach();
    }

    void TickerUs::_attach_ms(uint32_t milliseconds, bool repeat, callback_with_arg_t callback, void* arg)
    {
        if (_timer)
        {
            os_timer_disarm(_timer);
        }
        else
        {
            _timer = &_etsTimer;
        }

        os_timer_setfn(_timer, callback, arg);
        os_timer_arm(_timer, milliseconds, repeat);
    }

    void TickerUs::_attach_us(uint32_t microseconds, bool repeat, callback_with_arg_t callback, void* arg)
    {
        if (_timer)
        {
            os_timer_disarm(_timer);
        }
        else
        {
            _timer = &_etsTimer;
        }

        os_timer_setfn(_timer, callback, arg);
        os_timer_arm_us(_timer, microseconds, repeat);
    }

    void TickerUs::detach()
    {
        if (!_timer)
            return;

        os_timer_disarm(_timer);
        _timer = nullptr;
        _callback_function = nullptr;
    }

    bool TickerUs::active() const
    {
        return _timer;
    }

    void TickerUs::_static_callback(void* arg)
    {
        TickerUs* _this = reinterpret_cast<TickerUs*>(arg);
        if (_this && _this->_callback_function)
            _this->_callback_function();
    }
} 
#endif