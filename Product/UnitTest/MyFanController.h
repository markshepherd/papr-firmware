#pragma once
#include <stdint.h>
typedef uint8_t byte;

class FanController {
public:
    FanController() {}

    MOCK_METHOD(void, begin, (), ());
    MOCK_METHOD(unsigned int, getSpeed, (), ());
    MOCK_METHOD(void, setDutyCycle, (byte dutyCycle), ());
    MOCK_METHOD(byte, getDutyCycle, (), ());
};