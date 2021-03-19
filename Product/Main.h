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
enum PAPRState { stateOff, stateOn, stateOffCharging, stateOnCharging };

// - High Power Mode means that the PCB and MCU are fully powered 
// - Low Power Mode means that the MCU receives reduced voltage (approx 2.5 instead of 5)
//   and the rest of the PCB receives no power. In this mode, we must run the MCU at a
//   reduced clock speed (1 MHz instead of 8 MHz).
//
// We use low power mode when we're in the Power Off state, in order to conserve power
enum PowerMode { lowPowerMode, fullPowerMode };

class Main {
public:
    Main();

    // Arduino-style main loop
    void setup();
    void loop();

    // The Hardware object gives access to all the microcontroller hardware such as pins and timers. Please always use this object,
    // and never access any hardware or Arduino APIs directly. This gives us the option of using a fake hardware object for unit testing.
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
    void setLEDs(const int* pinList, int onOff);
    void flashAllLEDs(int millis, int count);
    void onToggleAlert();
    void enterAlertState(Alert alert);
    void setFanSpeed(FanSpeed speed);
    void checkForFanAlert();
    void checkForBatteryAlert();
    void onPowerPress();
    void setPowerMode(PowerMode mode);
    void enterState(PAPRState newState);
    void powerButtonInterruptCallback();
    void nap();
    void doAllUpdates();
    void updateBatteryCoulombs();
    void updateBatteryVoltage();
    void updateFanLEDs();
    void updateBatteryLEDs();
    void updateBatteryTimers();
    bool isCharging();
    void cancelAlert();
    bool doPowerOffWarning();

    // Event handler glue code
    static void staticToggleAlert();
    static void staticFanDownPress(const int);
    static void staticFanUpPress(const int);
    static void staticPowerPress(const int);
    static void staticPowerButtonInterruptCallback();

    /********************************************************************
     * Fan data
     ********************************************************************/

     // The current fan speed selected by the user.
    FanSpeed currentFanSpeed;

    // After we change the fan speed, we stop checking the RPMs for a few seconds, to let the speed stabilize.
    unsigned long dontCheckFanSpeedUntil;

    /********************************************************************
     * Battery and power data
     ********************************************************************/

    unsigned long lastBatteryCoulombsUpdateMicros;
    double batteryCoulombs; // How much charge is in the battery right now.
    unsigned long lastBatteryVoltsUpdateMillis;
    unsigned long batteryVoltageAccumulator;
    unsigned long numBatteryVoltageSamples;
    float batteryVolts; // The battery voltage right now.
    PAPRState paprState;
    unsigned long chargeStartTimeMillis; // millisecond timestamp of when the battery charger started up
    unsigned long lastVoltageChangeTimeMillis; // millisecond timestamp of when the battery voltage last changed
    bool prevIsCharging;
    double prevBatteryVolts;

    /********************************************************************
     * Alert data
     ********************************************************************/

     // Data used when we are in the alert state.
    Alert currentAlert;
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
