/*
 * Main.cpp
 *
 * The main program of the PAPR product firmware.
 * 
 */
#include "Main.h"
#include "PAPRHwDefs.h"
#include "Hardware.h"
#include <ButtonDebounce.h>
#include <FanController.h>

// Use these when you call delay()
const int DELAY_100ms = 100;
const int DELAY_200ms = 200;
const int DELAY_500ms = 500;
const int DELAY_1sec = 1000;
const int DELAY_3sec = 3000;

// The Hardware object gives access to all the microcontroller hardware such as pins and timers. Please always use this object,
// and never access any hardware or Arduino APIs directly. This gives us the abiity to use a fake hardware object for unit testing.
extern Hardware& hw = Hardware::instance();

/********************************************************************
 * Button data
 ********************************************************************/

// The ButtonDebounce object polls a pin, and calls a callback when the pin value changes. There is one ButtonDebounce object per button.
ButtonDebounce buttonFanUp(FAN_UP_PIN, DELAY_100ms);
ButtonDebounce buttonFanDown(FAN_DOWN_PIN, DELAY_100ms);
ButtonDebounce buttonPowerOff(MONITOR_PIN, DELAY_100ms);

/********************************************************************
 * Fan data
 ********************************************************************/

// How many milliseconds should there be between readings of the fan speed. A smaller value will update
// more often, while a higher value will give more accurate and smooth readings.
const int FAN_SPEED_READING_INTERVAL = 1000;

// The user can choose any of these speeds
enum FanSpeed { fanLow, fanMedium, fanHigh };

// The duty cycle for each fan speed.
const byte fanDutyCycles[] = { 0, 50, 100 };

// The expected RPM for each fan speed.
const unsigned int expectedFanRPM[] = { 3123, 12033, 15994 };

// How much tolerance do we give when checking for correct fan RPM
const float lowestOkFanRPM = 0.5;
const float highestOkFanRPM = 2.0;

// The fan speed when we startup.
const FanSpeed defaultFanSpeed = fanLow;

// The object that controls and monitors the fan.
FanController fanController(FAN_RPM_PIN, FAN_SPEED_READING_INTERVAL, FAN_PWM_PIN);

// The current fan speed selected by the user.
FanSpeed currentFanSpeed;

// After we change the fan speed, we stop checking the RPMs for a few seconds, to let the speed stabilize.
unsigned long dontCheckFanSpeedUntil = 0;

/********************************************************************
 * LED data
 ********************************************************************/

// A list of all the LEDs, from left to right.
extern const byte LEDpins[] = {
    BATTERY_LED_LOW_PIN,
    BATTERY_LED_MED_PIN,
    BATTERY_LED_HIGH_PIN,
    ERROR_LED_PIN,
    FAN_LOW_LED_PIN,
    FAN_MED_LED_PIN,
    FAN_HIGH_LED_PIN
};
extern const int numLEDs = sizeof(LEDpins) / sizeof(byte);

/********************************************************************
 * Battery data
 ********************************************************************/

// Here are the raw battery readings we expect for minimum and maximum battery voltages. These numbers were determined empirically.
const int readingAt12Volts = 384;
const int readingAt24Volts = 774;

// Low/medium/high battery LEDs.
const int batteryFullPercent = 90; // (22.7V) If the battery is above this value, we light the "full" LED
const int batteryHalfPercent = 45; // (17.4V) If the battery is above this value but less than full, we light the "half" LED
const int batteryLowPerent = 3;    // (12.3V) If the battery is above this value but less than half, we light the "low" LED
// If the battery is below low, we raise a low battery alert

// We don't check the battery level on every loop(). Rather, we average battery levels over
// a second or so, to smooth out the minor variations.
unsigned long nextBatteryCheckMillis = 0;
unsigned long batteryFullnessAccumulator = 0;
unsigned long numBatteryFullnessSamples = 0;

/********************************************************************
 * Alert data
 ********************************************************************/

// The different kinds of alert we can present to the user.
enum Alert {alertNone, alertBatteryLow, alertFanRPM };

// LEDs to flash when there is a battery low alert
const int batteryLowLEDs[] = { BATTERY_LED_LOW_PIN, BATTERY_LED_MED_PIN, BATTERY_LED_HIGH_PIN, -1 };

