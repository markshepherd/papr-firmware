#include "FanController.h"
#include "PressDetector.h"
#include "MySerial.h"
#include "Hardware.h"
#include <limits.h>
#include "Recorder.h"
#include "Battery.h"

// This app exercises all the pins defined in Hardware.h

///////////////////////////////////////////////////////////////////////
//
// Data
//
///////////////////////////////////////////////////////////////////////

Battery battery;
FanController fanController(FAN_RPM_PIN, 1000, FAN_PWM_PIN);
int currentDutyCycle;
bool toneOn = false;

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
    resetRecorder();
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
    battery.notifySystemActive(true);
}

void loop()
{
    offButton.update();
    onButton.update();
    downButton.update();
    upButton.update();
    fanController.getRPM();
    battery.update();
    updateRecorder(fanController.getRPM(), currentDutyCycle, battery.isCharging(), battery.getCoulombs());

    //loopCount += 1;
    //if (millis() - startMillis >= 10000) {
    //    serialPrintf("%ld loops in %d seconds", loopCount, 10);
    //    loopCount = 0;
    //    startMillis = millis();
    //}
}