#include "Battery.h"
#include "Hardware.h"
#include "MySerial.h"

#define hw Hardware::instance

/********************************************************************
 * Battery and power constants
 ********************************************************************/

const long long lowPassFilterN = 100LL;

const int BATTERY_VOLTAGE_UPDATE_INTERVAL_MILLISECS = 500;
const unsigned long CHARGER_WINDDOWN_TIME_MILLIS = 5 * 60 * 1000; // 5 minutes in milliseconds
const long long CHARGE_MICRO_AMPS_WHEN_FULL = 200000; // 0.2 Amps
const long long BATTERY_MILLI_VOLTS_CHANGED_THRESHOLD = 100; // 0.1 volts

void Battery::wakeUp() {
    lastCoulombsUpdateMicroSecs = hw.micros();
    microVolts = 20000000LL;
    chargeStartMilliSecs = hw.millis();
    lastVoltageChangeMilliSecs = hw.millis();
    prevIsCharging = false;
    prevMicroVolts = 0;
}

Battery::Battery()
{
    wakeUp();
    picoCoulombs = BATTERY_CAPACITY_PICO_COULOMBS / 2; // TODO use the voltage to estimate this
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
        // will definitely be greater than 10 volts, and if not connected it will definitely be less than 10 volts.
        return hw.readMicroVolts() > 10000000; // we have to use readMicroVolts() instead of microVolts, because microVolts doesn't get updated when in low power mode
    }

    long long chargeFlowMicroAmps = hw.readMicroAmps();

    if (systemActive) {
        // We know that the system is consuming at least 50 milliAmp. If the consumption seems to be way lower, we deduce that the charger must be connected. 
        return chargeFlowMicroAmps > -50000;
    } else {
        // The system is inactive, and therefore total consumption right now is very low, probably within the margin of
        // error of the current sensor.

        if (chargeFlowMicroAmps > 50000) {
            // we are definitely charging.
            return true;
        }

        if (chargeFlowMicroAmps < -50000) {
            // we are definitely discharging
            return false;
        }

        // At this point we don't really know if the charger is connected or not. 
        // The only way to tell is to temporarily switch the board to low power mode.
        // (it is safe to go into low power mode because (a) the system is inactive and
        // therefore we won't be disrupting anything important, and (b) the current is
        // very low so we won't be disruptive to coulomb counting.
        hw.setPowerMode(lowPowerMode);
        bool result = hw.readMicroVolts() > 10000000;
        hw.setPowerMode(fullPowerMode);
        return result;
    }
}

// Update "microVolts" which is just a low-pass filtered version of hw.readMicroVolts(). We do the filtering to smooth out random variations in the readings.
// This is probably not necessary because the readings are very stable, and because the voltage is only used in updateBatteryTimers() which
// already has a margin of slop.
void Battery::updateBatteryVoltage()
{
    microVolts = ((microVolts * lowPassFilterN) + hw.readMicroVolts()) / (lowPassFilterN + 1);
}

void Battery::updateBatteryTimers()
{
    bool isChargingNow = isCharging();
    if (isChargingNow && !prevIsCharging) {
        // we have just started charging
        chargeStartMilliSecs = hw.millis();
        //serialPrintf("Start charging at %ld", chargeStartMilliSecs);
    }
    prevIsCharging = isChargingNow;

    if (abs(microVolts - prevMicroVolts) >= BATTERY_MILLI_VOLTS_CHANGED_THRESHOLD) {
        // voltage has changed since last time we checked
        lastVoltageChangeMilliSecs = hw.millis();
        prevMicroVolts = microVolts;
        //serialPrintf("Voltage changed at %ld", lastVoltageChangeMilliSecs);
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

    // TaKe a sample of the current. Read the clock and the current as close as possible to the same moment.
    long long nowMicroSecs = hw.micros();
    long long chargeFlowMicroAmps = hw.readMicroAmps();

    // Note: even though there is a lot of random variation in charge flow readings (maybe 5-10%), we don't bother smoothing out the
    // data because the data will get smoothed as we accumulate picoCoulombs in many small increments.

    // Calculate the time interval between this sample and the previous, then use that to calculate how much charge
    // has flowed into/outof the battery since the last sample. We will assume that the current remained constant
    // between the current and previous samples.
    unsigned long deltaMicroSecs = nowMicroSecs - lastCoulombsUpdateMicroSecs;
    lastCoulombsUpdateMicroSecs = nowMicroSecs;
    long long deltaPicoCoulombs = chargeFlowMicroAmps * deltaMicroSecs;

    // update our counter of the battery charge. Don't let the number get out of range.
    picoCoulombs = picoCoulombs + deltaPicoCoulombs;
    picoCoulombs = constrain(picoCoulombs, 0, BATTERY_CAPACITY_PICO_COULOMBS);

    //serialPrintf("chargeFlowMicroAmps %s deltaMicroSecs %s deltaPicoCoulombs %s picoCoulombs %s",
    //    renderLongLong(chargeFlowMicroAmps), renderLongLong(deltaMicroSecs),
    //    renderLongLong(deltaPicoCoulombs), renderLongLong(picoCoulombs));
 
    // if the battery has reached the maximum charge, we can safely set the battery coulomb counter
    // to 100% of the battery capacity. We know we've reached the maximum charge when:
    unsigned long nowMillis = hw.millis();
    if ((picoCoulombs != BATTERY_CAPACITY_PICO_COULOMBS) &&
        (chargeFlowMicroAmps >= -10000) &&                                           // we're charging right now, AND
        (nowMillis - chargeStartMilliSecs > CHARGER_WINDDOWN_TIME_MILLIS) &&       // we've been charging for a few minutes. AND
        (nowMillis - lastVoltageChangeMilliSecs > CHARGER_WINDDOWN_TIME_MILLIS) && // the battery voltage hasn't changed for a few minutes, AND
        chargeFlowMicroAmps < CHARGE_MICRO_AMPS_WHEN_FULL)                                 // the current flow rate is below a threshold
    {
        serialPrintf("Charge full. PicoCoulombs was %s. ChargeFlow %s microAmps.", renderLongLong(picoCoulombs), renderLongLong(chargeFlowMicroAmps));
        picoCoulombs = BATTERY_CAPACITY_PICO_COULOMBS;
    }
}
