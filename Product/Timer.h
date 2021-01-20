// A minimal timer utility, that simply gives the ability to
// call a function at a specied future time. There are lots of
// Timer libraries out there that can do much more - this one does less.
#pragma once
#ifdef UNITTEST
#include "UnitTest/MyHardware.h"
#else
#include "Hardware.h"
#endif

extern Hardware* hw;

class Timer {
public:
    Timer() : _when(0), _callback(0) {}

    // schedules a callback to occur at the specified time interval from now
    void start(void (*callback)(), unsigned int intervalMillis) {
        _when = hw->millis() + intervalMillis;
        _callback = callback;
    }

    // call this from loop()
    void update() {
        if (_when && hw->millis() > _when) {
            _when = 0;
            (*_callback)();
        }
    }

private:
    unsigned long _when;
    void (*_callback)();
};
