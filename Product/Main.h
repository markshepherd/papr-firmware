/*
 * Main.h
 *
 * This defines the main program of the PAPR product firmware. This file is intended to be invoked either by
 * by the Arduino runtime (product environment) or by the unit test runtime (unit test environment).
 */
#pragma once
#include "PAPRHwDefs.h"
#include "Timer.h"
#ifdef UNITTEST
#include "UnitTest/MyButtonDebounce.h"
#include "UnitTest/MyFanController.h"
#include "UnitTest/MyHardware.h"
#else
#include "PressDetector.h"
#include <FanController.h>
#include "Hardware.h"
#endif

class PAPRMainTest;

// The different kinds of alert we can present to the user.
enum Alert { alertNone, alertBatteryLow, alertFanRPM };

// The user can choose any of these speeds
enum FanSpeed { fanLow, fanMedium, fanHigh };

// We can be either on or off, and either charging or not charging.
enum PowerState { powerOff, powerOn, powerOffCharging, powerOnCharging };

// Battery can be either low, normal, or full.
enum BatteryLevel { batteryLow, batteryNormal, batteryFull };

class Main {
public:
    Main();

    // Arduino-style main loop
    void setup();
    void loop();

    // The Hardware object gives access to all the microcontroller hardware such as pins and timers. Please always use this object,
    // and never access any hardware or Arduino APIs directly. This gives us the abiity to use a fake hardware object for unit testing.
    Hardware hw;

    // The PressDetector object polls a pin, and calls a callback when the pin value changes. There is one PressDetector object per button.
    PressDetector buttonFanUp;
    PressDetector buttonFanDown;
    PressDetector buttonPower;

    // The object that controls and monitors the fan.
    FanController fanController;

private:
    // Internal functions
    void allLEDsOff();
    void allLEDsOn();
    void setLEDs(const int* pinList, int state);
    void flashAllLEDs(int millis, int count);
    void realOnToggleAlert();
    void enterAlertState(Alert alert);
    void setFanSpeed(FanSpeed speed);
    void updateFan();
    void updateBattery();
    void realOnPowerPress(const int state);
    void setPowerMode(int mode);
    void enterState(PowerState newState);
    void realPowerButtonInterruptCallback();
    void nap();
    void doUpdates();
    bool stateOfChargeUpdate();
    void updateFanLEDs();

    // Event handlers
    static void onToggleAlert();
    static void onFanDownPress(const int state);
    static void onFanUpPress(const int state);
    static void onPowerPress(const int state);
    static void powerButtonInterruptCallback();

    /********************************************************************
     * Fan data
     ********************************************************************/

     // The current fan speed selected by the user.
    FanSpeed currentFanSpeed;

    // After we change the fan speed, we stop checking the RPMs for a few seconds, to let the speed stabilize.
    unsigned long dontCheckFanSpeedUntil = 0;

    /********************************************************************
     * Battery and power data
     ********************************************************************/

     // We don't check the battery level on every loop(). Rather, we average battery levels over
     // a second or so, to smooth out the minor variations.
    unsigned long nextBatteryCheckMillis = 0;
    unsigned long batteryLevelAccumulator = 0;
    unsigned long numBatteryLevelSamples = 0;

    BatteryLevel currentBatteryLevel = batteryFull;

    PowerState powerState;

    /********************************************************************
     * Alert data
     ********************************************************************/

     // Data used when we are in the alert state.
    Alert currentAlert = alertNone;
    const int* currentAlertLEDs = nullptr;
    const int* currentAlertMillis = nullptr;
    bool alertToggle;

    // The timer that pulses the lights and buzzer during an alert.
    Timer alertTimer;

public:
    // Glue
    unsigned long millis() { return hw.millis(); }
    static Main* instance;
};
