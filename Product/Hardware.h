/*
 * Hardware.h
 *
 * This file provides all the hardware-related functionality that the app needs,
 * including generic access to IO pins and timing, and PAPR-specific functions for
 * configuring pins and initializing the hardware to a default state.
 * 
 * There will be 2 different implementations of this API
 * - the normal version, which runs on the microcontroller and uses the Arduino API.
 * - the unit test version, which runs on a PC and pretends to be a microcontroller.
 */
#pragma once
#include "arduino.h"

class Hardware {
public:
    Hardware() {}

    // Covers for Arduino API functions.
    inline void pinMode(uint8_t pin, uint8_t mode) { ::pinMode(pin, mode); }
    inline void digitalWrite(uint8_t pin, uint8_t val) { ::digitalWrite(pin, val); }
    inline int digitalRead(uint8_t pin) { return ::digitalRead(pin); }
    inline int analogRead(uint8_t pin) { return ::analogRead(pin); }
    inline void analogWrite(uint8_t pin, int val) { ::analogWrite(pin, val); }
    inline unsigned long millis(void) { return ::millis(); }
    inline void delay(unsigned long ms) { ::delay(ms); }
    inline void delayMicroseconds(unsigned int us) { ::delayMicroseconds(us); }

    // PAPR-specific hardware.
    void configurePins();
    void initializeDevices();
};