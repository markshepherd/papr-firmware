/*
*   FactoryTest.ino
*
* This is the main program for the v2 PAPR board Factory Test app.
*
*/

#include "PAPRHwDefs.h"
#include "ButtonDebounce.h"
#include "FanController.h"

const int DELAY_100ms = 100;
const int DELAY_200ms = 200;
const int DELAY_500ms = 500;
const int DELAY_1Sec = 1000;

// ----------------------- Button data -----------------------

// The ButtonDebounce object polls the pin, and calls a callback when the pin value changes. There is one ButtonDebounce object per button.
ButtonDebounce buttonFanUp(FAN_UP_PIN, DELAY_100ms);
ButtonDebounce buttonFanDown(FAN_DOWN_PIN, DELAY_100ms);

// ----------------------- Fan data -----------------------

// How many milliseconds should there be between readings of the fan speed. A smaller value will update
// more often, while a higher value will give more accurate and smooth readings.
const int FAN_SPEED_READING_INTERVAL = 1000;

FanController fanController(FAN_RPM_PIN, FAN_SPEED_READING_INTERVAL, FAN_PWM_PIN);

const int FAN_DUTYCYCLE_MINIMUM = 0;
const int FAN_DUTYCYCLE_MEDIUM = 50;
const int FAN_DUTYCYCLE_MAXIMUM = 100;

// ----------------------- LED data -----------------------

// A list of all the LEDs, from left to right as they appear on the board.
byte LEDpins[] = {
    BATTERY_LED_1_PIN,
    BATTERY_LED_2_PIN,
    BATTERY_LED_3_PIN,
    Error_LED_PIN,
    MODE_LED_1_PIN,
    MODE_LED_2_PIN,
    MODE_LED_3_PIN
};
const int numLEDs = sizeof(LEDpins) / sizeof(byte);


/********************************************************************
 * LEDs
 ********************************************************************/

// Turn off all LEDs
void allLEDsOff() {
    for (int i = 0; i < numLEDs; i += 1) {
        digitalWrite(LEDpins[i], LED_OFF);
    }
}

// Turn a single LED on, then off again.
void flashLED(byte pin, unsigned long duration) {
    digitalWrite(pin, LED_ON);
    delay(duration);
    digitalWrite(pin, LED_OFF);
}

// Turn all the LEDs on, then off again.
void flashLEDs(unsigned long duration) {
    // Turn on all LEDs
    for (int i = 0; i < numLEDs; i += 1) {
        digitalWrite(LEDpins[i], LED_ON);
    }

    delay(duration);

    // Turn off all LEDs
    for (int i = 0; i < numLEDs; i += 1) {
        digitalWrite(LEDpins[i], LED_OFF);
    }

    delay(duration);
}

// Flash all the LEDs a few times
void exerciseAllLEDs() {
    for (int k = 0; k < 3; k += 1) {
        flashLEDs(DELAY_200ms);
    }
}

// Flash all the LEDs, one at a time, starting with firstIndex, ending at lastIndex.
void exerciseEachLED(int firstIndex, int lastIndex, int duration) {
    int increment = firstIndex < lastIndex ? 1 : -1;
    int i = firstIndex;
    do {
        digitalWrite(LEDpins[i], LED_ON);
        delay(duration);
        digitalWrite(LEDpins[i], LED_OFF);
        if (i == lastIndex) break;
        i += increment;
    } while (1);
}

// Give a sign to the user that an exercise has started 
void startExercise() {
    for (int i = 3; i >= 0; i -= 1) {
        digitalWrite(LEDpins[i], LED_ON);
        digitalWrite(LEDpins[6 - i], LED_ON);
        delay(DELAY_100ms);
        digitalWrite(LEDpins[i], LED_OFF);
        digitalWrite(LEDpins[6 - i], LED_OFF);
    }
}

// Give a sign to the user that an exercise has ended 
void endExercise() {
    for (int i = 0; i <= 3; i += 1) {
        digitalWrite(LEDpins[i], LED_ON);
        digitalWrite(LEDpins[6 - i], LED_ON);
        delay(DELAY_100ms);
        digitalWrite(LEDpins[i], LED_OFF);
        digitalWrite(LEDpins[6 - i], LED_OFF);
    }
}

/********************************************************************
 * Temporary debug code, because we don't yet have a usable serial port or debugger.
 ********************************************************************/

