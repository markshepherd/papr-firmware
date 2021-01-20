/*
 * Product.ino
 * 
 * This file acts as the glue between the Arduino runtime and the main program in Main.cpp.
 * Please don't add any code to this file. Instead, add code to Main.cpp, so that it can unit tested.
 */
#include "Main.h"
#include "PAPRHwDefs.h"

Main paprMain;
    
void setup() {
    paprMain.setup();
}

void loop() {
    paprMain.loop();
}
