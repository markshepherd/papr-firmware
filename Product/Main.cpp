/*
 * Main.cpp
 *
 * The main program of the PAPR product firmware.
 * 
 */
#include "Main.h"

#undef MYSERIAL

#ifdef MYSERIAL
#include "MySerial.h"
#endif

/********************************************************************
 * Fan constants
 ********************************************************************/

// How many milliseconds should there be between readings of the fan speed. A smaller value will update
// more often, while a higher value will give more accurate and smooth readings.
const int FAN_SPEED_READING_INTERVAL = 1000;

// The duty cycle for each fan speed.
const byte fanDutyCycles[] = { 0, 50, 100 };

// The expected RPM for each fan speed.
const unsigned int expectedFanRPM[] = { 3123, 12033, 15994 };

// How much tolerance do we give when checking for correct fan RPM
const float lowestOkFanRPM = 0.5;
const float highestOkFanRPM = 2.0;

// The fan speed when we startup.
const FanSpeed defaultFanSpeed = fanLow;

/********************************************************************
 * LED constants
 ********************************************************************/

// A list of all the LEDs, from left to right.
const byte LEDpins[] = {
    BATTERY_LED_LOW_PIN,
    BATTERY_LED_MED_PIN,
    BATTERY_LED_HIGH_PIN,
    ERROR_LED_PIN,
    FAN_LOW_LED_PIN,
    FAN_MED_LED_PIN,
    FAN_HIGH_LED_PIN
};
const int numLEDs = sizeof(LEDpins) / sizeof(byte);

/********************************************************************
 * Battery constants
 ********************************************************************/

// Battery levels of interest to the UI. These are determined empirically.
/*
measured:
    25.2 volts = 819
    21.6 volts = 703
    cutoff 16.2 volts = 525
    95% full = [est 20.6] volts = 670 TODO MEASURE FOR REAL
    30 minutes left (before 16.2) = [est 17.0] volts = 552 TODO MEASURE FOR REAL
*/
const int batteryFullLevel = 670; // (xx.xV) If the battery is above this value, we light the green LED
const int batteryLowLevel = 552;  // (yy.yV) If the battery is above this value but less than full, we light the yellow LED
                                  //         If the battery is below this value, we flash the red LED and pulse the buzzer

/********************************************************************
 * Alert constants
 ********************************************************************/

// Which LEDs to flash for each type of alert.
const int batteryLowLEDs[] = { BATTERY_LED_LOW_PIN, ERROR_LED_PIN , -1 };
const int fanRPMLEDs[] = { FAN_LOW_LED_PIN, FAN_MED_LED_PIN, FAN_HIGH_LED_PIN, ERROR_LED_PIN, -1 };
const int* alertLEDs[] = { 0, batteryLowLEDs, fanRPMLEDs };

// What are the on & off durations for the pulsed lights and buzzer for each type of alert. 
const int batteryAlertMillis[] = { 1000, 1000 };
const int fanAlertMillis[] = { 200, 200 };
const int* alertMillis[] = { 0, batteryAlertMillis, fanAlertMillis };

// Use these to specify time intervals
const int INTERVAL_100ms = 100;
const int INTERVAL_500ms = 500;
const int INTERVAL_3sec = 3000;

/********************************************************************
 * LED
 ********************************************************************/

// Turn off all LEDs
void Main::allLEDsOff()
{
    for (int i = 0; i < numLEDs; i += 1) {
        hw.digitalWrite(LEDpins[i], LED_OFF);
    }
}

// Turn on all LEDs
void Main::allLEDsOn()
{
    for (int i = 0; i < numLEDs; i += 1) {
        hw.digitalWrite(LEDpins[i], LED_ON);
    }
}

// Set a list of LEDs to a given state.
void Main::setLEDs(const int* pinList, int state)
{
    for (int i = 0; pinList[i] != -1; i += 1) {
        hw.digitalWrite(pinList[i], state);
    }
}

/********************************************************************
 * Alert
 ********************************************************************/

void Main::onToggleAlert()
{
    instance->realOnToggleAlert();
}

// This function pulses the lights and buzzer during an alert.
void Main::realOnToggleAlert()
{
    alertToggle = !alertToggle;
    instance->setLEDs(currentAlertLEDs, alertToggle ? LED_ON : LED_OFF);
    instance->hw.analogWrite(BUZZER_PIN, alertToggle ? BUZZER_ON : BUZZER_OFF);
    alertTimer.start(onToggleAlert, currentAlertMillis[alertToggle ? 0 : 1]);
}

