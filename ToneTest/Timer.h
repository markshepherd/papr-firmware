#pragma once

// A minimal timer utility, that simply gives the ability to
// call a function at a specied future time. There are lots of
// Timer libraries out there that can do much more - this one does less.

// This is based on the Product version of Timer, with a couple of things added. Todo: merge this into Product.

class Timer {
public:
    Timer() : _when(0) {}

    // schedule a callback to occur at the specified interval from now
    void startMillis(void (*callback)(), unsigned long intervalMillis)
    {
        _when = micros() + (intervalMillis * 1000);
        _callback = callback;
    }
    void startMicros(void (*callback)(), unsigned long intervalMicros)
    {
        _when = micros() + intervalMicros;
        _callback = callback;
    }

    void cancel()
    {
        _when = 0;
    }

    // call this from loop()
    void update() {
        if (_when && micros() > _when) {
            _when = 0;
            (*_callback)();
        }
    }

private:
    unsigned long _when; // in microseconds
    void (*_callback)();
};
