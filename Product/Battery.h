#pragma once

class Battery {
public:
    bool isCharging();
    void update();
    void wakeUp();
    long long getPicoCoulombs() { return picoCoulombs; }
    void notifySystemActive(bool active) { systemActive = active; }
    Battery();

private:
    void updateBatteryVoltage();
    void updateBatteryTimers();
    bool tempIsCharging();

    long long picoCoulombs; // How much charge is in the battery right now.
    long long microVolts;   // The voltage right now.
    unsigned long lastCoulombsUpdateMicroSecs;
    unsigned long chargeStartMilliSecs;       // millisecond timestamp of when the battery charger started up
    unsigned long lastVoltageChangeMilliSecs; // millisecond timestamp of when the battery voltage last changed
    bool prevIsCharging;
    long long prevMicroVolts;
    bool systemActive;
};