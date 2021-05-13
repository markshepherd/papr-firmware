/*
 * Main.cpp
 *
 * The main program of the PAPR product firmware.
 * 
 * KEEP THE MAIN LOOP RUNNING AT ALL TIMES. DO NOT USE DELAY(). 
 * 
 */
#include "Main.h"
#include <LowPower.h>
#include "MySerial.h"
#include "Hardware.h"
#include "Recorder.h"

 // The Hardware object gives access to all the microcontroller hardware such as pins and timers. Please always use this object,
 // and never access any hardware or Arduino APIs directly. This gives us the option of using a fake hardware object for unit testing.
#define hw Hardware::instance

/********************************************************************
 * Fan constants
 ********************************************************************/

// How many milliseconds should there be between readings of the fan speed. A smaller value will update
// more often, while a higher value will give more accurate and smooth readings.
const int FAN_SPEED_READING_INTERVAL = 1000;

// The duty cycle for each fan speed. Indexed by FanSpeed.
const byte fanDutyCycles[] = { 0, 10, 20 };

// The expected RPM for each fan speed. Indexed by FanSpeed.
const unsigned int expectedFanRPM[] = { 3123, 12033, 15994 };

// How much tolerance do we give when checking for correct fan RPM
const float LOWEST_FAN_OK_RPM = 0;
const float HIGHEST_FAN_OK_RPM = 1000;

// The fan speed when we startup.
const FanSpeed DEFAULT_FAN_SPEED = fanLow;

// When we change the fan speed, allow at least this many milliseconds before checking the speed.
const int FAN_STABILIZE_MILLIS = 3000;

/********************************************************************
 * Button constants
 ********************************************************************/

// The user must push a button for at least this many milliseconds.
const int BUTTON_DEBOUNCE_MILLIS = 1000;

// The power off button needs a very short debounce interval,
// so it can do a little song and dance before taking effect.
const int POWER_OFF_BUTTON_DEBOUNCE_MILLIS = 50;

// The power off button only takes effect if the user holds it pressed for at least this long.
const int POWER_OFF_BUTTON_HOLD_MILLIS = 1000;

/********************************************************************
 * LED constants
 ********************************************************************/

// A list of all the LEDs, from left to right.
const byte LEDpins[] = {
    BATTERY_LED_LOW_PIN,
    BATTERY_LED_MED_PIN,
    BATTERY_LED_HIGH_PIN,
    CHARGING_LED_PIN,
    FAN_LOW_LED_PIN,
    FAN_MED_LED_PIN,
    FAN_HIGH_LED_PIN
};
const int numLEDs = sizeof(LEDpins) / sizeof(byte);

/********************************************************************
 * Alert constants
 ********************************************************************/

// Which LEDs to flash for each type of alert.
const int batteryLowLEDs[] = { BATTERY_LED_LOW_PIN, CHARGING_LED_PIN , -1 };
const int fanRPMLEDs[] = { FAN_LOW_LED_PIN, FAN_MED_LED_PIN, FAN_HIGH_LED_PIN, -1 };
const int* alertLEDs[] = { 0, batteryLowLEDs, fanRPMLEDs }; // Indexed by enum Alert.

// What are the on & off durations for the pulsed lights and buzzer for each type of alert. 
const int batteryAlertMillis[] = { 1000, 1000 };
const int fanAlertMillis[] = { 200, 200 };
const int* alertMillis[] = { 0, batteryAlertMillis, fanAlertMillis }; // Indexed by enum Alert.

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
void Main::setLEDs(const int* pinList, int onOff)
{
    for (int i = 0; pinList[i] != -1; i += 1) {
        hw.digitalWrite(pinList[i], onOff);
    }
}

void Main::flashAllLEDs(int millis, int count)
{
    while (count--) {
        allLEDsOn();
        hw.delay(millis);
        allLEDsOff();
        hw.delay(millis);
    }
}

