/*
 * Main.cpp
 *
 * The main program of the PAPR product firmware.
 * 
 */
#include "Main.h"
#include <LowPower.h>
#include "MySerial.h"

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

// When we change the fan speed, allow at least this many milliseconds before checking the speed.
const int FAN_STABILIZE_MILLIS = 3000;

/********************************************************************
 * Button constants
 ********************************************************************/

// The user must push a button for at least this many milliseconds.
const int BUTTON_DEBOUNCE_MILLIS = 750;

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
 * Battery and power constants
 ********************************************************************/

// Battery levels of interest to the UI. These are determined empirically.
/*
measured:
    25.2 volts = 819
    21.6 volts = 703
    cutoff 16.2 volts = 525
*/

// Power Modes
const int FULL_POWER = 1;
const int LOW_POWER = 0;

const double ampsPerChargeFlowUnit = .0065;
const float voltsPerVoltageUnit = 123;
const double batteryCapacityCoulombs = 25200;
const double chargeAmpsWhenFull = 0.2;
const float voltsWhenAlmostEmpty = 17.5; // about 14 minutes away from shutdown (in a new battery, will be 10-12 minutes with an older battery).

/********************************************************************
 * Alert constants
 ********************************************************************/

// Which LEDs to flash for each type of alert.
const int batteryLowLEDs[] = { BATTERY_LED_LOW_PIN, CHARGING_LED_PIN , -1 };
const int fanRPMLEDs[] = { FAN_LOW_LED_PIN, FAN_MED_LED_PIN, FAN_HIGH_LED_PIN, -1 };
const int* alertLEDs[] = { 0, batteryLowLEDs, fanRPMLEDs };

// What are the on & off durations for the pulsed lights and buzzer for each type of alert. 
const int batteryAlertMillis[] = { 1000, 1000 };
const int fanAlertMillis[] = { 200, 200 };
const int* alertMillis[] = { 0, batteryAlertMillis, fanAlertMillis };

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

void Main::onToggleAlert()
{
    instance->realOnToggleAlert();
}

// This function pulses the lights and buzzer during an alert.
void Main::realOnToggleAlert()
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
    currentAlert = alert;
    currentAlertLEDs = alertLEDs[alert];
    currentAlertMillis = alertMillis[alert];
    alertToggle = false;
    realOnToggleAlert();
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

    // disable fan RPM monitor for a few seconds, until the new fan speed stabilizes
    dontCheckFanSpeedUntil = hw.millis() + FAN_STABILIZE_MILLIS;
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
        // enterAlertState(alertFanRPM);
    }
}

/********************************************************************
 * Battery and power
 ********************************************************************/

bool Main::isCharging()
{
    return currentFanSpeed == fanHigh; // hw.analogRead(CHARGE_FLOW_PIN) < 512;
}

void Main::stateOfChargeUpdate()
{
    // assert (we are not sleeping)

    // Calculate the time since our last sample, and grab a new sample. Do these back-to-back to help keep timing accurate.
    unsigned long now = hw.micros();
    long chargeFlow = hw.analogRead(CHARGE_FLOW_PIN); // 0 to 1023, < 512 means charging
    chargeFlow = isCharging() ? -51200 : 51200; // FOR TESTING ONLY, UNTIL THE PIN IS WORKING

    // Calculate the time interval between this sample and the previous.
    unsigned long deltaMicros = now - lastBatteryUpdateMicros; // check that wraparound works properly
    lastBatteryUpdateMicros = now;

    // Use the Charge Flow input to calculate the current that is flowing into or out of the battery.
    // We are assuming that the reported flow rate was constant between the current and previous samples.
    chargeFlow -= 512; // -512 to +511
    double chargeFlowAmps = ((double)-chargeFlow) * ampsPerChargeFlowUnit; // > 0 means charging
    double deltaSeconds = ((double)deltaMicros) / 1000000.0;

    // update our counter of the battery charge
    batteryCoulombs += chargeFlowAmps * deltaSeconds;

    // if the battery has reached the maximum charge, we can safely assume that the battery coulomb counter can be
    // set to 100% of the battery capacity. We know we've reached the maximum charge when the charging current drops below approx 200 mA.
    if (chargeFlow < 512 && chargeFlowAmps < chargeAmpsWhenFull) {
        batteryCoulombs = batteryCapacityCoulombs;
    }

    // if the battery has reached the minimum charge, we can safely assume that the battery coulomb counter can be
    // set to 0% of the battery capacity. We know we've reached the minimum charge when the voltage drops below approx 17.5 volts.
    #if 0
    int batteryVoltageInUnits = hw.analogRead(BATTERY_VOLTAGE_PIN);
    float batteryVoltage = ((float)batteryVoltageInUnits) * voltsPerVoltageUnit;
    if (batteryVoltage < voltsWhenAlmostEmpty) {
        batteryCoulombs = 0;
    }
    #endif
}

