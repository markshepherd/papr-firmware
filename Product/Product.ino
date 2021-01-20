/*
 * Product.ino
 * 
 * This file acts as the glue between the Arduino runtime and the main program in Main.cpp.
 * Please don't add any code to this file. Instead, add code to Main.cpp, so that it can unit tested.
 */
#include "Main.h"
#include "PAPRHwDefs.h"
 
static const unsigned int DELAY_100ms = 100;
static const unsigned int FAN_SPEED_READING_INTERVAL = 1000;

static ButtonDebounce buttonFanUp(FAN_UP_PIN, DELAY_100ms);
static ButtonDebounce buttonFanDown(FAN_DOWN_PIN, DELAY_100ms);
static ButtonDebounce buttonPowerOff(MONITOR_PIN, DELAY_100ms);
static FanController fanController(FAN_RPM_PIN, FAN_SPEED_READING_INTERVAL, FAN_PWM_PIN);
static Hardware hw;
Main paprMain;
    
void setup() {
    paprMain.init(&buttonFanUp, &buttonFanDown, &buttonPowerOff, &fanController, &hw);
    paprMain.setup();
}

void loop() {
    paprMain.loop();
}
