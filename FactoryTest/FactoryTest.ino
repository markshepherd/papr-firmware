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

// These are all the exercises that this app performs, in the order that they are performed.
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
// Note - this list doesn't include exercises for the fan buttons, because the fan up button 
// is used to cycle through the sequence of exercises so it gets plenty of exercise, and the
// fan down button has a separate exercise that is not part of the sequence.

/********************************************************************
 * LEDs
 ********************************************************************/

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

// Turn off all LEDs
void allLEDsOff() {
    for (int i = 0; i < numLEDs; i += 1) {
        digitalWrite(LEDpins[i], HIGH);
    }
}

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

// Give a sign to the user that an exercise has started 
void startExercise() {
    exerciseEachLED(0, numLEDs - 1, 50);
}

// Give a sign to the user that an exercise has ended 
void endExercise() {
    exerciseEachLED(numLEDs - 1, 0, 50);
}


/********************************************************************
 * Temporary debug code, because we don't yet have a usable serial port or debugger.
 ********************************************************************/

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
 * Buttons
 ********************************************************************/

// The ButtonDebounce object polls the pin, and calls a callback when the pin value changes. There is one ButtonDebounce object per button.
ButtonDebounce buttonFanUp(FAN_UP_PIN, DELAY_100ms);
ButtonDebounce buttonFanDown(FAN_DOWN_PIN, DELAY_100ms);

/********************************************************************
 * Fan
 ********************************************************************/

// How many milliseconds should there be between readings of the fan speed. A smaller value will update
// more often, while a higher value will give more accurate and smooth readings.
const int FAN_SPEED_READING_INTERVAL = 1000;

FanController fanController(FAN_RPM_PIN, FAN_SPEED_READING_INTERVAL, FAN_PWM_PIN);

const int FAN_DUTYCYCLE_MINIMUM = 0;
const int FAN_DUTYCYCLE_MEDIUM = 50;
const int FAN_DUTYCYCLE_MAXIMUM = 100;

void exerciseFan(int dutyCycle) {
    fanController.setDutyCycle(dutyCycle);
}

void exerciseFanSpeed() {
    fanController.setDutyCycle(FAN_DUTYCYCLE_MEDIUM);

    // wait for the fan to get up to speed
    delay(DELAY_1Sec);

    // read the speed
    unsigned int speed = fanController.getSpeed();

    if (speed > 65535) {
        // speed is too big for writeNumberToLights
        exerciseAllLEDs();
    } else {
        writeNumberToLights(speed);
    }

    fanController.setDutyCycle(FAN_DUTYCYCLE_MINIMUM);
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

void exerciseVibrator() {
    for (int i = 0; i < 3; i += 1) {
        digitalWrite(VIBRATOR_PIN, HIGH);
        delay(DELAY_500ms);
        digitalWrite(VIBRATOR_PIN, LOW);
        delay(DELAY_500ms);
    }
}

void exerciseBatteryVoltage() {
    // The battery readings we expect for minimum and maximum battery voltages.
    // TODO: we probably need to get rid of these hard-coded constants, and use a better way to derive these numbers.
    const int readingAt12Volts = 386;
    const int readingAt24Volts = 784;

    // For the next 10 seconds we will display the battery voltage on the LEDs.
    // Empty battery = 1 LEDs. Full battery = 7 LEDs.
    // As you change the input voltage, the LEDs will update accordingly.
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
        // Read the current battery voltage and limit the value to the expected range.
        uint16_t reading = analogRead(BATTERY_VOLTAGE_PIN);
        if (reading < readingAt12Volts) reading = readingAt12Volts;
        if (reading > readingAt24Volts) reading = readingAt24Volts;

        // Calculate how full the battery is. This will be a number between 0 and 1.
        // (Note: we use floating point because it's easier than trying to do this in fixed point. The program memory impact appears to be negligible.)
        float fullness = float(reading - readingAt12Volts) / float(readingAt24Volts - readingAt12Volts);

        // Calculate how many of the 7 LEDs we should show. 
        uint16_t howManyLEDs = int(fullness * 6.0) + 1;

        // Display the calculated number of LEDs.
        allLEDsOff();
        for (int i = 0; i < howManyLEDs; i += 1) {
            digitalWrite(LEDpins[i], LOW);
        }
    }
}

/********************************************************************
 * Main program that drives the sequence of exercises.
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

    startExercise();
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
    endExercise();
}

// Handler for Fan Button Down
// This button runs the fan-button-down exercise.
void onButtonDownChange(const int state) {
    if (state == HIGH) {
        startExercise();
        exerciseEachLED(0, numLEDs - 1, DELAY_500ms);
        endExercise();
    }
}

// Handler for Fan Button Up
// This button advances to the next exercise.
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
    fanController.getSpeed(); // The fan controller speed function works better if we call it often.
}