/********************************************************************
 * Alert
 ********************************************************************/

// This function pulses the lights and buzzer during an alert.
void Main::onToggleAlert()
{
    alertToggle = !alertToggle;
    setLEDs(currentAlertLEDs, alertToggle ? LED_ON : LED_OFF);
    hw.analogWrite(BUZZER_PIN, alertToggle ? BUZZER_ON : BUZZER_OFF);
    alertTimer.start(currentAlertMillis[alertToggle ? 0 : 1]);
}

// Enter the "alert" state. In this state we pulse the lights and buzzer to 
// alert the user to a problem. Once we are in this state, the only
// way out is for the user to turn the power off.
void Main::enterAlertState(Alert alert)
{
    serialPrintf("Begin %d Alert", alert);
    currentAlert = alert;
    currentAlertLEDs = alertLEDs[alert];
    currentAlertMillis = alertMillis[alert];
    alertToggle = false;
    onToggleAlert();
}

void Main::cancelAlert()
{
    currentAlert = alertNone;
    alertTimer.cancel();
}

/********************************************************************
 * Fan
 ********************************************************************/

void Main::updateFanLEDs()
{
    hw.digitalWrite(FAN_LOW_LED_PIN, LED_ON);
    hw.digitalWrite(FAN_MED_LED_PIN, currentFanSpeed > fanLow ? LED_ON : LED_OFF);
    hw.digitalWrite(FAN_HIGH_LED_PIN, currentFanSpeed == fanHigh ? LED_ON : LED_OFF);
}

// Set the fan to the indicated speed, and update the fan indicator LEDs.
void Main::setFanSpeed(FanSpeed speed)
{
    fanController.setDutyCycle(fanDutyCycles[speed]);
    currentFanSpeed = speed;
    updateFanLEDs();
    serialPrintf("Set Fan Speed %d", speed);

    // disable fan RPM monitor for a few seconds, until the new fan speed stabilizes
    dontCheckFanSpeedUntil = hw.millis() + FAN_STABILIZE_MILLIS;
    resetRecorder();
}

// Call this periodically to check that the fan RPM is within the expected range for the current FanSpeed.
void Main::checkForFanAlert() {
    const unsigned int fanRPM = fanController.getRPM(); 
    // Note: we call getRPM() even if we're not going to use the result, because getRPM() works better if you call it often.

    // If fan RPM checking is temporarily disabled, then do nothing.
    if (dontCheckFanSpeedUntil) {
        if (dontCheckFanSpeedUntil > hw.millis()) return;
        dontCheckFanSpeedUntil = 0;
    }

    // If the RPM is too low or too high compared to the expected value, raise an alert.
    const unsigned int expectedRPM = expectedFanRPM[currentFanSpeed];
    if ((fanRPM < (LOWEST_FAN_OK_RPM * expectedRPM)) || (fanRPM > (HIGHEST_FAN_OK_RPM * expectedRPM))) {
        enterAlertState(alertFanRPM);
    }
}

/********************************************************************
 * Battery
 ********************************************************************/

 // Call this to update the battery level LEDs.
void Main::updateBatteryLEDs() {
    int percentFull = (int)(battery.getCoulombs() / BATTERY_CAPACITY_COULOMBS * 100.0);

    // Turn on/off the battery LEDs as required
    hw.digitalWrite(BATTERY_LED_LOW_PIN, (percentFull < 40) ? LED_ON : LED_OFF); // red
    hw.digitalWrite(BATTERY_LED_MED_PIN, ((percentFull > 15) && (percentFull < 95)) ? LED_ON : LED_OFF); // yellow
    hw.digitalWrite(BATTERY_LED_HIGH_PIN, (percentFull > 70) ? LED_ON : LED_OFF); // green

    // Turn on/off the charging indicator LED as required
    hw.digitalWrite(CHARGING_LED_PIN, battery.isCharging() ? LED_ON : LED_OFF); // orange
}

