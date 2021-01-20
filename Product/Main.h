/*
 * Main.h
 *
 * This defines the main program of the PAPR product firmware. This file is intended to be invoked either by
 * by the Arduino runtime (product environment) or by the unit test runtime (unit test environment).
 */
#pragma once
#include "PAPRHwDefs.h"
#ifdef UNITTEST
#include "UnitTest/MyButtonDebounce.h"
#include "UnitTest/MyFanController.h"
#include "UnitTest/MyHardware.h"
#else
#include <ButtonDebounce.h>
#include <FanController.h>
#include "Hardware.h"
#endif

// The different kinds of alert we can present to the user.
enum Alert { alertNone, alertBatteryLow, alertFanRPM };

// The user can choose any of these speeds
enum FanSpeed { fanLow, fanMedium, fanHigh };

class Main {
public:
    const int DELAY_100ms = 100;

    // How many milliseconds should there be between readings of the fan speed. A smaller value will update
    // more often, while a higher value will give more accurate and smooth readings.
    const int FAN_SPEED_READING_INTERVAL = 1000;

    Main() :
        buttonFanUp(FAN_UP_PIN, DELAY_100ms),
        buttonFanDown(FAN_DOWN_PIN, DELAY_100ms),
        buttonPowerOff(MONITOR_PIN, DELAY_100ms),
        fanController(FAN_RPM_PIN, FAN_SPEED_READING_INTERVAL, FAN_PWM_PIN)
    {
        instance = this;
    }
    void setup();
    void loop();

    void allLEDsOff();
    void allLEDsOn();
    void setLEDs(const int* pinList, int state);
    static void toggleAlert();
    void enterAlertState(Alert alert);
    void setFanSpeed(FanSpeed speed);
    void updateFan();
    unsigned int readBatteryFullness();
    void updateBattery();
    static void onFanDownButtonChange(const int state);
    static void onFanUpButtonChange(const int state);
    static void onMonitorChange(const int state);
    void realOnMonitorChange(const int state);

    ButtonDebounce buttonFanUp;
    ButtonDebounce buttonFanDown;
    ButtonDebounce buttonPowerOff;
    FanController fanController;
    Hardware hw;

    static Main* instance;
};