// LEDs to flash when there is a fan RPM alert
const int fanRPMLEDs[] = { FAN_LOW_LED_PIN, FAN_MED_LED_PIN, FAN_HIGH_LED_PIN, -1 };

// Which LEDs to flash for each type of alert.
const int* alertLEDs[] = { 0, batteryLowLEDs, fanRPMLEDs };

/********************************************************************
 * LEDs
 ********************************************************************/

// Turn off all LEDs
void allLEDsOff()
{
    for (int i = 0; i < numLEDs; i += 1) {
        hw.digitalWrite(LEDpins[i], LED_OFF);
    }
}

/********************************************************************
 * Alerts
 ********************************************************************/

// Enter the "alert" state. In this state we pulse the lights, buzzer, and/or fan to 
// alert the user to an alert condition. Once we are in this state, the only
// way out is for the user to turn the power off.
void enterAlertState(Alert alert)
{
    // Which LEDs should we flash?
    const int* leds = alertLEDs[alert];

    // Turn off all LEDs
    for (int i = 0; i < numLEDs; i += 1) {
        hw.digitalWrite(LEDpins[i], LED_OFF);
    }

    // Should we pulse the fan?
    bool pulseFan = alert == alertFanRPM;

    // Flash the alert LEDs, sound the buzzer, and pulse the fan, until the user powers off.
    while (1) {
        hw.analogWrite(BUZZER_PIN, BUZZER_ON);
        hw.digitalWrite(ERROR_LED_PIN, LED_ON);
        for (int i = 0; leds[i] != -1; i += 1) {
            hw.digitalWrite(leds[i], LED_ON);
        }
        if (pulseFan) fanController.setDutyCycle(fanDutyCycles[fanMedium]);
        hw.delay(DELAY_500ms);

        hw.analogWrite(BUZZER_PIN, BUZZER_OFF);
        hw.digitalWrite(ERROR_LED_PIN, LED_OFF);
        for (int i = 0; leds[i] != -1; i += 1) {
            hw.digitalWrite(leds[i], LED_OFF);
        }
        if (pulseFan) fanController.setDutyCycle(fanDutyCycles[fanLow]);
        hw.delay(DELAY_200ms);
    }
}

/********************************************************************
 * Fan
 ********************************************************************/

// Set the fan to the indicated speed, and update the fan indicator LEDs.
void setFanSpeed(FanSpeed speed)
{
    fanController.setDutyCycle(fanDutyCycles[speed]);
    
    hw.digitalWrite(FAN_LOW_LED_PIN,  LED_ON);
    hw.digitalWrite(FAN_MED_LED_PIN,  speed >  fanLow  ? LED_ON : LED_OFF);
    hw.digitalWrite(FAN_HIGH_LED_PIN, speed == fanHigh ? LED_ON : LED_OFF);

    currentFanSpeed = speed;

    // disable fan RPM monitor for a few seconds, until the new fan speed stabilizes
    dontCheckFanSpeedUntil = hw.millis() + DELAY_3sec;
}

// Call this periodically to check that the fan RPM is within the expected range for the current FanSpeed.
void updateFan() {
    const unsigned int fanRPM = fanController.getSpeed(); 
    // Note: we call getSpeed() even if we're not going to use the result, because getSpeed() works better if you call it often.

    // If fan RPM checking is temporarily disabled, then do nothing.
    if (dontCheckFanSpeedUntil) {
        if (dontCheckFanSpeedUntil > hw.millis()) return;
        dontCheckFanSpeedUntil = 0;
    }

    // If the RPM is too low or too high compared to the expected value, raise an alert.
    const unsigned int expectedRPM = expectedFanRPM[currentFanSpeed];
    if ((fanRPM < (lowestOkFanRPM * expectedRPM)) || (fanRPM > (highestOkFanRPM * expectedRPM))) {
        enterAlertState(alertFanRPM);
    }
}

/********************************************************************
 * Battery
 ********************************************************************/

 // Return battery fullness as a number between 0 (empty = 12 volts) and 100 (full = 24 volts).
