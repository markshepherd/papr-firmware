#include <SoftwareSerial.h>
#include "MySerial.h"
#include "PAPRHwDefs.h"
#include <ButtonDebounce.h>
#include <FanController.h>

FanController fanController(FAN_RPM_PIN, 1000, FAN_PWM_PIN);
ButtonDebounce upButton(FAN_UP_PIN, 100);
ButtonDebounce downButton(FAN_DOWN_PIN, 100);

int currentDutyCycle;

unsigned long samplePeriodEndMillis;
bool skipReport;
unsigned long sampleCount;
unsigned int lowestFanRPM;
unsigned int highestFanRPM;
unsigned long fanRPMAccumulator;
unsigned int lowestBatteryLevel;
unsigned int highestBatteryLevel;
unsigned long batteryLevelAccumulator;

bool toneOn = false;

void beginSamplePeriod()
{
    samplePeriodEndMillis = millis() + 5000;
    skipReport = false;
    sampleCount = 0;
    lowestFanRPM = 65535;
    highestFanRPM = 0;
    fanRPMAccumulator = 0;
    lowestBatteryLevel = 65535;
    highestBatteryLevel = 0;
    batteryLevelAccumulator = 0;
}

void takeSample()
{
    sampleCount += 1;

    unsigned int rpm = fanController.getSpeed();
    fanRPMAccumulator += rpm;
    if (rpm < lowestFanRPM) {
        lowestFanRPM = rpm;
    }
    if (rpm > highestFanRPM) {
        highestFanRPM = rpm;
    }

    unsigned int batteryLevel = analogRead(BATTERY_VOLTAGE_PIN);
    batteryLevelAccumulator += batteryLevel;
    if (batteryLevel < lowestBatteryLevel) {
        lowestBatteryLevel = batteryLevel;
    }
    if (batteryLevel > highestBatteryLevel) {
        highestBatteryLevel = batteryLevel;
    }
}

void endSamplePeriod()
{
    if (!skipReport) {
        unsigned int averageRPM = fanRPMAccumulator / sampleCount;
        unsigned int averageBatteryLevel = batteryLevelAccumulator / sampleCount;
        float voltage;
        if (currentDutyCycle == 0) {
            voltage = ((averageBatteryLevel - 452) * 0.030612) + 14;
        } else if (currentDutyCycle == 100) {
            voltage = ((averageBatteryLevel - 436) * 0.029851) + 14;
        } else {
            voltage = 0;
        }
        myPrintf("Duty cycle %d, RPM min %u, avg %u, max %u, battery min %d, avg %d, max %d, voltage %d.%d, tone %s, samples %lu\r\n",
            currentDutyCycle, lowestFanRPM, averageRPM, highestFanRPM,
            lowestBatteryLevel, averageBatteryLevel, highestBatteryLevel,
            (int)voltage, int(voltage * 10) - (int(voltage) * 10), 
            toneOn ? "on" : "off", sampleCount);
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
    myPrintf("Duty cycle %d\r\n\r\n", dutyCycle);
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

void setClockPrescaler(int prescalerSelect)
{
    noInterrupts();
    CLKPR = (1 << CLKPCE);
    CLKPR = prescalerSelect;
    interrupts();
}

void setup()
{
    setClockPrescaler(0);
    initSerial();
    upButton.setCallback(onUpButton);
    downButton.setCallback(onDownButton);
    fanController.begin();
    setFanDutyCycle(0);
    pinMode(FAN_LOW_LED_PIN, OUTPUT);
    pinMode(BATTERY_LED_MED_PIN, OUTPUT);
    pinMode(BATTERY_VOLTAGE_PIN, INPUT);
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
