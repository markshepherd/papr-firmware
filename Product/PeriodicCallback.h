// A minimal timer utility, that simply gives the ability to
// call a function at a specied future time. There are lots of
// timer libraries out there that can do much more - this one does less.
#pragma once

extern unsigned long getMillis();

class PeriodicCallback {
public:
    PeriodicCallback(unsigned long intervalMillis, void (*callback)()) : _intervalMillis(intervalMillis), _callback(callback), _active(false) {}

    void start() {
        _active = true;
        _intervalStartMillis = getMillis();
    }

    void update() {
        unsigned long now = getMillis();
        if (_active && ((now - _intervalStartMillis) > _intervalMillis)) {
            _intervalStartMillis = now;
            (*_callback)();
        }
    }

    void cancel() {
        _active = false;
    }

    bool isActive() {
        return _active;
    }

private:
    unsigned long _intervalMillis;
    unsigned long _intervalStartMillis;
    bool _active;
    void (*_callback)();
};
