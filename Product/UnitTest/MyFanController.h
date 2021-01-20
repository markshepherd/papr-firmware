#pragma once
#include <stdint.h>
typedef uint8_t byte;

class FanController {
public:
    FanController(byte sensorPin, unsigned int sensorThreshold, byte pwmPin = 0) : _sensorPin(sensorPin), _sensorThreshold(sensorThreshold), _pwmPin(pwmPin) {}

    MOCK_METHOD(void, begin, (), ());
    MOCK_METHOD(unsigned int, getSpeed, (), ());
    MOCK_METHOD(void, setDutyCycle, (byte dutyCycle), ());
    MOCK_METHOD(byte, getDutyCycle, (), ());

public:
    byte _sensorPin;
    unsigned int _sensorThreshold;
    byte _pwmPin;
};