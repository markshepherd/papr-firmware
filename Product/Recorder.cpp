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

unsigned long samplePeriodEndMillis;
bool skipReport;
Sampler voltage;
Sampler rpm;
Sampler current;
Sampler rawRPM;
Sampler referenceVoltage;
Sampler computedCurrent;
Sampler computedVoltage;

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

void updateRecorder(unsigned int fanSpeed, int currentDutyCycle, bool isCharging, double coulombs)
{
    if (millis() >= samplePeriodEndMillis) {
        if (!skipReport) {
            float batteryVolts = voltage.average() * VOLTS_PER_VOLTAGE_UNIT;
            int buzzerDutyCycle = (OCR1BH << 8) | OCR1BL;
            bool toneOn = buzzerDutyCycle != 0;
            char buffer1[50];
            char buffer2[50];
            char buffer3[50];
            serialPrintf("Duty cycle %d  RPM %l, %l, %l  VOLTAGE %l, %l, %l  filt %s comp %s\
  CURRENT %l, %l, %l  REF %l, %l, %l  μAMPS %l, %l, %l  mVOLTS %l, %l, %l  TONE %s  CHARGING %s  COULOMBS %s  SAMPLES %lu",
                currentDutyCycle,
                rpm.lowest, rpm.average(), rpm.highest,
                voltage.lowest, voltage.average(), voltage.highest, renderDouble(filteredBatteryLevel), renderDouble(batteryVolts, buffer1),
                current.lowest, current.average(), current.highest,
                referenceVoltage.lowest, referenceVoltage.average(), referenceVoltage.highest,
                computedCurrent.lowest, computedCurrent.average(), computedCurrent.highest,
                computedVoltage.lowest, computedVoltage.average(), computedVoltage.highest,
                toneOn ? "on" : "off", isCharging ? "yes" : "no", renderDouble(coulombs, buffer3), rpm.sampleCount);
        }

        beginSamplePeriod();
    }

    unsigned int batteryLevel = analogRead(BATTERY_VOLTAGE_PIN);

    rpm.sample(fanSpeed);
    rawRPM.sample(digitalRead(FAN_RPM_PIN));
    voltage.sample(batteryLevel);
    referenceVoltage.sample(analogRead(REFERENCE_VOLTAGE_PIN));
    current.sample(analogRead(CHARGE_CURRENT_PIN));
    computedCurrent.sample((long)(Hardware::instance.readCurrent() * 1000000.0));
    computedVoltage.sample((long)(Hardware::instance.readVoltage() * 1000.0));

    // Do a low pass filter on the battery level.
    // I have found that the filtered battery level varies +/- 2 ADC units, which is much better
    // than the +/- 50 units we get with the unfiltered battery level. However, the 1-second average method gives
    // +/- 0.5, which is even better.
    filteredBatteryLevel = ((filteredBatteryLevel * lowPassFilterN) + (float)batteryLevel) / (lowPassFilterN + 1.0);
}

void resetRecorder() {
    beginSamplePeriod();
    skipReport = true;
}