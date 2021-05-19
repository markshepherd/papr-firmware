#include "MySerial.h"
#include "Hardware.h"
#include <limits.h>
#include "FanController.h"

class Sampler {
public:
    long highest;
    long lowest;
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

private:
    long long accumulator;
};

unsigned long samplePeriodEndMillis;
bool skipReport;
Sampler voltage;
Sampler rpm;
Sampler current;
Sampler rawRPM;
Sampler referenceVoltage;
Sampler computedMicroAmps;
Sampler computedMicroVolts;

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
    computedMicroAmps.reset();
    computedMicroVolts.reset();
}

void updateRecorder(unsigned int fanRPM, int currentDutyCycle, bool isCharging, long long picoCoulombs)
{
    if (millis() >= samplePeriodEndMillis) {
        if (!skipReport) {
            long long batteryMilliVolts = voltage.average() * NANO_VOLTS_PER_VOLTAGE_UNIT / 1000000;
            int buzzerDutyCycle = (OCR1BH << 8) | OCR1BL;
            bool toneOn = buzzerDutyCycle != 0;
            char buffer1[50];
            char buffer2[50];
            char buffer3[50];
            serialPrintf("Duty %d  RPM %ld|%ld|%ld  VOLT %ld|%ld|%ld  filt %ld comp %s\
  CURR %ld|%ld|%ld  REF %ld|%ld|%ld  uAMPS %ld|%ld|%ld  uVOLTS %ld|%ld|%ld  TONE %s  CHARGE %s  PICOCOUL %s  SAMPLES %lu",
                currentDutyCycle,
                rpm.lowest, rpm.average(), rpm.highest,
                voltage.lowest, voltage.average(), voltage.highest, (long)filteredBatteryLevel, renderLongLong(batteryMilliVolts),
                current.lowest, current.average(), current.highest,
                referenceVoltage.lowest, referenceVoltage.average(), referenceVoltage.highest,
                computedMicroAmps.lowest, computedMicroAmps.average(), computedMicroAmps.highest,
                computedMicroVolts.lowest, computedMicroVolts.average(), computedMicroVolts.highest,
                toneOn ? "on" : "off", isCharging ? "yes" : "no", renderLongLong(picoCoulombs), rpm.sampleCount);
        }

        beginSamplePeriod();
    }

    unsigned int batteryLevel = analogRead(BATTERY_VOLTAGE_PIN);

    rpm.sample(fanRPM);
    rawRPM.sample(digitalRead(FAN_RPM_PIN));
    voltage.sample(batteryLevel);
    referenceVoltage.sample(analogRead(REFERENCE_VOLTAGE_PIN));
    current.sample(analogRead(CHARGE_CURRENT_PIN));
    computedMicroAmps.sample((long)(Hardware::instance.readMicroAmps()));
    computedMicroVolts.sample((long)(Hardware::instance.readMicroVolts()));

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