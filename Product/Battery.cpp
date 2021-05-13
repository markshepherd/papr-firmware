#include "Battery.h"
#include "Hardware.h"
#include "MySerial.h"

#define hw Hardware::instance

/********************************************************************
 * Battery and power constants
 ********************************************************************/

const double CHARGE_AMPS_WHEN_FULL = 0.2;
const int BATTERY_VOLTAGE_UPDATE_INTERVAL_MILLIS = 500;
const double BATTERY_VOLTS_CHANGED_THRESHOLD = 0.1; // in volts
const unsigned long CHARGER_WINDDOWN_TIME_MILLIS = 5 * 60 * 1000; // 5 minutes in milliseconds

void Battery::wakeUp() {
    lastCoulombsUpdateMicros = hw.micros();
    lastVoltsUpdateMillis = hw.millis();
    voltageAccumulator = 0;
    numVoltageSamples = 0;
    volts = 20.0;
    chargeStartMillis = hw.millis();
    lastVoltageChangeMillis = hw.millis();
    prevIsCharging = false;
    prevVolts = 0;
}

Battery::Battery()
{
    wakeUp();
    coulombs = BATTERY_CAPACITY_COULOMBS / 2.0; // TODO use the voltage to estimate this
}

/*
if ~PWR_EN then V > 10 ? yes : no

if power on (fan consumes at least 100 mA)
    current > W ? yes : no		high enough that we are certain it's charging (-50 mA?)

if power off (no fan, total power consumption is only a few mA)
    if current > X then yes		high enough that we are certain it's charging (50 mA?)
    if current < Y then no		low enough that we are certain it's not charging (-50 mA?) (should never happen)
    set ~PWR_EN, V > 10 ? yes : no		
*/
bool Battery::isCharging()
{
    if (hw.getPowerMode() == lowPowerMode) {
        // When the board is in low power mode, the battery is disconnected from the charger,
        // and the voltage pin gives the charger's voltage. If the charger is connected, this voltage
        // will definitely be greater than 10, and if not connected it will definitely be less than 10.
        return hw.readVoltage() > 10.0;
    }

    double chargeFlowAmps = hw.readCurrent();

    if (systemActive) {
        // We know that the system is consuming at least 100 mA. If the consumption seems to be way lower, we deduce that the charger must be connected. 
        return chargeFlowAmps > -0.050;
    } else {
        // The system is inactive, and therefore total consumption right now is very low, probably within the margin of
        // error of the current sensor.

        if (chargeFlowAmps > 0.050) {
            // we are definitely charging.
            return true;
        }

        if (chargeFlowAmps < -0.050) {
            // we are definitely discharging
            return false;
        }

        // At this point we don't really know if the charger is connected or not. 
        // The only way to tell is to temporarily switch the board to low power mode.
        // (it is safe to go into low power mode because (a) the system is inactive and
        // therefore we won't be disrupting anything important, and (b) the current is
        // very low so we won't be disruptive to coulomb counting.
        hw.setPowerMode(lowPowerMode);
        bool result = hw.readVoltage() > 10.0;
        hw.setPowerMode(fullPowerMode);
        return result;
    }
}

void Battery::updateBatteryVoltage()
{
    unsigned long nowMillis = hw.millis();
    if (nowMillis - lastVoltsUpdateMillis > BATTERY_VOLTAGE_UPDATE_INTERVAL_MILLIS && numVoltageSamples > 0) {
        volts = voltageAccumulator / numVoltageSamples * VOLTS_PER_VOLTAGE_UNIT;
        voltageAccumulator = 0;
        numVoltageSamples = 0;
        lastVoltsUpdateMillis = nowMillis;
    }
    else {
        voltageAccumulator += hw.analogRead(BATTERY_VOLTAGE_PIN);
        numVoltageSamples += 1;
    }
}

void Battery::updateBatteryTimers()
{
    bool isChargingNow = isCharging();
    if (isChargingNow && !prevIsCharging) {
        // we have just started charging
        chargeStartMillis = hw.millis();
        serialPrintf("Start charging at %ld", chargeStartMillis);
    }
    prevIsCharging = isChargingNow;

    if (abs(volts - prevVolts) >= BATTERY_VOLTS_CHANGED_THRESHOLD) {
        // voltage has changed since last time we checked
        lastVoltageChangeMillis = hw.millis();
        prevVolts = volts;
        serialPrintf("Voltage changed at %ld", lastVoltageChangeMillis);
    }
}

void Battery::update()
{
    if (hw.getPowerMode() != fullPowerMode) {
        // this should never happen!
        return;
    }

    updateBatteryVoltage();
    updateBatteryTimers();

    // TaKe a sample of the battery current.
    unsigned long nowMicros = hw.micros();
    double chargeFlowAmps = hw.readCurrent();

    // Calculate the time interval between this sample and the previous, then use that to calculate how much charge
    // has flowed into/outof the battery since the last sample. We will assume that the current remained constant
    // between the current and previous samples.
    unsigned long deltaMicros = nowMicros - lastCoulombsUpdateMicros;
    lastCoulombsUpdateMicros = nowMicros;
    double deltaSeconds = ((double)deltaMicros) / 1000000.0;
    double deltaCoulombs = chargeFlowAmps * deltaSeconds;

    // update our counter of the battery charge. Don't let the number get out of range.
    coulombs = coulombs + deltaCoulombs;
    coulombs = constrain(coulombs, 0, BATTERY_CAPACITY_COULOMBS);

    // if the battery has reached the maximum charge, we can safely set the battery coulomb counter
    // to 100% of the battery capacity. We know we've reached the maximum charge when:
    unsigned long nowMillis = hw.millis();
    if ((coulombs != BATTERY_CAPACITY_COULOMBS) &&
        (chargeFlowAmps >= -0.010) &&                                           // we're charging right now, AND
        (nowMillis - chargeStartMillis > CHARGER_WINDDOWN_TIME_MILLIS) &&       // we've been charging for a few minutes. AND
        (nowMillis - lastVoltageChangeMillis > CHARGER_WINDDOWN_TIME_MILLIS) && // the battery voltage hasn't changed for a few minutes, AND
        chargeFlowAmps < CHARGE_AMPS_WHEN_FULL)                                 // the current flow rate is below a threshold
    {
        serialPrintf("Charge full. Coulombs was %s. ChargeFlow %ld milliAmps.", renderDouble(coulombs), long(chargeFlowAmps * 1000.0));
        coulombs = BATTERY_CAPACITY_COULOMBS;
    }
}
