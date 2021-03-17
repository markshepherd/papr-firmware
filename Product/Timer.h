// A minimal timer utility, that simply gives the ability to
// call a function at a specied future time. There are lots of
// Timer libraries out there that can do much more - this one does less.
#pragma once

extern unsigned long getMillis();

class Timer {
public:
    Timer(void (*callback)()) : _when(0), _callback(callback) {}

    // schedules a callback to occur at the specified time interval from now
    void start(unsigned int intervalMillis) {
        _when = getMillis() + intervalMillis;
        if (_when == 0) _when = 1;
    }

    // call this from loop()
    void update() {
        if (_when && getMillis() > _when) {
            _when = 0;
            (*_callback)();
        }
    }

    void cancel() {
        _when = 0;
    }

private:
    unsigned long _when;
    void (*_callback)();
};