unsigned int readBatteryFullness()
{
    // Read the current battery voltage. To smoothen the random variations in the
    // reading, we take several readings and calculate the average.
    const int numReadings = 10;
    uint16_t reading = 0;
    for (int i = 0; i < numReadings; i += 1) {
        reading += hw.analogRead(BATTERY_VOLTAGE_PIN);
        hw.delayMicroseconds(5);
    }
    reading = reading / numReadings;

    // Limit the value to the allowed range.
    if (reading < readingAt12Volts) reading = readingAt12Volts;
    if (reading > readingAt24Volts) reading = readingAt24Volts;

    // Calculate how full the battery is. This will be a number between 0 and 100, inclusive.
    const float fullness = float(reading - readingAt12Volts) / float(readingAt24Volts - readingAt12Volts);
    return (unsigned int)(fullness * 100);
}

// Call this periodically to update the battery level LEDs, and raise an alert if the level gets too low.
void updateBattery() {
    // Don't update the LEDs too often. This smooths out any small variations.
    const unsigned long now = millis();
    if (now < nextBatteryCheckMillis || numBatteryFullnessSamples == 0) {
        // we have not reached the end of the averaging period. Gather a measurement.
        batteryFullnessAccumulator += readBatteryFullness();
        numBatteryFullnessSamples += 1;
        return;
    }

    // The averaging period has ended...

    // ...Calculate the battery fullness (0-100%) by taking the average of all the readings we made during the period.
    const unsigned int fullness = batteryFullnessAccumulator / numBatteryFullnessSamples;

    // ...Start a new averaging period
    nextBatteryCheckMillis = now + DELAY_1sec;
    batteryFullnessAccumulator = 0;
    numBatteryFullnessSamples = 0;

    // Turn off all the battery LEDs
    hw.digitalWrite(BATTERY_LED_LOW_PIN, LED_OFF);
    hw.digitalWrite(BATTERY_LED_MED_PIN, LED_OFF);
    hw.digitalWrite(BATTERY_LED_HIGH_PIN, LED_OFF);

    // Turn on the LED corresponding to the current fullness
    if (fullness >= batteryFullPercent) {
        hw.digitalWrite(BATTERY_LED_HIGH_PIN, LED_ON);
    } else if (fullness >= batteryHalfPercent) {
        hw.digitalWrite(BATTERY_LED_MED_PIN, LED_ON);
    } else if (fullness >= batteryLowPerent) {
        hw.digitalWrite(BATTERY_LED_LOW_PIN, LED_ON);
    } else {
        enterAlertState(alertBatteryLow);
    }
}

// Handler for Fan Down button
void onFanDownButtonChange(const int state)
{
    if (state == BUTTON_RELEASED) {
        setFanSpeed((currentFanSpeed == fanHigh) ? fanMedium : fanLow);
    }
}

// Handler for Fan Up button
void onFanUpButtonChange(const int state)
{
    if (state == BUTTON_RELEASED) {
        setFanSpeed((currentFanSpeed == fanLow) ? fanMedium : fanHigh);
    }
}

// Handler for the Power Off button
void onMonitorChange(const int state)
{
    if (state == BUTTON_PUSHED) {
        // Set the UI to "going away soon"

        // Turn on all LEDs, and the buzzer
        for (int i = 0; i < numLEDs; i += 1) {
            hw.digitalWrite(LEDpins[i], LED_ON);
        }
        hw.analogWrite(BUZZER_PIN, BUZZER_ON);

        // Leave the lights and buzzer on. We expect to lose power in just a moment.
    } else {
        // The user must have pushed and released the button very quickly, not long enough
        // to actually shut off the power. Go back to normal.
        allLEDsOff();
        hw.analogWrite(BUZZER_PIN, BUZZER_OFF);
        setFanSpeed(currentFanSpeed); // to update the fan speed LEDs
        // The battery level indicator will be updated by updateBattery() on the next call to loop().
    }
}

void Main::setup()
{
    // Initialize the hardware
    hw.configurePins();
    hw.initializeDevices();

    // Initialize the buttons
    buttonFanUp.setCallback(onFanUpButtonChange);
    buttonFanDown.setCallback(onFanDownButtonChange);
    buttonPowerOff.setCallback(onMonitorChange);

    // Initialize the fan
    fanController.begin();
    setFanSpeed(defaultFanSpeed);
}

void Main::loop()
{
    // Run various polling functions
    buttonFanUp.update();
    buttonFanDown.update();
    buttonPowerOff.update();
    updateFan();
    updateBattery();
}
