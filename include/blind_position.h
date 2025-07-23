#ifndef IOHC_BLIND_POSITION_H
#define IOHC_BLIND_POSITION_H

#include <stdint.h>

namespace IOHC {
    class BlindPosition {
    public:
        explicit BlindPosition(uint32_t travelTimeMs = 0);

        void setTravelTime(uint32_t ms);
        uint32_t getTravelTime() const;

        void startOpening();
        void startClosing();
        void stop();
        void update();

        float getPosition() const;
        bool isMoving() const;

    private:
        enum class State { Idle, Opening, Closing };
        State state;
        uint32_t travelTime; // milliseconds
        uint64_t lastUpdateUs;
        float position; // 0..100
    };
}

#endif // IOHC_BLIND_POSITION_H