void Main::setPowerMode(int mode)
{
    if (mode == FULL_POWER) {
        // Set the PCB to Full Power mode.
        hw.digitalWrite(BOARD_POWER_PIN, HIGH);

        // Wait for things to settle down
        hw.delay(10);

        // Set the clock prescaler to give the max speed.
        hw.setClockPrescaler(0);

        // We are now running at full power, full speed.
    } else {
        // Full speed doesn't work in low power mode, so drop our speed to 1 MHz (8 MHz internal oscillator divided by 2**3). 
        hw.setClockPrescaler(3);

        // Now we can enter low power mode,
        hw.digitalWrite(BOARD_POWER_PIN, LOW);

        // We are now running at low power, low speed.
    }
}

void Main::enterPowerState(PowerState newState)
{
    MySerial::printf("enter state %d", newState);
    powerState = newState;
    switch (newState) {
        case powerOn:
        case powerOnCharging:
            setPowerMode(FULL_POWER);
            setFanSpeed(currentFanSpeed);
            updateFanLEDs();
            updateBatteryLEDs();
            break;

        case powerOff:
            setPowerMode(LOW_POWER);
            allLEDsOff();
            break;

        case powerOffCharging:
            //setPowerMode(LOW_POWER);
            allLEDsOff();
            updateBatteryLEDs();
            break;
    }
}

// Call this to update the battery level LEDs, and raise an alert if the level gets too low.
void Main::updateBatteryLEDs() {
    int percentFull = (int)(batteryCoulombs / batteryCapacityCoulombs * 100.0);

    // Turn on/off the battery LEDs as required
    hw.digitalWrite(BATTERY_LED_LOW_PIN,   (percentFull < 40)                        ? LED_ON : LED_OFF); // red
    hw.digitalWrite(BATTERY_LED_MED_PIN,  ((percentFull > 15) && (percentFull < 95)) ? LED_ON : LED_OFF); // yellow
    hw.digitalWrite(BATTERY_LED_HIGH_PIN,  (percentFull > 70)                        ? LED_ON : LED_OFF); // green

    // Turn on/off the charging indicator LED as required
    hw.digitalWrite(CHARGING_LED_PIN, isCharging() ? LED_ON : LED_OFF);
}

void Main::updateBattery()
{
    int percentFull = (int)(batteryCoulombs / batteryCapacityCoulombs * 100.0);

    if (percentFull < 8) {
        enterAlertState(alertBatteryLow);
    }
}

void Main::nap()
{
    setPowerMode(LOW_POWER);
    hw.wdt_disable();
    while (true) {
        LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
        hw.digitalWrite(FAN_HIGH_LED_PIN, LED_ON);
        hw.delay(10);
        hw.digitalWrite(FAN_HIGH_LED_PIN, LED_OFF);
        long wakeupTime = millis();
        int pin = hw.digitalRead(POWER_PIN);
        while (hw.digitalRead(POWER_PIN) == BUTTON_PUSHED) {
            if (hw.millis() - wakeupTime > 125) { // we're at 1/8 speed, so this is really 1000 ms (8 * 125)
                enterPowerState(powerOn);
                while (hw.digitalRead(POWER_PIN) == BUTTON_PUSHED) {}
                hw.wdt_enable(WDTO_8S);
                return;
            }
        }
    }
}

void Main::realOnPowerPress(const int state)
{
    switch (powerState) {
        case powerOn:
            enterPowerState(powerOff);
            break;

        case powerOff:
            enterPowerState(powerOn);
            break;

        case powerOnCharging:
            enterPowerState(powerOffCharging);
            break;

        case powerOffCharging:
            enterPowerState(powerOnCharging);
            break;
    }

#if 0
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
#endif
}

void Main::realPowerButtonInterruptCallback()
{
    if (hw.digitalRead(POWER_PIN) == BUTTON_PUSHED && hw.digitalRead(FAN_UP_PIN) == BUTTON_PUSHED && hw.digitalRead(FAN_DOWN_PIN) == BUTTON_PUSHED) {
        // it's a user interrupt
        hw.reset();
        // for testing - cause a watchdog timeout
        //while (true) {
        //    hw.digitalWrite(ERROR_LED_PIN, LED_ON);
        //    hw.digitalWrite(ERROR_LED_PIN, LED_OFF);
        //}
    }
}

/********************************************************************
 * Static event handlers. These simply call the corresponding instance methods.
 ********************************************************************/

 // Handler for Fan Down button
void Main::onFanDownPress(const int)
{
    instance->setFanSpeed((instance->currentFanSpeed == fanHigh) ? fanMedium : fanLow);
}

// Handler for Fan Up button
void Main::onFanUpPress(const int)
{
    instance->setFanSpeed((instance->currentFanSpeed == fanLow) ? fanMedium : fanHigh);
}

