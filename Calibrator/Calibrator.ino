#include "FanController.h"
#include "PressDetector.h"
#include "MySerial.h"
#include "Hardware.h"
#include <limits.h>

// This app exercises all the pins defined in Hardware.h

///////////////////////////////////////////////////////////////////////
//
// Sampler
//
///////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////
//
// Data
//
///////////////////////////////////////////////////////////////////////

FanController fanController(FAN_RPM_PIN, 1000, FAN_PWM_PIN);
int currentDutyCycle;

unsigned long samplePeriodEndMillis;
bool skipReport;
Sampler voltage;
Sampler rpm;
Sampler current;
Sampler rawRPM;
Sampler referenceVoltage;
Sampler computedCurrent;

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

int increment = 10;
bool skipUpRelease = false;
bool skipDownRelease = false;


///////////////////////////////////////////////////////////////////////
//
// Code
//
///////////////////////////////////////////////////////////////////////


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

void setFanDutyCycle(int dutyCycle)
{
    if (dutyCycle < -10) dutyCycle = -10;
    if (dutyCycle > 100) dutyCycle = 100;
    currentDutyCycle = dutyCycle;
    if (currentDutyCycle >= 0) {
        digitalWrite(FAN_ENABLE_PIN, FAN_ON);
        fanController.setDutyCycle(dutyCycle);
    } else {
        digitalWrite(FAN_ENABLE_PIN, FAN_OFF);
    }
    beginSamplePeriod();
    skipReport = true;
    serialPrintf("Duty cycle %d\r\n\r\n", dutyCycle);
}

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
    Hardware::instance.setup();
    initializeSerial();
    fanController.begin();
    setFanDutyCycle(-10);
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