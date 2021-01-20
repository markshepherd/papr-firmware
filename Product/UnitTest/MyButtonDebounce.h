#pragma once
#include "gmock/gmock.h"
#include "gtest/gtest.h"

class ButtonDebounce {
public:
    ButtonDebounce() : _callback(0) {}

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
};