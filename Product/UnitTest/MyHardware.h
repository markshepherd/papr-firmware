#pragma once
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "stdint.h"

class Hardware {
public:
    MOCK_METHOD(void, pinMode, (uint8_t pin, uint8_t mode), ());
    MOCK_METHOD(void, digitalWrite, (uint8_t pin, uint8_t val), ());
    MOCK_METHOD(int, digitalRead, (uint8_t pin), ());
    MOCK_METHOD(int, analogRead, (uint8_t pin), ());
    MOCK_METHOD(void, analogWrite, (uint8_t pin, int val), ());
    MOCK_METHOD(unsigned long, millis, (), ());
    MOCK_METHOD(void, delay, (unsigned long ms), ());
    MOCK_METHOD(void, delayMicroseconds, (unsigned int us), ());
    MOCK_METHOD(void, configurePins, (), ());
    MOCK_METHOD(void, initializeDevices, (), ());
};