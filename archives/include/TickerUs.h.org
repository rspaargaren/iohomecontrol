#include <functional>
#include <Schedule.h>
#include <ets_sys.h>

void system_timer_reinit();
#define INIT_US system_timer_reinit();

namespace Timers {
    class TickerUs {
    public:
        TickerUs();
        ~TickerUs();

        typedef void (*callback_with_arg_t)(void*);
        typedef std::function<void(void)> callback_function_t;

        // callback will be called at following loop() after ticker fires
        void attach_scheduled(float seconds, callback_function_t callback)  {
            _callback_function = [callback]() { schedule_function(callback); };
            _attach_ms(1000UL * seconds, true);
        }

        // callback will be called in SYS ctx when ticker fires
        void attach(float seconds, callback_function_t callback) {
            _callback_function = std::move(callback);
            _attach_ms(1000UL * seconds, true);
        }

        // callback will be called at following loop() after ticker fires
        void attach_ms_scheduled(uint32_t milliseconds, callback_function_t callback) {
            _callback_function = [callback]() { schedule_function(callback); };
            _attach_ms(milliseconds, true);
        }

        // callback will be called at following yield() after ticker fires
        void attach_ms_scheduled_accurate(uint32_t milliseconds, callback_function_t callback) {
            _callback_function = [callback]() { schedule_recurrent_function_us([callback]() { callback(); return false; }, 0); };
            _attach_ms(milliseconds, true);
        }

        // callback will be called in SYS ctx when ticker fires
        void attach_ms(uint32_t milliseconds, callback_function_t callback)  {
            _callback_function = std::move(callback);
            _attach_ms(milliseconds, true);
        }

        // --- Us block
        // callback will be called at following loop() after ticker fires
        void attach_us_scheduled(uint32_t microseconds, callback_function_t callback) {
            _callback_function = [callback]() { schedule_function(callback); };
            _attach_us(microseconds, true);
        }

        // callback will be called at following yield() after ticker fires
        void attach_us_scheduled_accurate(uint32_t microseconds, callback_function_t callback) {
            _callback_function = [callback]() { schedule_recurrent_function_us([callback]() { callback(); return false; }, 0); };
            _attach_us(microseconds, true);
        }

        // callback will be called in SYS ctx when ticker fires
        void attach_us(uint32_t microseconds, callback_function_t callback) {
            _callback_function = std::move(callback);
            _attach_us(microseconds, true);
        }
        // --- Us block


        // callback will be called in SYS ctx when ticker fires
        template<typename TArg>
        void attach(float seconds, void (*callback)(TArg), TArg arg)  {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-function-type"
            static_assert(sizeof(TArg) <= sizeof(void*), "attach() callback argument size must be <= sizeof(void*)");
            _attach_ms(1000UL * seconds, true, reinterpret_cast<callback_with_arg_t>(callback), reinterpret_cast<void*>(arg));
    #pragma GCC diagnostic pop
        }

        // callback will be called in SYS ctx when ticker fires
        template<typename TArg>
        void attach_ms(uint32_t milliseconds, void (*callback)(TArg), TArg arg) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-function-type"
            static_assert(sizeof(TArg) <= sizeof(void*), "attach() callback argument size must be <= sizeof(void*)");
            _attach_ms(milliseconds, true, reinterpret_cast<callback_with_arg_t>(callback), reinterpret_cast<void*>(arg));
    #pragma GCC diagnostic pop
        }


        // --- Us block
        // callback will be called in SYS ctx when ticker fires
        template<typename TArg>
        void attach_us(uint32_t microseconds, void (*callback)(TArg), TArg arg) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wcast-function-type"
            static_assert(sizeof(TArg) <= sizeof(void*), "attach() callback argument size must be <= sizeof(void*)");
            _attach_us(microseconds, true, reinterpret_cast<callback_with_arg_t>(callback), reinterpret_cast<void*>(arg));
    #pragma GCC diagnostic pop
        }
        // --- Us block


        // callback will be called at following loop() after ticker fires
        void once_scheduled(float seconds, callback_function_t callback) {
            _callback_function = [callback]() { schedule_function(callback); };
            _attach_ms(1000UL * seconds, false);
        }

        // callback will be called in SYS ctx when ticker fires
        void once(float seconds, callback_function_t callback) {
            _callback_function = std::move(callback);
            _attach_ms(1000UL * seconds, false);
        }

        // callback will be called at following loop() after ticker fires
        void once_ms_scheduled(uint32_t milliseconds, callback_function_t callback) {
            _callback_function = [callback]() { schedule_function(callback); };
            _attach_ms(milliseconds, false);
        }

        // callback will be called in SYS ctx when ticker fires
        void once_ms(uint32_t milliseconds, callback_function_t callback) {
            _callback_function = std::move(callback);
            _attach_ms(milliseconds, false);
        }

        // --- Us block
        // callback will be called at following loop() after ticker fires
        void once_us_scheduled(uint32_t microseconds, callback_function_t callback) {
            _callback_function = [callback]() { schedule_function(callback); };
            _attach_us(microseconds, false);
        }

        // callback will be called in SYS ctx when ticker fires
        void once_us(uint32_t microseconds, callback_function_t callback) {
            _callback_function = std::move(callback);
            _attach_us(microseconds, false);
        }
        // --- Us block


        // callback will be called in SYS ctx when ticker fires
        template<typename TArg>
        void once(float seconds, void (*callback)(TArg), TArg arg) {
            static_assert(sizeof(TArg) <= sizeof(void*), "attach() callback argument size must be <= sizeof(void*)");
            _attach_ms(1000UL * seconds, false, reinterpret_cast<callback_with_arg_t>(callback), reinterpret_cast<void*>(arg));
        }

        // callback will be called in SYS ctx when ticker fires
        template<typename TArg>
        void once_ms(uint32_t milliseconds, void (*callback)(TArg), TArg arg) {
            static_assert(sizeof(TArg) <= sizeof(void*), "attach() callback argument size must be <= sizeof(void*)");
            _attach_ms(milliseconds, false, reinterpret_cast<callback_with_arg_t>(callback), reinterpret_cast<void*>(arg));
        }


        // --- Us block
        // callback will be called in SYS ctx when ticker fires
        template<typename TArg>
        void once_us(uint32_t microseconds, void (*callback)(TArg), TArg arg) {
            static_assert(sizeof(TArg) <= sizeof(void*), "attach() callback argument size must be <= sizeof(void*)");
            _attach_us(microseconds, false, reinterpret_cast<callback_with_arg_t>(callback), reinterpret_cast<void*>(arg));
        }
        // --- Us block


        void detach();
        bool active() const;

    protected:
        static void _static_callback(void* arg);
        void _attach_ms(uint32_t milliseconds, bool repeat, callback_with_arg_t callback, void* arg);
        void _attach_ms(uint32_t milliseconds, bool repeat)
        {
            _attach_ms(milliseconds, repeat, _static_callback, this);
        }
        void _attach_us(uint32_t microseconds, bool repeat, callback_with_arg_t callback, void* arg);
        void _attach_us(uint32_t microseconds, bool repeat)
        {
            _attach_us(microseconds, repeat, _static_callback, this);
        }

        ETSTimer* _timer;
        callback_function_t _callback_function = nullptr;

    private:
        ETSTimer _etsTimer;
    };
}