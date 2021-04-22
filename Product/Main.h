/*
 * Main.h
 *
 * This defines the main program of the PAPR product firmware. This file is intended to be invoked either by
 * by the Arduino runtime (product environment) or by the unit test runtime (unit test environment).
 */
#pragma once
#include "Timer.h"
#ifdef UNITTEST
#include "UnitTest/MyButtonDebounce.h"
#include "UnitTest/MyFanController.h"
#include "UnitTest/MyHardware.h"
#else
#include "PressDetector.h"
#include "FC.h"
#include "Hardware.h"
#endif

class PAPRMainTest;

// The different kinds of alert we can present to the user.
enum Alert { alertNone, alertBatteryLow, alertFanRPM };

// The user can choose any of these speeds
enum FanSpeed { fanLow, fanMedium, fanHigh };

// We can be either on or off, and either charging or not charging.
enum PAPRState { stateOff, stateOn, stateOffCharging, stateOnCharging };

class Main : public InterruptCallback {
public:
    Main();

    // Arduino-style main loop
    void setup();
    void loop();

    // The Hardware object gives access to all the microcontroller hardware such as pins and timers. Please always use this object,
    // and never access any hardware or Arduino APIs directly. This gives us the option of using a fake hardware object for unit testing.
    #define hw Hardware::instance

    // The PressDetector object polls a pin, and calls a callback when the pin value changes. There is one PressDetector object per button.
    PressDetector buttonFanUp;
    PressDetector buttonFanDown;
    PressDetector buttonPowerOff;
    PressDetector buttonPowerOn;

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
    void onPowerOffPress();
    void onPowerOnPress();
    void enterState(PAPRState newState);
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
    void initBatteryData();

    // Event handler glue code
    static void staticToggleAlert();
    static void staticFanDownPress(const int);
    static void staticFanUpPress(const int);
    static void staticPowerOffPress(const int);
    static void staticPowerOnPress(const int);

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

    double batteryCoulombs; // How much charge is in the battery right now.
    float batteryVolts; // The battery voltage right now.
    PAPRState paprState;

    unsigned long lastBatteryCoulombsUpdateMicros;
    unsigned long lastBatteryVoltsUpdateMillis;
    unsigned long batteryVoltageAccumulator;
    unsigned long numBatteryVoltageSamples;
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
    virtual void callback();
};
