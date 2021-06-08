/*
 * Main.cpp
 *
 * The main program of the PAPR product firmware.
 * 
 * KEEP THE MAIN LOOP RUNNING AT ALL TIMES. DO NOT USE DELAY().
 * Be careful - this firmware keeps running even when the user does "Power Off". This code may need to run correctly for months at a time. Don't break this code!
 * All code must work when millis() and micros() wrap around
 * 
 */
#include "Main.h"
#include <LowPower.h>
#include "MySerial.h"
#include "Hardware.h"

 // The Hardware object gives access to all the microcontroller hardware such as pins and timers. Please always use this object,
 // and never access any hardware or Arduino APIs directly. This gives us the option of using a fake hardware object for unit testing.
#define hw Hardware::instance

const int URGENT_BATTERY_PERCENT = 8;

const char* STATE_NAMES[] = { "Off", "On", "Off Charging", "On Charging" }; // indexed by PAPRState


/********************************************************************
 * Fan constants
 ********************************************************************/

// How many milliseconds should there be between readings of the fan speed. A smaller value will update
// more often, while a higher value will give more accurate and smooth readings.
const int FAN_SPEED_READING_INTERVAL = 1000;

// The duty cycle for each fan speed. Indexed by FanSpeed.
const byte fanDutyCycles[] = { 0, 50, 100 };

// The expected RPM for each fan speed. Indexed by FanSpeed.
const unsigned int expectedFanRPM[] = { 7479, 16112, 22271 };
/* Here are measured values for fan RMP for the San Ace 9GA0412P3K011
   %    MIN     MAX     AVG

   0,    7461,   7480,    7479
  10,    9431,   9481,    9456
  20,   11264,  11284,   11274
  30,   12908,  12947,   12928
  40,   14580,  14626,   14603
  50,   16047,  16177,   16112
  60,   17682,  17743,   17743
  70,   19092,  19150,   19121
  80,   20408,  20488,   20448
  90,   21510,  21556,   21533
 100,   22215,  22327,   22271 
*/

// How much tolerance do we give when checking for correct fan RPM. We allow +/- 5%.
const float LOWEST_FAN_OK_RPM = 0.95;
const float HIGHEST_FAN_OK_RPM = 1.05;

// The fan speed when we startup.
const FanSpeed DEFAULT_FAN_SPEED = fanLow;

// When we change the fan speed, allow at least this many milliseconds before checking the speed.
const int FAN_STABILIZE_MILLIS = 6000;

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

void Main::setLED(const int pin, int onOff) {
    hw.digitalWrite(pin, onOff);
    for (int i = 0; i < numLEDs; i++) {
        if (pin == LEDpins[i]) {
            ledState[i] = onOff;
            return;
        }
    }
}

// Turn off all LEDs
void Main::allLEDsOff()
{
    for (int i = 0; i < numLEDs; i += 1) {
        setLED(LEDpins[i], LED_OFF);
    }
}

// Turn on all LEDs
void Main::allLEDsOn()
{
    for (int i = 0; i < numLEDs; i += 1) {
        setLED(LEDpins[i], LED_ON);
    }
}

