/*
 * Main.cpp
 *
 * The main program of the PAPR product firmware.
 * 
 */
#include "Main.h"
#include "PAPRHwDefs.h"
#include "Hardware.h"
#include "Timer.h"
#include <ButtonDebounce.h>
#include <FanController.h>

#undef MYSERIAL

#ifdef MYSERIAL
#include "MySerial.h"
#endif

// Use these when you call delay()
const int DELAY_100ms = 100;
const int DELAY_500ms = 500;
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

// Battery levels of interest to the UI. These are determined empirically.
/*
measured:
    full 21.6 volts = 703
    cutoff 16.2 volts = 525
    95% full = [est 20.6] volts = 669 TODO MEASURE FOR REAL
    30 minutes left (before 16.2) = [est 17.0] volts = 552 TODO MEASURE FOR REAL
*/
const int batteryFullLevel = 669; // (xx.xV) If the battery is above this value, we light the green LED
const int batteryLowLevel = 552;  // (yy.yV) If the battery is above this value but less than full, we light the yellow LED
                                  //         If the battery is below this value, we flash the red LED and pulse the buzzer

// We don't check the battery level on every loop(). Rather, we average battery levels over
// a second or so, to smooth out the minor variations.
unsigned long nextBatteryCheckMillis = 0;
unsigned long batteryLevelAccumulator = 0;
unsigned long numBatteryLevelSamples = 0;

enum BatteryLevel { batteryLow, batteryNormal, batteryFull };
BatteryLevel currentBatteryLevel = batteryFull;

/********************************************************************
 * Alert data
 ********************************************************************/

// The different kinds of alert we can present to the user.
enum Alert { alertNone, alertBatteryLow, alertFanRPM };

// Which LEDs to flash for each type of alert.
const int batteryLowLEDs[] = { BATTERY_LED_LOW_PIN, ERROR_LED_PIN , -1 };
const int fanRPMLEDs[] = { FAN_LOW_LED_PIN, FAN_MED_LED_PIN, FAN_HIGH_LED_PIN, ERROR_LED_PIN, -1 };
const int* alertLEDs[] = { 0, batteryLowLEDs, fanRPMLEDs };

// What are the on & off durations for the pulsed lights and buzzer for each type of alert. 
const int batteryAlertMillis[] = { 1000, 1000 };
const int fanAlertMillis[] = { 200, 200 };
const int* alertMillis[] = { 0, batteryAlertMillis, fanAlertMillis };

// Data used when we are in the alert state.
Alert currentAlert = alertNone;
const int* currentAlertLEDs;
const int* currentAlertMillis;
bool alertToggle;

// The timer that pulses the lights and buzzer during an alert.
Timer alertTimer;

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

// Turn on all LEDs
void allLEDsOn()
{
    for (int i = 0; i < numLEDs; i += 1) {
        hw.digitalWrite(LEDpins[i], LED_ON);
    }
}

// Set a list of LEDs to a given state.
void setLEDs(const int* pinList, int state)
{
    for (int i = 0; pinList[i] != -1; i += 1) {
        hw.digitalWrite(pinList[i], state);
    }
}

/********************************************************************
 * Alerts
 ********************************************************************/

// This function pulses the lights and buzzer during an alert.
void toggleAlert()
{
    alertToggle = !alertToggle;
    setLEDs(currentAlertLEDs, alertToggle ? LED_ON : LED_OFF);
    hw.analogWrite(BUZZER_PIN, alertToggle ? BUZZER_ON : BUZZER_OFF);
    alertTimer.start(toggleAlert, currentAlertMillis[alertToggle ? 0 : 1]);
}

// Enter the "alert" state. In this state we pulse the lights and buzzer to 
// alert the user to a problem. Once we are in this state, the only
// way out is for the user to turn the power off.
void enterAlertState(Alert alert)
{
    currentAlert = alert;
    currentAlertLEDs = alertLEDs[alert];
    currentAlertMillis = alertMillis[alert];
    alertToggle = false;
    alertTimer.start(toggleAlert, 1);
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

// Call this periodically to update the battery level LEDs, and raise an alert if the level gets too low.
void updateBattery() {
    // Don't update the LEDs too often. This smooths out any small variations.
    const unsigned long now = hw.millis();
    if (now < nextBatteryCheckMillis || numBatteryLevelSamples == 0) {
        // we have not reached the end of the averaging period. Gather a measurement.
        batteryLevelAccumulator += hw.analogRead(BATTERY_VOLTAGE_PIN);
        numBatteryLevelSamples += 1;
        return;
    }

    // The averaging period has ended...

    // ...Calculate the battery level by taking the average of all the readings we made during the period.
    const unsigned int batteryLevel = batteryLevelAccumulator / numBatteryLevelSamples;
    // myPrintf("battery level %d\r\n", batteryLevel);

    // ...Start a new averaging period
    nextBatteryCheckMillis = now + DELAY_500ms;
    batteryLevelAccumulator = 0;
    numBatteryLevelSamples = 0;

    // Turn off all the battery LEDs
    hw.digitalWrite(BATTERY_LED_LOW_PIN, LED_OFF);
    hw.digitalWrite(BATTERY_LED_MED_PIN, LED_OFF);
    hw.digitalWrite(BATTERY_LED_HIGH_PIN, LED_OFF);

    // Turn on the LED corresponding to the current level. To keep the UI from jittering, don't allow the level
    // to go up, only down. 
    if (batteryLevel >= batteryFullLevel && currentBatteryLevel >= batteryFull) {
        currentBatteryLevel = batteryFull;
        hw.digitalWrite(BATTERY_LED_HIGH_PIN, LED_ON);
    } else if (batteryLevel >= batteryLowLevel && currentBatteryLevel >= batteryNormal) {
        currentBatteryLevel = batteryNormal;
        hw.digitalWrite(BATTERY_LED_MED_PIN, LED_ON);
    } else {
        currentBatteryLevel = batteryLow;
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
        allLEDsOn();
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

    #ifdef MYSERIAL
    initSerial();
    myPrintf("PAPR startup\r\n");
    #endif

    // Initialize the buttons
    buttonFanUp.setCallback(onFanUpButtonChange);
    buttonFanDown.setCallback(onFanDownButtonChange);
    buttonPowerOff.setCallback(onMonitorChange);

    // Initialize the fan
    fanController.begin();
    setFanSpeed(defaultFanSpeed);

    nextBatteryCheckMillis = hw.millis() + DELAY_500ms;
}

void Main::loop()
{
    // Run various polling functions
    buttonFanUp.update();
    buttonFanDown.update();
    buttonPowerOff.update();
    alertTimer.update();
    if (currentAlert != alertFanRPM) updateFan();
    if (currentAlert != alertBatteryLow) updateBattery();
}
