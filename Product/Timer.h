// A minimal timer utility, that simply gives the ability to
// call a function at a specied future time. There are lots of
// Timer libraries out there that can do much more - this one does less.
#pragma once

extern unsigned long getMillis();

class Timer {
public:
    Timer(void (*callback)()) : _intervalMillis(0), _callback(callback) {}

    // schedules a callback to occur at the specified time interval from now
    void start(unsigned int intervalMillis) {
        _intervalMillis = intervalMillis;
        _intervalStartMillis = getMillis();
    }

    // call this from loop()
    void update() {
        if (_intervalMillis && ((getMillis() - _intervalStartMillis) > _intervalMillis)) {
            _intervalMillis = 0;
            (*_callback)();
        }
    }

    void cancel() {
        _intervalMillis = 0;
    }

private:
    unsigned long _intervalMillis;
    unsigned long _intervalStartMillis;
    void (*_callback)();
};