void Main::checkForBatteryAlert()
{
    int percentFull = (int)(battery.getCoulombs() / BATTERY_CAPACITY_COULOMBS * 100.0);

    if (percentFull < 8) {
        enterAlertState(alertBatteryLow);
    }
}

/********************************************************************
 * states and modes
 ********************************************************************/

void Main::enterState(PAPRState newState)
{
    serialPrintf("\r\nenter state %d", newState); delay(1000);

    paprState = newState;
    switch (newState) {
        case stateOn:
        case stateOnCharging:
            battery.notifySystemActive(true);
            digitalWrite(FAN_ENABLE_PIN, FAN_ON);
            setFanSpeed(currentFanSpeed);
            analogWrite(BUZZER_PIN, BUZZER_OFF);
            if (currentAlert != alertFanRPM) {
                updateFanLEDs();
            }
            if (currentAlert != alertBatteryLow) {
                updateBatteryLEDs();
            }
            break;

        case stateOff:
        case stateOffCharging:
            battery.notifySystemActive(false);
            pinMode(BUZZER_PIN, INPUT); // tri-state the output pin, so the buzzer receives no signal and consumes no power.
            digitalWrite(FAN_ENABLE_PIN, FAN_OFF);
            currentFanSpeed = DEFAULT_FAN_SPEED;
            cancelAlert();
            allLEDsOff();
            break;
    }
}

// Be careful inside this function, it's the only place where we mess around with
// power, speed, watchdog, and sleeping. If you break this code it will mess up
// a lot of things! 
// 
// When this function returns, the board MUST be in full power mode,
// and the watchdog timer MUST be enabled. 
void Main::nap()
{
    hw.wdt_disable();
    hw.setPowerMode(lowPowerMode);
    while (true) {
        LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);

        // TEMP
        hw.digitalWrite(FAN_HIGH_LED_PIN, LED_ON);
        hw.delayMicroseconds(100);
        hw.digitalWrite(FAN_HIGH_LED_PIN, LED_OFF);

        if (battery.isCharging()) {
            hw.setPowerMode(fullPowerMode);
            enterState(stateOffCharging);
            hw.wdt_enable(WDTO_8S);
            return;
        }

        long wakeupTime = hw.millis();
        while (hw.digitalRead(POWER_ON_PIN) == BUTTON_PUSHED) {
            if (hw.millis() - wakeupTime > 125) { // we're at 1/8 speed, so this is really 1000 ms (8 * 125)
                hw.setPowerMode(fullPowerMode);
                enterState(stateOn);
                while (hw.digitalRead(POWER_ON_PIN) == BUTTON_PUSHED) {}
                hw.wdt_enable(WDTO_8S);
                return;
            }
        }
    }
}

/********************************************************************
 * UI event handlers
 ********************************************************************/

bool Main::doPowerOffWarning()
{
    // Turn on all LEDs, and the buzzer
    allLEDsOn();
    hw.analogWrite(BUZZER_PIN, BUZZER_ON);

    // If the user holds the button for long enough, we will return true,
    // which tells the caller to go ahead and enter the off state. 
    unsigned long startMillis = hw.millis();
    while (hw.digitalRead(POWER_OFF_PIN) == BUTTON_PUSHED) {
        if (hw.millis() - startMillis > POWER_OFF_BUTTON_HOLD_MILLIS) {
            allLEDsOff();
            while (hw.digitalRead(POWER_OFF_PIN) == BUTTON_PUSHED) {}
            return true;
        }
    }

    // The user did not hold the button long enough. Restore the UI
    // and tell the caller not to enter the off state.
    allLEDsOff();
    hw.analogWrite(BUZZER_PIN, BUZZER_OFF);
    if (currentAlert != alertFanRPM) {
        updateFanLEDs();
    }
    if (currentAlert != alertBatteryLow) {
        updateBatteryLEDs();
    }
    return false;
}


