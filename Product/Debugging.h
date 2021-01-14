/*
 * Debugging.h
 * 
 * Temporary debug code, because we don't yet have a usable serial port or debugger.
 * 
 */
#pragma once
#include "stdint.h"

 // Write a 16-bit number to the LEDs, in hex. It looks like this ...
 //    all LEDs quickly flash, left to right
 //    high-order hex digit, in binary
 //    next hex digit, in binary
 //    next hex digit, in binary
 //    low-order hex digit, in binary
 //    all LEDs quickly flash, right to left
void writeNumberToLights(uint16_t number);