void writeHexDigitToLights(int hexDigit) {
    allLEDsOff();

    // Turn on the leftmost light, to show that we're displaying a number
    digitalWrite(LEDpins[0], LED_ON);

    // Turn on the lights corresponding to the bits of hexDigit
    digitalWrite(LEDpins[3], (hexDigit & 8) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[4], (hexDigit & 4) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[5], (hexDigit & 2) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[6], (hexDigit & 1) ? LED_ON : LED_OFF);

    delay(DELAY_1Sec);
    allLEDsOff();
}

// Write a 16-bit number to the LEDs, in hex. It looks like this ...
//    all LEDs quickly flash, left to right
//    high-order hex digit, in binary
//    next hex digit, in binary
//    next hex digit, in binary
//    low-order hex digit, in binary
//    all LEDs quickly flash, right to left
void writeNumberToLights(uint16_t number) {
    exerciseEachLED(0, numLEDs - 1, 50);

    writeHexDigitToLights((number >> 12) & 0xf);
    delay(DELAY_500ms);

    writeHexDigitToLights((number >> 8) & 0xf);
    delay(DELAY_500ms);

    writeHexDigitToLights((number >> 4) & 0xf);
    delay(DELAY_500ms);

    writeHexDigitToLights((number) & 0xf);
    delay(DELAY_500ms);

    exerciseEachLED(numLEDs - 1, 0, 50);
}

/********************************************************************
 * Fan
 ********************************************************************/

void exerciseFan(int dutyCycle) {
    fanController.setDutyCycle(dutyCycle);
}

void exerciseFanMaximum() {
    exerciseFan(FAN_DUTYCYCLE_MAXIMUM);
}

void exerciseFanMedium() {
    exerciseFan(FAN_DUTYCYCLE_MEDIUM);
}

void exerciseFanMinimum() {
    exerciseFan(FAN_DUTYCYCLE_MINIMUM);
}

/********************************************************************
 * Other Devices
 ********************************************************************/

void exerciseBuzzer() {
    for (int i = 0; i < 3; i += 1) {
        analogWrite(BUZZER_PIN, 255);
        delay(DELAY_500ms);
        analogWrite(BUZZER_PIN, 0);
        delay(DELAY_500ms);
    }
}

void exerciseBatteryVoltage() {

    // For the next 10 seconds we will display the battery voltage on the LEDs.
    // Empty battery = 1 LEDs. Full battery = 7 LEDs.
    // As you change the input voltage, the LEDs will update accordingly.
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
        unsigned int fullness = readBatteryFullness();

        // Calculate how many of the 7 LEDs we should show. 
        uint16_t howManyLEDs = ((fullness * 6) / 100) + 1;

        // Display the calculated number of LEDs.
        allLEDsOff();
        for (int i = 0; i < howManyLEDs; i += 1) {
            digitalWrite(LEDpins[i], LED_ON);
        }
    }
}

/********************************************************************
 * Main program that drives the sequence of exercises.
 ********************************************************************/

void (*exercises[])() = {
        exerciseAllLEDs,
        exerciseBuzzer,
        exerciseFanMaximum,
        exerciseFanMedium,
        exerciseFanMinimum,
        exerciseBatteryVoltage
};
const int numberOfExercises = sizeof(exercises) / sizeof(void (*)());

int currentExercise;

// Do the next exercise. If we've done them all, then start over at the first.
void doNextExercise() {
    currentExercise = (currentExercise + 1) % numberOfExercises;

    startExercise();
    (*exercises[currentExercise])();
    endExercise();
}

// Handler for Fan Button Down
// This button runs the fan-button-down exercise.
void onButtonDownChange(const int state) {
    if (state == BUTTON_RELEASED) {
        startExercise();
        exerciseEachLED(0, numLEDs - 1, DELAY_500ms);
        endExercise();
    }
}

// Handler for Fan Button Up
// This button advances to the next exercise.
void onButtonUpChange(const int state) {
    if (state == BUTTON_RELEASED) {
        // advance to the next exercise
        doNextExercise();
    }
}

void setup() {
    // Initialize the hardware
    configurePins();
    initializeDevices();

    // Initialize the software
    buttonFanUp.setCallback(onButtonUpChange);
    buttonFanDown.setCallback(onButtonDownChange);
    fanController.begin();

    // Do our startup exercise
    currentExercise = -1;
    doNextExercise();
}

void loop() {
    // Run various polling functions and dispatch any events that have occured.
    // All the functionality of the app takes place in event handlers.
    buttonFanUp.update();
    buttonFanDown.update();
    fanController.getSpeed(); // The fan controller speed function works better if we call it often.
}
