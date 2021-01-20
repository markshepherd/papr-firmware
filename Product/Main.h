/*
 * Main.h
 *
 * This defines the main program of the PAPR product firmware. This file is intended to be invoked either by
 * by the Arduino runtime (product environment) or by the unit test runtime (unit test environment).
 */
#pragma once
#ifdef UNITTEST
#include "UnitTest/MyButtonDebounce.h"
#include "UnitTest/MyFanController.h"
#include "UnitTest/MyHardware.h"
#else
#include <ButtonDebounce.h>
#include <FanController.h>
#include "Hardware.h"
#endif

class Main {
public:
    Main() {}
    void init(
        ButtonDebounce* pButtonFanUp,
        ButtonDebounce* pButtonFanDown,
        ButtonDebounce* pPowerOffButton,
        FanController* pFanController,
        Hardware* pHardware
    );
    void setup();
    void loop();
};