void Main::onPowerOnPress()
{
    switch (paprState) {
        case stateOn:
        case stateOnCharging:
        case stateOff:
            // do nothing
            break;

        case stateOffCharging:
            enterState(stateOnCharging);
            break;
    }
}

void Main::onPowerOffPress()
{
    switch (paprState) {
        case stateOn:
            if (doPowerOffWarning()) {
                enterState(stateOff);
            }
            break;

        case stateOff:
        case stateOffCharging:
            // these should never happen
            break;

        case stateOnCharging:
            if (doPowerOffWarning()) {
                enterState(stateOffCharging);
            }
            break;
    }
}

void Main::callback()
{
    if (hw.digitalRead(POWER_ON_PIN) == BUTTON_PUSHED && hw.digitalRead(FAN_UP_PIN) == BUTTON_PUSHED && hw.digitalRead(FAN_DOWN_PIN) == BUTTON_PUSHED) {
        // it's a user reset
        hw.reset();
        // TEMP cause a watchdog timeout
        //while (true) {
        //    hw.digitalWrite(ERROR_LED_PIN, LED_ON);
        //    hw.digitalWrite(ERROR_LED_PIN, LED_OFF);
        //}
    }
}

/********************************************************************
 * Static event handlers
 ********************************************************************/

void Main::staticToggleAlert()
{
    instance->onToggleAlert();
}

void Main::staticFanDownPress(const int)
{
    serialPrintf("fan down press");
    instance->setFanSpeed((instance->currentFanSpeed == fanHigh) ? fanMedium : fanLow);
}

void Main::staticFanUpPress(const int)
{
    serialPrintf("fan up press");
    instance->setFanSpeed((instance->currentFanSpeed == fanLow) ? fanMedium : fanHigh);
}

void Main::staticPowerOffPress(const int buttonState)
{
    serialPrintf("power off press");
    instance->onPowerOffPress();
}

void Main::staticPowerOnPress(const int buttonState)
{
    serialPrintf("power on press"); delay(1000);
    instance->onPowerOnPress();
}


/********************************************************************
 * Startup and run
 ********************************************************************/

Main::Main() :
    buttonFanUp(FAN_UP_PIN, BUTTON_DEBOUNCE_MILLIS, staticFanUpPress),
    buttonFanDown(FAN_DOWN_PIN, BUTTON_DEBOUNCE_MILLIS, staticFanDownPress),
    buttonPowerOff(POWER_OFF_PIN, POWER_OFF_BUTTON_DEBOUNCE_MILLIS, staticPowerOffPress),
    buttonPowerOn(POWER_ON_PIN, BUTTON_DEBOUNCE_MILLIS, staticPowerOnPress),
    alertTimer(staticToggleAlert),
    fanController(FAN_RPM_PIN, FAN_SPEED_READING_INTERVAL, FAN_PWM_PIN),
    currentFanSpeed(fanLow),
    currentAlert(alertNone)
{
    instance = this;
}

void Main::setup()
{
    // Make sure watchdog is off. Remember what kind of reset just happened. Setup the hardware.
    int resetFlags = hw.watchdogStartup();
    hw.setup();

    #ifdef SERIAL_ENABLED
    delay(1000);
    serialInit();
    serialPrintf("PAPR Rev 3, MCUSR = %x", resetFlags);
    #endif

    // Decide what power state we should be in.
    // If the reset that just happened was NOT a simple power-on, then flash some LEDs to tell the user something happened.
    PAPRState initialPowerState;
    if (resetFlags & (1 << WDRF)) {
        // Watchdog timer expired.
        flashAllLEDs(100, 5);
        initialPowerState = stateOn;
    } else if (resetFlags == 0) {
        // Manual reset
        flashAllLEDs(100, 10);
        initialPowerState = stateOn;
    } else {
        //flashAllLEDs(50, 2);
        // The power just came on. This will happen when:
        // - somebody in the factory just connected the battery to the PCB; or
        // - the battery had been fully drained (and therefore not delivering any power), and the user just plugged in the charger.
        initialPowerState = stateOff;
    }

    // Initialize the fan
    fanController.begin();
    setFanSpeed(DEFAULT_FAN_SPEED);

    // Enable the watchdog timer. (Note: Don't make the timeout value too small - we need to give the IDE a chance to
    // call the bootloader in case something dumb happens during development and the WDT
    // resets the MCU too quickly. Once the code is solid, you could make it shorter.)
    wdt_enable(WDTO_8S);

    hw.setPowerOnButtonInterruptCallback(this);
   
    if (battery.isCharging()) {
        initialPowerState = (PAPRState)((int)initialPowerState + 2);
    }
    enterState(initialPowerState);
}

