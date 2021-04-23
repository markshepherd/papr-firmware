#include "Battery.h"
#include "Hardware.h"

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
    chargeStartTimeMillis = hw.millis();
    lastChangeTimeMillis = hw.millis();
    prevIsCharging = false;
    prevVolts = 0;
}

Battery::Battery()
{
    wakeUp();
    coulombs = BATTERY_CAPACITY_COULOMBS / 2.0; // TODO use the voltage to estimate this
}

bool Battery::isCharging()
{
    return hw.analogRead(CHARGE_CURRENT_PIN) < (512 - 10); // TODO what's the right fudge factor?
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
        chargeStartTimeMillis = hw.millis();
        //serialPrintf("Start charging at %ld", chargeStartTimeMillis);
    }
    prevIsCharging = isChargingNow;

    if (abs(volts - prevVolts) >= BATTERY_VOLTS_CHANGED_THRESHOLD) {
        // voltage has changed since last time we checked
        lastChangeTimeMillis = hw.millis();
        prevVolts = volts;
        //serialPrintf("Voltage changed at %ld", lastVoltageChangeTimeMillis);
    }
}

void Battery::update()
{
    if (!hw.fullSpeed()) {
        // this should never happen!
        return;
    }

    updateBatteryVoltage();
    updateBatteryTimers();

    // Calculate the time since our last sample, and grab a new sample. Do these back-to-back to help keep timing accurate.
    unsigned long nowMicros = hw.micros();
    long currentReading = hw.analogRead(CHARGE_CURRENT_PIN);
    long referenceReading = hw.analogRead(REFERENCE_VOLTAGE_PIN);

    // Calculate the time interval between this sample and the previous.
    unsigned long deltaMicros = nowMicros - lastCoulombsUpdateMicros;
    lastCoulombsUpdateMicros = nowMicros;

    // Use the Charge Flow reading to calculate the current that is flowing into or out of the battery.
    // We assume that the flow rate was constant between the current and previous samples.
    double chargeFlowAmps = (currentReading - referenceReading) * AMPS_PER_CHARGE_FLOW_UNIT; // TODO maybe flip negative
    double deltaSeconds = ((double)deltaMicros) / 1000000.0;
    double deltaCoulombs = chargeFlowAmps * deltaSeconds;

    // update our counter of the battery charge. Don't let the number get out of range.
    coulombs = coulombs + deltaCoulombs;
    coulombs = constrain(coulombs, 0, BATTERY_CAPACITY_COULOMBS);

    // if the battery has reached the maximum charge, we can safely set the battery coulomb counter
    // to 100% of the battery capacity. We know we've reached the maximum charge when:
    unsigned long nowMillis = hw.millis();
    if ((coulombs != BATTERY_CAPACITY_COULOMBS) &&
        (chargeFlowAmps >= 0) &&                                                    // we're charging right now, AND
        (nowMillis - chargeStartTimeMillis > CHARGER_WINDDOWN_TIME_MILLIS) &&       // we've been charging for a few minutes. AND
        (nowMillis - lastChangeTimeMillis > CHARGER_WINDDOWN_TIME_MILLIS) && // the battery voltage hasn't changed for a few minutes, AND
        chargeFlowAmps < CHARGE_AMPS_WHEN_FULL)                                     // the current flow rate is below a threshold
    {
        //serialPrintf("Charge full. batteryCoulombs was %s. ChargeFlow %ld milliAmps.", renderDouble(batteryCoulombs), long(chargeFlowAmps * 1000.0));
        coulombs = BATTERY_CAPACITY_COULOMBS;
    }
}
