#pragma once

/********************************************************************
 * Long Press detector for buttons
 ********************************************************************/

class LongPressDetector {
private:
    unsigned int _pin;
    unsigned long _longPressMillis;
    void (*_callback)(const int);
    int _currentState;
    unsigned long _pressMillis;
    bool _callbackCalled;

public:
    LongPressDetector(int pin, unsigned long longPressMillis) : _pin(pin), _longPressMillis(longPressMillis) {
        _currentState = digitalRead(_pin);
        _pressMillis = millis();
    }

    void update()
    {
        int state = digitalRead(_pin);

        if (state == BUTTON_PUSHED) {
            if (_currentState == BUTTON_PUSHED) {
                // The button is already pressed. See if the button has been pressed long enough to be a long press.
                if (!_callbackCalled && (millis() - _pressMillis > _longPressMillis)) {
                    _callback(state);
                    _callbackCalled = true;
                }
            } else if (_currentState == BUTTON_RELEASED) {
                // The button has just been pushed. Record the start time of this press.
                _pressMillis = millis();
                _callbackCalled = false;
            }
        }
        _currentState = state;
    }

    int state() {
        return digitalRead(_pin);
    }

    void onLongPress(void(*callback)(const int)) {
        _callback = callback;
    }
};