// Set a list of LEDs to a given state.
void Main::setLEDs(const int* pinList, int onOff)
{
    for (int i = 0; pinList[i] != -1; i += 1) {
        setLED(pinList[i], onOff);
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

void Main::onChargeReminder() {
    serialPrintf("reminder beep");
    setBuzzer(BUZZER_ON);
    setLED(CHARGING_LED_PIN, LED_ON);
    beepTimer.start(500);
}

void Main::onStatusReport() {
    serialPrintf("Fan %s  Buzzer %s  Alert %s  Charging %s  LEDs %s %s %s, %s, %s %s %s  milliVolts %s  milliAmps %s  Coulombs %s  %d%% charged",
        (currentFanSpeed == fanLow) ? "lo" : ((currentFanSpeed == fanMedium) ? "med" : "hi"),
        (buzzerState == BUZZER_ON) ? "on" : "off",
        currentAlertName(),
        battery.isCharging() ? "yes" : "no",
        (ledState[0] == LED_ON) ? "red" : "---",
        (ledState[1] == LED_ON) ? "yellow" : "---",
        (ledState[2] == LED_ON) ? "green" : "---",
        (ledState[3] == LED_ON) ? "amber" : "---",
        (ledState[4] == LED_ON) ? "blue" : "---",
        (ledState[5] == LED_ON) ? "blue" : "---",
        (ledState[6] == LED_ON) ? "blue" : "---",
        renderLongLong(hw.readMicroVolts() / 1000LL),
        renderLongLong(hw.readMicroAmps() / 1000LL),
        renderLongLong(battery.getPicoCoulombs() / 1000000000000LL),
        getBatteryPercentFull());
}

void Main::onBeepTimer() {
    setBuzzer(BUZZER_OFF);
    setLED(CHARGING_LED_PIN, LED_OFF);
}

/********************************************************************
 * Alert
 ********************************************************************/

// This function pulses the lights and buzzer during an alert.
void Main::onToggleAlert()
{
    alertToggle = !alertToggle;
    setLEDs(currentAlertLEDs, alertToggle ? LED_ON : LED_OFF);
    setBuzzer(alertToggle ? BUZZER_ON : BUZZER_OFF);
    alertTimer.start(currentAlertMillis[alertToggle ? 0 : 1]);
}

// Enter the "alert" state. In this state we pulse the lights and buzzer to 
// alert the user to a problem. Once we are in this state, the only
// way out is for the user to turn the power off.
void Main::raiseAlert(Alert alert)
{
    currentAlert = alert;
    serialPrintf("Begin %s Alert", currentAlertName());
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

void Main::setBuzzer(int onOff) {
    hw.analogWrite(BUZZER_PIN, onOff);
    buzzerState = onOff;
}

/********************************************************************
 * Fan
 ********************************************************************/

void Main::updateFanLEDs()
{
    setLED(FAN_LOW_LED_PIN, LED_ON);
    setLED(FAN_MED_LED_PIN, currentFanSpeed > fanLow ? LED_ON : LED_OFF);
    setLED(FAN_HIGH_LED_PIN, currentFanSpeed == fanHigh ? LED_ON : LED_OFF);
}

// Set the fan to the indicated speed, and update the fan indicator LEDs.
void Main::setFanSpeed(FanSpeed speed)
{
    fanController.setDutyCycle(fanDutyCycles[speed]);
    currentFanSpeed = speed;
    updateFanLEDs();
    serialPrintf("Set Fan Speed %d", speed);

    // disable fan RPM monitor for a few seconds, until the new fan speed stabilizes
    lastFanSpeedChangeMilliSeconds = hw.millis();
    fanSpeedRecentlyChanged = true;
    //resetRecorder();
}

// Call this periodically to check that the fan RPM is within the expected range for the current FanSpeed.
void Main::checkForFanAlert() {
    const unsigned int fanRPM = fanController.getRPM(); 
    // Note: we call getRPM() even if we're not going to use the result, because getRPM() works better if you call it often.

    // If fan RPM checking is temporarily disabled, then do nothing.
    if (fanSpeedRecentlyChanged) {
        if (hw.millis() - lastFanSpeedChangeMilliSeconds < FAN_STABILIZE_MILLIS) {
            return;
        }
        fanSpeedRecentlyChanged = false;
    }

    // If the RPM is too low or too high compared to the expected value, raise an alert.
    const unsigned int expectedRPM = expectedFanRPM[currentFanSpeed];
    if ((fanRPM < (LOWEST_FAN_OK_RPM * expectedRPM)) || (fanRPM > (HIGHEST_FAN_OK_RPM * expectedRPM))) {
        raiseAlert(alertFanRPM);
    }
}

/********************************************************************
 * Battery
 ********************************************************************/

int Main::getBatteryPercentFull() {
    return (int)((battery.getPicoCoulombs() - BATTERY_MIN_CHARGE_PICO_COULOMBS) / ((BATTERY_CAPACITY_PICO_COULOMBS - BATTERY_MIN_CHARGE_PICO_COULOMBS) / 100LL));
}

// Call this to update the battery level LEDs.
void Main::updateBatteryLEDs() {
    int percentFull = getBatteryPercentFull();

    bool redLED = (percentFull < 40);
    if (percentFull <= URGENT_BATTERY_PERCENT) {
        bool ledToggle = (hw.millis() / 1000) & 1;
        redLED = redLED && ledToggle;
    }

    // Turn on/off the battery LEDs as required
    setLED(BATTERY_LED_LOW_PIN, redLED ? LED_ON : LED_OFF); // red
    setLED(BATTERY_LED_MED_PIN, ((percentFull > 15) && (percentFull < 95)) ? LED_ON : LED_OFF); // yellow
    setLED(BATTERY_LED_HIGH_PIN, (percentFull > 70) ? LED_ON : LED_OFF); // green

    // Turn on/off the charging indicator LED as required
    setLED(CHARGING_LED_PIN, battery.isCharging() ? LED_ON : LED_OFF); // orange
    
    if (!battery.isCharging() && percentFull <= 15 && currentAlert != alertBatteryLow) {
        if (!chargeReminder.isActive()) {
            onChargeReminder();
            chargeReminder.start();
        }
    } else {
        chargeReminder.stop();
    }
}

void Main::checkForBatteryAlert()
{
    if (currentAlert == alertBatteryLow) {
        if (battery.isCharging() || getBatteryPercentFull() > URGENT_BATTERY_PERCENT) {
            cancelAlert();
        }
    } else if (getBatteryPercentFull() <= URGENT_BATTERY_PERCENT && !battery.isCharging()) {
        chargeReminder.stop();
        raiseAlert(alertBatteryLow);
    }
}

/********************************************************************
 * states and modes
 ********************************************************************/

void Main::enterState(PAPRState newState)
{
    serialPrintf("\r\nenter state %s", STATE_NAMES[newState]);
    onStatusReport();

    paprState = newState;
    switch (newState) {
        case stateOn:
        case stateOnCharging:
            battery.notifySystemActive(true);
            hw.digitalWrite(FAN_ENABLE_PIN, FAN_ON);
            setFanSpeed(currentFanSpeed);
            setBuzzer(BUZZER_OFF);
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
            hw.digitalWrite(FAN_ENABLE_PIN, FAN_OFF);
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
        //setLED(FAN_HIGH_LED_PIN, LED_ON);
        //hw.delayMicroseconds(100);
        //setLED(FAN_HIGH_LED_PIN, LED_OFF);

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
    setBuzzer(BUZZER_ON);

    // If the user holds the button for long enough, we will return true,
    // which tells the caller to go ahead and enter the off state. 
    unsigned long startMillis = hw.millis();
    while (hw.digitalRead(POWER_OFF_PIN) == BUTTON_PUSHED) {
        if (hw.millis() - startMillis > POWER_OFF_BUTTON_HOLD_MILLIS) {
            allLEDsOff();
            return true;
        }
    }

    // The user did not hold the button long enough. Restore the UI
    // and tell the caller not to enter the off state.
    allLEDsOff();
    setBuzzer(BUZZER_OFF);
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

void Main::onFanDownPress()
{
    /* TEMP */
    if (digitalRead(POWER_ON_PIN) == BUTTON_PUSHED) {
        battery.DEBUG_incrementPicoCoulombs(-1500000000000000LL);
        serialPrintf("Charge is %d%", getBatteryPercentFull());
        return;
    }

    setFanSpeed((currentFanSpeed == fanHigh) ? fanMedium : fanLow);
}

void Main::onFanUpPress()
{
    /* TEMP */
    if (digitalRead(POWER_ON_PIN) == BUTTON_PUSHED) {
        battery.DEBUG_incrementPicoCoulombs(1500000000000000LL);
        serialPrintf("Charge is %d%", getBatteryPercentFull());
        return;
    }

    setFanSpeed((instance->currentFanSpeed == fanLow) ? fanMedium : fanHigh);
}

void Main::callback()
{
    if (hw.digitalRead(POWER_ON_PIN) == BUTTON_PUSHED && hw.digitalRead(FAN_UP_PIN) == BUTTON_PUSHED && hw.digitalRead(FAN_DOWN_PIN) == BUTTON_PUSHED) {
        // it's a user reset
        hw.reset();
        // TEMP cause a watchdog timeout
        //while (true) {
        //    setLED(ERROR_LED_PIN, LED_ON);
        //    setLED(ERROR_LED_PIN, LED_OFF);
        //}
    }
}


/********************************************************************
 * Startup and run
 ********************************************************************/

Main::Main() :
    // In the following code we are using the c++ lambda expressions as glue to our event handler functions.
    buttonFanUp(FAN_UP_PIN, BUTTON_DEBOUNCE_MILLIS,
        []() { instance->onFanUpPress(); }),
    buttonFanDown(FAN_DOWN_PIN, BUTTON_DEBOUNCE_MILLIS,
        []() { instance->onFanDownPress(); }),
    buttonPowerOff(POWER_OFF_PIN, POWER_OFF_BUTTON_DEBOUNCE_MILLIS, 
        []() { instance->onPowerOffPress(); }),
    buttonPowerOn(POWER_ON_PIN, BUTTON_DEBOUNCE_MILLIS, 
        []() { instance->onPowerOnPress(); }),
    alertTimer(
        []() { instance->onToggleAlert(); }),
    beepTimer(
        []() { instance->onBeepTimer(); }),
    chargeReminder(10000, 
        []() { instance->onChargeReminder(); }),
    statusReport(10000, 
        []() { instance->onStatusReport(); }),
    fanController(FAN_RPM_PIN, FAN_SPEED_READING_INTERVAL, FAN_PWM_PIN),
    currentFanSpeed(fanLow),
    fanSpeedRecentlyChanged(false),
    ledState({ LED_OFF, LED_OFF, LED_OFF, LED_OFF, LED_OFF, LED_OFF, LED_OFF}),
    buzzerState(BUZZER_OFF),
    currentAlert(alertNone)
{
    instance = this;
}

void Main::setup()
{
    //unsigned long setupStartMillis = hw.millis();
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
    //serialPrintf("setup() duration %ld mSec", hw.millis() - setupStartMillis);
    enterState(initialPowerState);
    statusReport.start();
}

void Main::doAllUpdates()
{
    battery.update();
    if (currentAlert == alertNone) {
        checkForFanAlert();
    }
    if (currentAlert != alertFanRPM) {
        updateFanLEDs();
    }
    checkForBatteryAlert();
    if (currentAlert != alertBatteryLow) {
        updateBatteryLEDs();
    }
    buttonFanUp.update();
    buttonFanDown.update();
    buttonPowerOff.update();
    alertTimer.update();
    chargeReminder.update();
    beepTimer.update();
    statusReport.update();
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
    //    setLED(FAN_HIGH_LED_PIN, heartbeatToggle ? LED_ON : LED_OFF);
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
            statusReport.update();
            break;
    }
}

Main* Main::instance;
