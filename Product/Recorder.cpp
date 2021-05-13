#include "MySerial.h"
#include "Hardware.h"
#include <limits.h>
#include "FanController.h"

class Sampler {
public:
    long highest;
    long lowest;
    long accumulator;
    long sampleCount;

    void reset()
    {
        highest = LONG_MIN;
        lowest = LONG_MAX;
        accumulator = 0;
        sampleCount = 0;
    }

    void sample(long value)
    {
        if (value < lowest) lowest = value;
        if (value > highest) highest = value;
        accumulator += value;
        sampleCount += 1;
    }

    long average()
    {
        return sampleCount > 0 ? (accumulator / sampleCount) : 0;
    }
};

extern FanController fanController;
extern int currentDutyCycle;
extern bool toneOn;

unsigned long samplePeriodEndMillis;
bool skipReport;
Sampler voltage;
Sampler rpm;
Sampler current;
Sampler rawRPM;
Sampler referenceVoltage;
Sampler computedCurrent;

float filteredBatteryLevel = 0.0;
const float lowPassFilterN = 500.0;

void beginSamplePeriod()
{
    samplePeriodEndMillis = millis() + 5000;
    skipReport = false;
    voltage.reset();
    rpm.reset();
    current.reset();
    rawRPM.reset();
    referenceVoltage.reset();
    computedCurrent.reset();
}

void takeSample()
{
    unsigned int batteryLevel = analogRead(BATTERY_VOLTAGE_PIN);

    rpm.sample(fanController.getSpeed());
    rawRPM.sample(digitalRead(FAN_RPM_PIN));
    voltage.sample(batteryLevel);
    referenceVoltage.sample(analogRead(REFERENCE_VOLTAGE_PIN));
    current.sample(analogRead(CHARGE_CURRENT_PIN));
    computedCurrent.sample((long)(Hardware::instance.readCurrent() * 1000000.0));

    // Do a low pass filter on the battery level.
    // I have found that the filtered battery level varies +/- 2 ADC units, which is much better
    // than the +/- 50 units we get with the unfiltered battery level. However, the 1-second average method gives
    // +/- 0.5, which is even better.
    filteredBatteryLevel = ((filteredBatteryLevel * lowPassFilterN) + (float)batteryLevel) / (lowPassFilterN + 1.0);
}

void endSamplePeriod()
{
    if (!skipReport) {
        float batteryVolts = voltage.average() * VOLTS_PER_VOLTAGE_UNIT;
        char buffer1[50];
        char buffer2[50];
        serialPrintf("Duty cycle %d, RPM min %l, avg %l, max %l, VOLTAGE min %l, avg %l, max %l, filt %s, comp %s,\
 CURRENT min %l, avg %l, max %l, REF min %l, avg %l, max %l, �AMPS min %l, avg %l, max %l, TONE %s, samples %lu",
            currentDutyCycle,
            rpm.lowest, rpm.average(), rpm.highest,
            voltage.lowest, voltage.average(), voltage.highest, renderDouble(filteredBatteryLevel), renderDouble(batteryVolts, buffer1),
            current.lowest, current.average(), current.highest,
            referenceVoltage.lowest, referenceVoltage.average(), referenceVoltage.highest,
            computedCurrent.lowest, computedCurrent.average(), computedCurrent.highest,
            toneOn ? "on" : "off", rpm.sampleCount);
    }
}

void updateRecorder()
{
    if (millis() >= samplePeriodEndMillis) {
        endSamplePeriod();
        beginSamplePeriod();
    }
    takeSample();
}

void resetRecorder() {
    beginSamplePeriod();
    skipReport = true;
}