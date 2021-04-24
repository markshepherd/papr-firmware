#pragma once

class Battery {
public:
    bool isCharging();
    void update();
    void wakeUp();
    double getCoulombs() { return coulombs; }
    void notifySystemActive(bool active) { systemActive = active; }
    Battery();

private:
    void updateBatteryVoltage();
    void updateBatteryTimers();

    double coulombs; // How much charge is in the battery right now.
    float volts; // The battery voltage right now.
    unsigned long lastCoulombsUpdateMicros;
    unsigned long lastVoltsUpdateMillis;
    unsigned long voltageAccumulator;
    unsigned long numVoltageSamples;
    unsigned long chargeStartTimeMillis; // millisecond timestamp of when the battery charger started up
    unsigned long lastChangeTimeMillis; // millisecond timestamp of when the battery voltage last changed
    bool prevIsCharging;
    double prevVolts;
    bool systemActive;
};