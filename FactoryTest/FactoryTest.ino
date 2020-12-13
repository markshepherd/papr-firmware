/*
*   FactoryTest.ino
*
* This file is the main program for factory testing of the v2 PAPR board.
*
*/

#include "PAPRHwDefs.h"
#include "ButtonDebounce.h"
#include "FanController.h"

const int DELAY_100ms = 100;
const int DELAY_200ms = 200;
const int DELAY_500ms = 500;
const int DELAY_1Sec = 1000;

enum Exercise {
    begin,
    buzzer,
    vibrator,
    fanMaximum,
    fanMedium,
    fanMinimum,
    batteryVoltage,
    fanRPM,
    end
};
// Note - this list doesn't have exercises for the fan buttons, because the fan up button 
// is used to cycle through the sequence of exercises so it gets plenty of exercise, and the
// fan down button has a separate exercise that is not part of the sequence.

/********************************************************************
 * LEDs
 ********************************************************************/

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
const int numLEDs = sizeof(LEDpins) / sizeof(byte);

// Turn a single LED on, then off again.
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
        digitalWrite(LEDpins[i], LOW);
        delay(duration);
        digitalWrite(LEDpins[i], HIGH);
        if (i == lastIndex) break;
        i += increment;
    } while (1);
}

void displayStartTest() {
    exerciseEachLED(0, numLEDs - 1, 50);
}

void displayEndTest() {
    exerciseEachLED(numLEDs - 1, 0, 50);
}


/********************************************************************
 * Temporary debug code, because we don't yet have a usable serial port or debugger.
 ********************************************************************/

void allLEDsOff() {
    for (int i = 0; i < numLEDs; i += 1) {
        digitalWrite(LEDpins[i], HIGH);
    }
}

void writeHexDigitToLights(int hexDigit) {
    allLEDsOff();

    // Turn on the leftmost light, to show that we're displaying a number
    digitalWrite(LEDpins[0], LOW);

    // Turn on the lights corresponding to the bits of hexDigit
    digitalWrite(LEDpins[3], (hexDigit & 8) ? LOW : HIGH);
    digitalWrite(LEDpins[4], (hexDigit & 4) ? LOW : HIGH);
    digitalWrite(LEDpins[5], (hexDigit & 2) ? LOW : HIGH);
    digitalWrite(LEDpins[6], (hexDigit & 1) ? LOW : HIGH);

    delay(DELAY_1Sec);
    allLEDsOff();
}

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
 * Buttons
 ********************************************************************/

ButtonDebounce buttonFanUp(FAN_UP_PIN, DELAY_100ms);
ButtonDebounce buttonFanDown(FAN_DOWN_PIN, DELAY_100ms);

/********************************************************************
 * Fan
 ********************************************************************/

// How many milliseconds between readings of the fan speed. A smaller value will update
// more often, while a higher value will give more accurate and smooth readings.
const int FAN_SPEED_READING_INTERVAL = 1000;

FanController fanController(FAN_RPM_PIN, FAN_SPEED_READING_INTERVAL, FAN_PWM_PIN);

const int FAN_DUTYCYCLE_MINIMUM = 0;
const int FAN_DUTYCYCLE_MEDIUM = 50;
const int FAN_DUTYCYCLE_MAXIMUM = 100;

void exerciseFan(int dutyCycle) {
    displayStartTest();
    fanController.setDutyCycle(dutyCycle);
    displayEndTest();
}

void exerciseFanSpeed() {
    displayStartTest();
    displayEndTest();
}

/********************************************************************
 * Other Devices
 ********************************************************************/

void exerciseBuzzer() {
    displayStartTest();
    for (int i = 0; i < 3; i += 1) {
        analogWrite(BUZZER_PIN, 255);
        delay(DELAY_500ms);
        analogWrite(BUZZER_PIN, 0);
        delay(DELAY_500ms);
    }
    displayEndTest();
}

void exerciseVibrator() {
    displayStartTest();
    for (int i = 0; i < 3; i += 1) {
        digitalWrite(VIBRATOR_PIN, HIGH);
        delay(DELAY_500ms);
        digitalWrite(VIBRATOR_PIN, LOW);
        delay(DELAY_500ms);
    }
    displayEndTest();
}

void exerciseBatteryVoltage() {
    displayStartTest();

    // The battery readings we expect for minimum and maximum battery voltages.
    const int readingAt12Volts = 386;
    const int readingAt24Volts = 784;

    // For the next 10 seconds we will display the battery voltage on the LEDs.
    // Empty battery = 1 LEDs. Full battery = 7 LEDs.
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
        // Read the current battery voltage and limit the value to the expected range.
        uint16_t reading = analogRead(BATTERY_VOLTAGE_PIN);
        if (reading < readingAt12Volts) reading = readingAt12Volts;
        if (reading > readingAt24Volts) reading = readingAt24Volts;

        // Calculate how full the battery is. This is a number between 0 and 1.
        float fullness = float(reading - readingAt12Volts) / float(readingAt24Volts - readingAt12Volts);

        // Calculate how many of the 7 LEDs we should show. 
        uint16_t howManyLEDs = int(fullness * 6.0) + 1;

        // Display the calculated number of LEDs.
        allLEDsOff();
        for (int i = 0; i < howManyLEDs; i += 1) {
            digitalWrite(LEDpins[i], LOW);
        }
    }

    displayEndTest();
}

/********************************************************************
 * Main program that drives the sequence of tests.
 ********************************************************************/

Exercise currentExercise;

// Find the next exercise value. if e is the last exercise, then we wrap around to the first exercise.
Exercise nextExercise(Exercise e) {
    e = static_cast<Exercise>(static_cast<int>(e) + 1);
    if (e == Exercise::end) {
        e = begin;
    }
    return e;
}

// Do the specified exercise.
void doExercise(Exercise exercise) {
    currentExercise = exercise;

    switch (exercise) {
    case begin:
        exerciseAllLEDs();
        break;
    case buzzer:
        exerciseBuzzer();
        break;
    case vibrator:
        exerciseVibrator();
        break;
    case fanMaximum:
        exerciseFan(FAN_DUTYCYCLE_MAXIMUM);
        break;
    case fanMedium:
        exerciseFan(FAN_DUTYCYCLE_MEDIUM);
        break;
    case fanMinimum:
        exerciseFan(FAN_DUTYCYCLE_MINIMUM);
        break;
    case batteryVoltage:
        exerciseBatteryVoltage();
        break;
    case fanRPM:
        exerciseFanSpeed();
        break;
    }
}

// Handler for Fan Button Down
void onButtonDownChange(const int state) {
    if (state == HIGH) {
        displayStartTest();
        exerciseEachLED(0, numLEDs - 1, DELAY_500ms);
        displayEndTest();
    }
}

// Handler for Fan Button Down
void onButtonUpChange(const int state) {
    if (state == HIGH) {
        // advance to the next exercise
        doExercise(nextExercise(currentExercise));
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
    doExercise(begin);
}

void loop() {
    // Run various polling functions and dispatch any events that have occured.
    // All the functionality of the app takes place in event handlers.
    buttonFanUp.update();
    buttonFanDown.update();
}