void Main::doAllUpdates()
{
    battery.update();
    if (currentAlert != alertFanRPM) {
        updateFanLEDs();
    }
    if (currentAlert != alertBatteryLow) {
        updateBatteryLEDs();
    }
    if (currentAlert == alertNone) {
        checkForFanAlert();
        checkForBatteryAlert();
    }
    buttonFanUp.update();
    buttonFanDown.update();
    buttonPowerOff.update();
    alertTimer.update();
    updateRecorder(fanController.getRPM(), fanDutyCycles[currentFanSpeed], battery.isCharging(), battery.getCoulombs());
}

// TEMP
//unsigned long lastBatteryPrintMillis = 0;

// TEMP
//unsigned long loopCount = 0;
//unsigned long lastLoopCountReportMillis = 0;

// TEMP
//unsigned long lastHeartbeatToggleMillis = 0;
//bool heartbeatToggle = false;

void Main::loop()
{
    hw.wdt_reset_();

    // TEMP
    //if (hw.millis() - lastHeartbeatToggleMillis > 1000) {
    //    heartbeatToggle = !heartbeatToggle;
    //    digitalWrite(FAN_HIGH_LED_PIN, heartbeatToggle ? LED_ON : LED_OFF);
    //    lastHeartbeatToggleMillis = millis();
    //}

    // TEMP
    //unsigned long now = hw.millis();
    //if (now - lastBatteryPrintMillis > 5000) {
    //    int percentFull = (int)(batteryCoulombs / BATTERY_CAPACITY_COULOMBS * 100.0);
    //    serialPrintf("batteryCoulombs %ld = %d percent", (long)batteryCoulombs, percentFull);
    //    lastBatteryPrintMillis = now;
    //}

    // TEMP
    //if (hw.millis() - lastLoopCountReportMillis > 10000) {
    //    serialPrintf("%ld loops in %ld millis", loopCount, hw.millis() - lastLoopCountReportMillis);
    //    lastLoopCountReportMillis = hw.millis();
    //    loopCount = 0;
    //} else {
    //    loopCount += 1;
    //}
    // March 18, 2021 - one loop() takes just under 1 millisecond, at 8 Mhz clock

    switch (paprState) {
        case stateOn:
            // Update everything
            doAllUpdates();
            if (battery.isCharging()) {
                enterState(stateOnCharging); 
            }
            break;

        case stateOff:
            nap();
            // We only come out of nap() when we are no longer in stateOff.
            battery.wakeUp();
            break;

        case stateOnCharging:
            // Update everything
            doAllUpdates();
            if (!battery.isCharging()) {
                enterState(stateOn);
            }
            break;

        case stateOffCharging:
            // Only do updates related to battery
            battery.update();
            updateBatteryLEDs();
            if (!battery.isCharging()) {
                enterState(stateOff);
            }
            buttonPowerOn.update();
            updateRecorder(fanController.getRPM(), fanDutyCycles[currentFanSpeed], battery.isCharging(), battery.getCoulombs());
            break;
    }
}

Main* Main::instance;
