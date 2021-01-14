/*
 * Main.h
 *
 * This defines the main program of the PAPR product firmware. This file is intended to be invoked either by
 * by the Arduino runtime (product environment) or by the unit test runtime (unit test environment).
 */
#pragma once

class Main {
public:
    Main() {}
    void setup();
    void loop();
};