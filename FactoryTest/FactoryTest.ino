/*
*   FactoryTest.ino
*
* This file is the main program for factory testing of the v2 PAPR board.
*
*/

#include "PAPRHwDefs.h"
#include "ButtonDebounce.h"

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

int currentLED = -1;

#define DELAY_10ms 10
#define DELAY_100ms 100
#define DELAY_500ms 500
#define DELAY_1SEC 1000
#define DELAY_5SEC 5000

bool Fan_Up_Value;
bool Fan_Down_Value;
bool Monitor_PIN_Value;

uint8_t s_fan_pwm_off = 0;
uint8_t s_fan_pwm_low = 51;
uint8_t s_fan_pwm_mid_low = 102;
uint8_t s_fan_pwm_mid = 153;
uint8_t s_fan_pwm_mid_hi = 204;
uint8_t s_fan_pwm_hi = 255;

int iSpeed = 0;

uint8_t speeds[] = {
    s_fan_pwm_off,
    s_fan_pwm_low,
    s_fan_pwm_mid_low,
    s_fan_pwm_mid,
    s_fan_pwm_mid_hi,
    s_fan_pwm_hi,
};
const int cSpeeds = sizeof(speeds) / sizeof(uint8_t);

ButtonDebounce buttonFanUp(FAN_UP_PIN, 100);
ButtonDebounce buttonFanDown(FAN_DOWN_PIN, 100);

//================================================================

void flashLED(byte pin, unsigned long duration) {
    digitalWrite(pin, LOW);
    delay(duration);
    digitalWrite(pin, HIGH);
}

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

void onButtonUpChange(const int state) {
    if (state == HIGH) {
        currentLED += 1;
        if (currentLED >= numLEDs) currentLED = 0;
        flashLED(LEDpins[currentLED], 1000);
    }
}

void onButtonDownChange(const int state) {
    if (state == HIGH) {
        currentLED -= 1;
        if (currentLED < 0) currentLED = numLEDs - 1;
        flashLED(LEDpins[currentLED], 1000);
    }
}

void flashUsartState() {
    int receiveEnabled = bitRead(UCSR0B, RXEN0);
    int transmitEnabled = bitRead(UCSR0B, TXEN0);
    delay(2000);
    if (receiveEnabled) {
        flashLEDs(100);
    }
    else {
        flashLEDs(100);
        flashLEDs(100);
    }
    delay(2000);
    if (transmitEnabled) {
        flashLEDs(100);
    }
    else {
        flashLEDs(100);
        flashLEDs(100);
    }
}
void setup() {
    configurePins();
    initializeDevices();

    // Initialize the button scanners
    buttonFanUp.setCallback(onButtonUpChange);
    buttonFanDown.setCallback(onButtonDownChange);

    // Flash all LEDs a few times
    for (int k = 0; k < 5; k += 1) {
        flashLEDs(200);
    }
}

void loop() {
    buttonFanUp.update();
    buttonFanDown.update();
}
