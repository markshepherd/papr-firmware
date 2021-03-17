/*
 * Hardware.h
 *
 * This file provides all the hardware-related functionality that the app needs,
 * including generic access to IO pins and timing, and PAPR- or MCU-specific functions
 * such as configuring pins, initializing the hardware, and using the watchdog timer.
 * 
 * There will be 2 different implementations of this API
 * - the product version, which runs on a PAPR board with MCU
 * - the unit test version, which runs on a PC and pretends to be a PAPR and MCU
 */
#pragma once
#include "arduino.h"
#include <avr/wdt.h> 

class Hardware {
public:
    // Covers for Arduino API functions.
    inline void pinMode(uint8_t pin, uint8_t mode) { ::pinMode(pin, mode); }
    inline void digitalWrite(uint8_t pin, uint8_t val) { ::digitalWrite(pin, val); }
    inline int digitalRead(uint8_t pin) { return ::digitalRead(pin); }
    inline int analogRead(uint8_t pin) { return ::analogRead(pin); }
    inline void analogWrite(uint8_t pin, int val) { ::analogWrite(pin, val); }
    inline unsigned long millis(void) { return ::millis(); }
    inline unsigned long micros(void) { return ::micros(); }
    inline void delay(unsigned long ms) { ::delay(ms); }
    inline void delayMicroseconds(unsigned int us) { ::delayMicroseconds(us); }
    inline void wdt_enable(const uint8_t value) { ::wdt_enable(value); }
    inline void wdt_disable() { ::wdt_disable(); }
    inline void wdt_reset_() { wdt_reset(); } // wdt_reset is a macro so we can't use "::"

    // PAPR-specific hardware functions
    void configurePins();
    void initializeDevices();

    // Microcontroller-specific hardware functions
    void reset();
    int watchdogStartup(void);
    void setClockPrescaler(int prescalerSelect);
    void setPowerButtonInterruptCallback(void (*callback) ());
};