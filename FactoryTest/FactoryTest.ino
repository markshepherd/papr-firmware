/*
*   FactoryTest.ino
*
* This file is the main program for factory testing of the v2 PAPR board.
*
*/

#include "PAPRHwDefs.h"
#include "ButtonDebounce.h"

// A list of all the LEDs, from left to right as they appear on the box.
byte LEDpins[] = {
    BATTERY_LED_1_PIN,
    BATTERY_LED_2_PIN,
    BATTERY_LED_3_PIN,
    Error_LED_PIN,
    MODE_LED_1_PIN,
    MODE_LED_2_PIN,
    MODE_LED_3_PIN
};
int numLEDs = sizeof(LEDpins) / sizeof(byte);

#define DELAY_100ms 100
#define DELAY_200ms 200

// These objects monitor the buttons and give a callback when the button state changes.
ButtonDebounce buttonFanUp(FAN_UP_PIN, DELAY_100ms);
ButtonDebounce buttonFanDown(FAN_DOWN_PIN, DELAY_100ms);

//================================================================

// Turn an LED on, then off again.
void flashLED(byte pin, unsigned long duration) {
    digitalWrite(pin, LOW);
    delay(duration);
    digitalWrite(pin, HIGH);
}

// Turn all the LEDs on, then off again.
void flashLEDs(unsigned long duration) {
    // Turn on all LEDs
    for (int i = 0; i < numLEDs; i += 1) {
        digitalWrite(LEDpins[i], LOW);
    }

    delay(duration);

    // Turn off all LEDs
    for (int i = 0; i < numLEDs; i += 1) {
        digitalWrite(LEDpins[i], HIGH);
    }

    delay(duration);
}

// 
void onButtonUpChange(const int state) {
    if (state == HIGH) {
    }
}

void onButtonDownChange(const int state) {
    if (state == HIGH) {
    }
}

void setup() {
    // Initialize the hardware
    configurePins();
    initializeDevices();

    // Initialize the button scanners
    buttonFanUp.setCallback(onButtonUpChange);
    buttonFanDown.setCallback(onButtonDownChange);

    // Flash all the LEDs a few times
    for (int k = 0; k < 4; k += 1) {
        flashLEDs(DELAY_200ms);
    }
}

void loop() {
    buttonFanUp.update();
    buttonFanDown.update();
}
