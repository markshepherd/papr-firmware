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
#include "stdint.h"

class Hardware {
public:
    static Hardware& instance();

    // Covers for Arduino API functions.
    void pinMode(uint8_t pin, uint8_t mode);
    void digitalWrite(uint8_t pin, uint8_t val);
    int digitalRead(uint8_t pin);
    int analogRead(uint8_t pin);
    void analogWrite(uint8_t pin, int val);
    unsigned long millis(void);
    void delay(unsigned long ms);
    void delayMicroseconds(unsigned int us);

    // PAPR-specific hardware.
    void configurePins();
    void initializeDevices();

private:
    Hardware() {}
    static Hardware hardwareInstance; // our singleton instance
};