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
const int numLEDs = sizeof(LEDpins) / sizeof(byte);

const int DELAY_100ms = 100;
const int DELAY_200ms = 200;
const int DELAY_500ms = 500;

// These objects monitor the buttons and give a callback when the button state changes.
ButtonDebounce buttonFanUp(FAN_UP_PIN, DELAY_100ms);
ButtonDebounce buttonFanDown(FAN_DOWN_PIN, DELAY_100ms);

enum Exercise {
    begin,
    LEDsLeftToRight,
    LEDsRightToLeft,
    end,
    buzzer,
    vibrator,
    fanMaximum,
    fanMedium,
    fanMinimum,
    batteryVoltage,
    fanRPM
};

Exercise currentExercise;

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

void sequenceLEDs(int firstIndex, int lastIndex) {
    int increment = firstIndex < lastIndex ? 1 : -1;
    int i = firstIndex;
    do {
        digitalWrite(LEDpins[i], LOW);
        delay(DELAY_500ms);
        digitalWrite(LEDpins[i], HIGH);
        if (i == lastIndex) break;
        i += increment;
    } while (1);
}

void exerciseBuzzer() {
    for (int i = 0; i < 3; i += 1) {
        analogWrite(BUZZER_PIN, 255);
        delay(DELAY_500ms);
        analogWrite(BUZZER_PIN, 0);
        delay(DELAY_500ms);
    }
}

void exerciseVibrator() {
    for (int i = 0; i < 3; i += 1) {
        digitalWrite(VIBRATOR_PIN, HIGH);
        delay(DELAY_500ms);
        digitalWrite(VIBRATOR_PIN, LOW);
        delay(DELAY_500ms);
    }
}

// return the next exercise value. if e is the last exercise, then we return the first.
Exercise nextExercise(Exercise e) {
    e = static_cast<Exercise>(static_cast<int>(e) + 1);
    if (e == Exercise::end) {
        e = begin;
    }
    return e;
}

void doExercise(Exercise exercise) {
    currentExercise = exercise;

    switch (exercise) {
    case begin:
        // Flash all the LEDs a few times
        for (int k = 0; k < 4; k += 1) {
            flashLEDs(DELAY_200ms);
        }
        break;
    case LEDsLeftToRight:
        sequenceLEDs(0, numLEDs - 1);
        break;
    case LEDsRightToLeft:
        sequenceLEDs(numLEDs - 1, 0);
        break;
    case buzzer:
        exerciseBuzzer();
        break;
    case vibrator:
        exerciseVibrator();
        break;
    case fanMaximum:
        break;
    case fanMedium:
        break;
    case fanMinimum:
        break;
    case batteryVoltage:
        break;
    case fanRPM:
        break;
    default:
        break;
    }
}

void onButtonDownChange(const int state) {
    if (state == HIGH) {
        doExercise(begin);
    }
}

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

    // Initialize the button scanners
    buttonFanUp.setCallback(onButtonUpChange);
    buttonFanDown.setCallback(onButtonDownChange);

    // Do our startup exercise
    doExercise(begin);
}

void loop() {
    // Run various polling functions and dispatch any events that have occured.
    // All the functionality of the app takes place in event handlers.
    buttonFanUp.update();
    buttonFanDown.update();
}
