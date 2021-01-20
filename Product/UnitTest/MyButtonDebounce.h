#pragma once
#include "gmock/gmock.h"
#include "gtest/gtest.h"

class ButtonDebounce {
public:
    ButtonDebounce(int pin, unsigned long delay) : _callback(0), _pin(pin), _delay(delay) {}

    void setCallback(void (*callback)(int))
    {
        _callback = callback;
        setCallback_();
    }

    MOCK_METHOD(void, update, (), ());
    MOCK_METHOD(int, state, (), ());
    MOCK_METHOD(void, setCallback_, (), ());

public:
    void (*_callback)(int);
    int _pin;
    unsigned long _delay;
};