// Handler for the Power button
void Main::onPowerPress(const int state)
{
    instance->realOnPowerPress(state);
}

void Main::powerButtonInterruptCallback()
{
    instance->realPowerButtonInterruptCallback();
}

/********************************************************************
 * Startup and run
 ********************************************************************/

Main::Main() :
    buttonFanUp(FAN_UP_PIN, BUTTON_DEBOUNCE_MILLIS, onFanUpPress),
    buttonFanDown(FAN_DOWN_PIN, BUTTON_DEBOUNCE_MILLIS, onFanDownPress),
    buttonPower(MONITOR_PIN, BUTTON_DEBOUNCE_MILLIS, onPowerPress),
    alertTimer(onToggleAlert),
    fanController(FAN_RPM_PIN, FAN_SPEED_READING_INTERVAL, FAN_PWM_PIN),
    currentFanSpeed(fanLow),
    batteryCoulombs(batteryCapacityCoulombs / 2),
    currentAlert(alertNone)
{
    instance = this;
    lastBatteryUpdateMicros = hw.micros();
}

void Main::setup()
{
    // Make sure watchdog is off. Remember what kind of reset just happened.
    int resetFlags = hw.watchdogStartup();

    // If the power has just come on, then the PCB is in Low Power mode, and the MCU
    // is running at 1 MHz (because the CKDIV8 fuse bit is programmed). 
    // Bump us up to full soeed.
    setPowerMode(FULL_POWER);

    // Initialize the hardware
    hw.configurePins();
    hw.initializeDevices();
    hw.digitalWrite(POWER_PIN, BUTTON_RELEASED); // TEMP for some reason, this pin's initial value is "pushed"

    MySerial::init();
    MySerial::printf("PAPR Rev 3, MCUSR = %x, pow %d, down %d, up %d", resetFlags,
        hw.digitalRead(POWER_PIN), hw.digitalRead(FAN_DOWN_PIN), hw.digitalRead(FAN_UP_PIN));

    // Decide what power state we should be in.
    // If the reset that just happened was NOT a simple power-on, then flash some LEDs to tell the user something happened.
    PowerState initialState = powerOff;
    if (resetFlags & (1 << WDRF)) {
        // Watchdog timer expired.
        flashAllLEDs(100, 5);
        initialState = powerOn;
    } else if (resetFlags == 0) {
        // Manual reset
        flashAllLEDs(100, 10);
        initialState = powerOn;
    } else {
        // The power just came on. This will happen when:
        // - somebody in the factory just connected the battery to the PCB; or
        // - the battery had been fully drained (and therefore not delivering any power), but the user just plugged in the charger.
        initialState = powerOff;
    }

    // Initialize the fan
    fanController.begin();
    setFanSpeed(defaultFanSpeed);

    // Enable the watchdog timer. (Note: Don't make the timeout value too small - we need to give the IDE a chance to
    // call the bootloader in case something dumb happens during development and the WDT
    // resets the MCU too quickly. Once the code is solid, you could make it shorter.)
    wdt_enable(WDTO_8S);

    hw.setPowerButtonInterruptCallback(powerButtonInterruptCallback);

    enterPowerState(initialState);
}

void Main::doUpdates()
{
    stateOfChargeUpdate();
    updateBatteryLEDs();
    if (currentAlert != alertFanRPM) updateFan();
    if (currentAlert != alertBatteryLow) updateBattery();
    buttonFanUp.update();
    buttonFanDown.update();
    buttonPower.update();
    alertTimer.update();
}

unsigned long lastChargeUpdateMillis = 0;

void Main::loop()
{
    // TEMP
    unsigned long now = hw.millis();
    if (now - lastChargeUpdateMillis > 5000) {
        int percentFull = (int)(batteryCoulombs / batteryCapacityCoulombs * 100.0);
        MySerial::printf("batteryCoulombs %ld = %d percent", (long)batteryCoulombs, percentFull);
        lastChargeUpdateMillis = now;
    }

    hw.wdt_reset_();

    switch (powerState) {
        case powerOn:
            doUpdates();
            if (isCharging()) {
                enterPowerState(powerOnCharging); 
            }
            break;

        case powerOff:
            // Nothing to do, take a nap.
            nap();
            lastBatteryUpdateMicros = hw.micros();
            if (isCharging()) {
                enterPowerState(powerOffCharging);
            }
            break;

        case powerOnCharging:
            doUpdates();
            if (!isCharging()) {
                enterPowerState(powerOn);
            }
            break;

        case powerOffCharging:
            stateOfChargeUpdate();
            updateBatteryLEDs();
            buttonPower.update();
            if (!isCharging()) {
                enterPowerState(powerOff);
            }
            break;
    }
}

Main* Main::instance;

unsigned long getMillis()
{
    return Main::instance->millis();
}