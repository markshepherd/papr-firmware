#include <FanController.h>
#include "PressDetector.h"
#include "MySerial.h"
#include "Hardware.h"
#include <limits.h>

// This app exercises all pins except POWER_OFF_PIN, POWER_ON_PIN.

FanController fanController(FAN_RPM_PIN, 1000, FAN_PWM_PIN);

Hardware hardware;

int currentDutyCycle;

class Sampler {
public:
    unsigned long highest;
    unsigned long lowest;
    unsigned long accumulator;
    unsigned long sampleCount;

    void reset()
    {
        highest = 0;
        lowest = ULONG_MAX;
        accumulator = 0;
        sampleCount = 0;
    }

    void sample(unsigned long value)
    {
        if (value < lowest) lowest = value;
        if (value > highest) highest = value;
        accumulator += value;
        sampleCount += 1;
    }

    unsigned long average()
    {
        return sampleCount > 0 ? (accumulator / sampleCount) : 0;
    }
};

unsigned long samplePeriodEndMillis;
bool skipReport;
Sampler voltage;
Sampler rpm;
Sampler current;

bool toneOn = false;

float filteredBatteryLevel = 0.0;
const float lowPassFilterN = 500.0;

void onUpButton(int state);
void onDownButton(int state);

PressDetector upButton(FAN_UP_PIN, 100, onUpButton);
PressDetector downButton(FAN_DOWN_PIN, 100, onDownButton);

void allLEDs(int state)
{
    digitalWrite(BATTERY_LED_LOW_PIN, state);
    digitalWrite(BATTERY_LED_MED_PIN, state);
    digitalWrite(BATTERY_LED_HIGH_PIN, state);
    digitalWrite(CHARGING_LED_PIN, state);
    digitalWrite(FAN_LOW_LED_PIN, state);
    digitalWrite(FAN_MED_LED_PIN, state);
    digitalWrite(FAN_HIGH_LED_PIN, state);
}

void flashLEDs(int interval, int count)
{
    while (count-- > 0) {
        allLEDs(LED_ON);
        delay(interval);
        allLEDs(LED_OFF);
        delay(interval);
    }
}


void beginSamplePeriod()
{
    samplePeriodEndMillis = millis() + 5000;
    skipReport = false;
    voltage.reset();
    rpm.reset();
    current.reset();
}

void takeSample()
{
    unsigned int batteryLevel = analogRead(BATTERY_VOLTAGE_PIN);
    unsigned int chargeCurrent = analogRead(CHARGE_CURRENT_PIN);

    rpm.sample(fanController.getSpeed());
    voltage.sample(batteryLevel);
    current.sample(chargeCurrent);

    // Do a low pass filter on the battery level.
    // I have found that the filtered battery level varies +/- 2 ADC units, which is much better
    // than the +/- 50 units we get with the unfiltered battery level. However, the 1-second average method gives
    // +/- 0.5, which is even better.
    filteredBatteryLevel = ((filteredBatteryLevel * lowPassFilterN) + (float)batteryLevel) / (lowPassFilterN + 1.0);
}

void endSamplePeriod()
{
    if (!skipReport) {
        serialPrintf("Duty cycle %d, RPM min %u, avg %u, max %u, VOLTAGE min %d, avg %d, max %d, filt %s, CURRENT min %d, avg %d, max %d, TONE %s, samples %lu\r\n",
            currentDutyCycle,
            rpm.lowest, rpm.average(), rpm.highest,
            voltage.lowest, voltage.average(), voltage.highest, renderDouble(filteredBatteryLevel),
            current.lowest, current.average(), current.highest,
            toneOn ? "on" : "off", rpm.sampleCount);
    }
}

void setFanDutyCycle(int dutyCycle)
{
    if (dutyCycle < 0) dutyCycle = 0;
    if (dutyCycle > 100) dutyCycle = 100;
    currentDutyCycle = dutyCycle;
    fanController.setDutyCycle(dutyCycle);
    beginSamplePeriod();
    skipReport = true;
    serialPrintf("Duty cycle %d\r\n\r\n", dutyCycle);
}

int increment = 10;
bool skipUpRelease = false;
bool skipDownRelease = false;

void onUpButton(int state)
{
    digitalWrite(FAN_HIGH_LED_PIN, state == BUTTON_PUSHED ? LED_ON : LED_OFF);
    if (state == BUTTON_RELEASED) {
        if (skipUpRelease) {
            skipUpRelease = false;
        } else {
            setFanDutyCycle(currentDutyCycle + increment);
        }
    } else {
        if (downButton.state() == BUTTON_PUSHED) {
            increment = (increment == 1) ? 10 : 1;
            skipUpRelease = true;
            skipDownRelease = true;
        }
    }
}

void onDownButton(int state)
{
    digitalWrite(FAN_LOW_LED_PIN, state == BUTTON_PUSHED ? LED_ON : LED_OFF);
    if (state == BUTTON_RELEASED) {
        if (skipDownRelease) {
            skipDownRelease = false;
        } else {
            setFanDutyCycle(currentDutyCycle - increment);
        }
    } else {
        if (upButton.state() == BUTTON_PUSHED) {
            toneOn = !toneOn;
            if (toneOn) {
                analogWrite(BUZZER_PIN, 128);
            } else {
                analogWrite(BUZZER_PIN, 0);
            }
            skipUpRelease = true;
            skipDownRelease = true;
        }
    }
}

void setup()
{
    hardware.setPowerMode(fullPowerMode);
    hardware.configurePins();
    hardware.initializeDevices();
    Serial.begin(57600);
    Serial.println("PAPR Calibrator");
    Serial.println("Up/Down button: increase/decrease fan speed");
    Serial.println("Up button while Down pressed: toggle increment");
    Serial.println("Down button while Up pressed: toggle sound");
    fanController.begin();
    setFanDutyCycle(0);
    flashLEDs(500, 3);
    digitalWrite(FAN_LOW_LED_PIN, LED_ON);
    digitalWrite(BATTERY_LED_MED_PIN, LED_ON);
}

void loop()
{
    upButton.update();
    downButton.update();

    if (millis() >= samplePeriodEndMillis) {
        endSamplePeriod();
        beginSamplePeriod();
    }

    takeSample();
}