// Enter the "alert" state. In this state we pulse the lights and buzzer to 
// alert the user to a problem. Once we are in this state, the only
// way out is for the user to turn the power off.
void Main::enterAlertState(Alert alert)
{
    currentAlert = alert;
    currentAlertLEDs = alertLEDs[alert];
    currentAlertMillis = alertMillis[alert];
    alertToggle = false;
    realOnToggleAlert();
}

/********************************************************************
 * Fan
 ********************************************************************/

// Set the fan to the indicated speed, and update the fan indicator LEDs.
void Main::setFanSpeed(FanSpeed speed)
{
    fanController.setDutyCycle(fanDutyCycles[speed]);
    
    hw.digitalWrite(FAN_LOW_LED_PIN,  LED_ON);
    hw.digitalWrite(FAN_MED_LED_PIN,  speed >  fanLow  ? LED_ON : LED_OFF);
    hw.digitalWrite(FAN_HIGH_LED_PIN, speed == fanHigh ? LED_ON : LED_OFF);

    currentFanSpeed = speed;

    // disable fan RPM monitor for a few seconds, until the new fan speed stabilizes
    dontCheckFanSpeedUntil = hw.millis() + INTERVAL_3sec;
}

// Call this periodically to check that the fan RPM is within the expected range for the current FanSpeed.
void Main::updateFan() {
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
void Main::updateBattery() {
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

    // ...Start a new averaging period
    nextBatteryCheckMillis = now + INTERVAL_500ms;
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

/********************************************************************
 * Events
 ********************************************************************/

// Handler for Fan Down button
void Main::onFanDownLongPress(const int)
{
    instance->setFanSpeed((instance->currentFanSpeed == fanHigh) ? fanMedium : fanLow);
}

// Handler for Fan Up button
void Main::onFanUpLongPress(const int)
{
    instance->setFanSpeed((instance->currentFanSpeed == fanLow) ? fanMedium : fanHigh);
}


// Handler for the Power Off button
void Main::onMonitorLongPress(const int state)
{
    instance->realOnMonitorLongPress(state);
}

void Main::realOnMonitorLongPress(const int state) {
    // Turn on all LEDs, and the buzzer
    allLEDsOn();
    hw.analogWrite(BUZZER_PIN, BUZZER_ON);

    // Wait until the power goes off, or the user releases the button.
    while (hw.digitalRead(MONITOR_PIN) == BUTTON_PUSHED) {}

    // The user must have pushed and released the button very quickly, not long enough
    // for the machine to shut itself off. Go back to normal.
    allLEDsOff();
    hw.analogWrite(BUZZER_PIN, BUZZER_OFF);
    setFanSpeed(currentFanSpeed); // to update the fan speed LEDs
    // The battery level indicator will be updated by updateBattery() on the next call to loop().
}

/********************************************************************
 * Startup and run
 ********************************************************************/

 // prescalerSelect is 0..8, giving division factor of 1..256
void setClockPrescaler(int prescalerSelect)
{
    noInterrupts();
    CLKPR = (1 << CLKPCE);
    CLKPR = prescalerSelect;
    interrupts();
}

Main::Main() :
    buttonFanUp(FAN_UP_PIN, INTERVAL_500ms, onFanUpLongPress),
    buttonFanDown(FAN_DOWN_PIN, INTERVAL_500ms, onFanDownLongPress),
    buttonPowerOff(MONITOR_PIN, 50, onMonitorLongPress),
    fanController(FAN_RPM_PIN, FAN_SPEED_READING_INTERVAL, FAN_PWM_PIN)
{
    instance = this;
}

void Main::setup()
{
    setClockPrescaler(0);

    // Initialize the hardware
    hw.configurePins();
    hw.initializeDevices();

    #ifdef MYSERIAL
    initSerial();
    myPrintf("PAPR startup\r\n");
    #endif

    // Initialize the fan
    fanController.begin();
    setFanSpeed(defaultFanSpeed);

    nextBatteryCheckMillis = hw.millis() + INTERVAL_500ms;
}

void Main::loop()
{
    buttonFanUp.update();
    buttonFanDown.update();
    buttonPowerOff.update();
    alertTimer.update();
    if (currentAlert != alertFanRPM) updateFan();
    if (currentAlert != alertBatteryLow) updateBattery();
}

Main* Main::instance;

unsigned long getMillis()
{
    return Main::instance->millis();
}