#include "FC.h"
#include "PressDetector.h"
#include "MySerial.h"
#include "Hardware.h"
#include <limits.h>

// This app exercises all pins except POWER_OFF_PIN, POWER_ON_PIN.

FanController fanController(FAN_RPM_PIN, 1000, FAN_PWM_PIN);

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

    double daverage()
    {
        return sampleCount > 0 ? (((double)accumulator) / ((double)sampleCount)) : 0;
    }
};

unsigned long samplePeriodEndMillis;
bool skipReport;
Sampler voltage;
Sampler rpm;
Sampler current;
Sampler rawRPM;

bool toneOn = false;

float filteredBatteryLevel = 0.0;
const float lowPassFilterN = 500.0;

void onUpButton();
void onDownButton();
void onOnButton();
void onOffButton();

PressDetector offButton(POWER_OFF_PIN, 100, onOffButton);
PressDetector onButton(POWER_ON_PIN, 100, onOnButton);
PressDetector downButton(FAN_DOWN_PIN, 100, onDownButton);
PressDetector upButton(FAN_UP_PIN, 100, onUpButton);

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
    rawRPM.reset();
}

void takeSample()
{
    unsigned int batteryLevel = analogRead(BATTERY_VOLTAGE_PIN);
    unsigned int chargeCurrent = analogRead(CHARGE_CURRENT_PIN);

    rpm.sample(fanController.getSpeed());
    rawRPM.sample(digitalRead(FAN_RPM_PIN));
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
        float batteryVolts = voltage.average() * VOLTS_PER_VOLTAGE_UNIT;

        long chargeFlow = current.average(); // 0 to 1023, < 512 means charging, the units are arbitrary "charge flow units"
        chargeFlow -= 512; // shift the range to -512(charging) to +511(discharging)
        double chargeFlowAmps = ((double)-chargeFlow) * AMPS_PER_CHARGE_FLOW_UNIT; // convert to amps, and flip so that > 0 means charging

        char buffer1[50];
        char buffer2[50];
        serialPrintf("Duty cycle %d, RPM min %lu, avg %lu, max %lu, VOLTAGE min %lu, avg %lu, max %lu, filt %s, comp %s, CURRENT min %lu, avg %lu, max %lu, comp %s, TONE %s, samples %lu",
            currentDutyCycle,
            rpm.lowest, rpm.average(), rpm.highest,
            voltage.lowest, voltage.average(), voltage.highest, renderDouble(filteredBatteryLevel), renderDouble(batteryVolts, buffer1),
            current.lowest, current.average(), current.highest, renderDouble(chargeFlowAmps, buffer2),
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

void onUpButton()
{
    setFanDutyCycle(currentDutyCycle + increment);
}

void onDownButton()
{
    setFanDutyCycle(currentDutyCycle - increment);
}

void onOffButton() {
    toneOn = !toneOn;
    if (toneOn) {
        analogWrite(BUZZER_PIN, 128);
    }
    else {
        analogWrite(BUZZER_PIN, 0);
    }
    serialPrintf("Sound is now %s", toneOn ? "on" : "off");
}

void onOnButton() {
    increment = (increment == 1) ? 10 : 1;
    serialPrintf("Increment is now %d", increment);
}

void initializeSerial() {
    Serial.begin(57600);
    delay(10);
    Serial.println("\n\nPAPR Calibrator for Rev 3.0A board");
    Serial.println("Off button: toggle sound");
    Serial.println("On button: toggle increment");
    Serial.println("Down button: decrease fan speed");
    Serial.println("Up button: increase fan speed\n");
}

class PowerOnButtonInterruptCallback : public InterruptCallback {
public:
    virtual void callback() {
        serialPrintf("PowerOnButtonInterruptCallback, button is now %s", 
            (digitalRead(POWER_ON_PIN) == BUTTON_PUSHED) ? "pushed" : "released");
    }
};

PowerOnButtonInterruptCallback powerOnButtonInterruptCallback;
//unsigned long loopCount;
//unsigned long startMillis;

void setup()
{
    Hardware::instance.setPowerMode(fullPowerMode);
    Hardware::instance.configurePins();
    Hardware::instance.initializeDevices();
    initializeSerial();
    fanController.begin();
    setFanDutyCycle(0);
    flashLEDs(500, 3);
    digitalWrite(FAN_LOW_LED_PIN, LED_ON);
    digitalWrite(BATTERY_LED_MED_PIN, LED_ON);
    Hardware::instance.setPowerOnButtonInterruptCallback(&powerOnButtonInterruptCallback);
    //loopCount = 0;
    //startMillis = millis();
}

void loop()
{
    offButton.update();
    onButton.update();
    downButton.update();
    upButton.update();
    fanController.getSpeed();

    if (millis() >= samplePeriodEndMillis) {
        endSamplePeriod();
        beginSamplePeriod();
    }

    takeSample();

    //loopCount += 1;
    //if (millis() - startMillis >= 10000) {
    //    serialPrintf("%ld loops in %d seconds", loopCount, 10);
    //    loopCount = 0;
    //    startMillis = millis();
    //}
}