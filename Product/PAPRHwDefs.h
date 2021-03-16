/*
* PAPRHwDefs.h
* 
* This header file defines all the input/output pins on the v2 PAPR board.
*/
#pragma once
#ifdef UNITTEST
const int A0 = 20;
const int A1 = 21;
const int A2 = 22;
const int A3 = 23;
const int A4 = 24;
const int A5 = 25;
const int A6 = 26;
const int A7 = 27;
const int LOW = 0;
const int HIGH = 1;
#else
#include "Arduino.h"
#endif

//================================================================
// OUTPUT PINS
const int FAN_PWM_PIN            =  5;  /* PD5 Analog:  0-255 duty cycle     */
const int BATTERY_LED_LOW_PIN    = A0;  /* PC0 Digital: LOW = on, HIGH = off */
const int BATTERY_LED_MED_PIN    = A1;  /* PC1 Digital: LOW = on, HIGH = off */
const int BATTERY_LED_HIGH_PIN   = A2;  /* PC2 Digital: LOW = on, HIGH = off */
const int ERROR_LED_PIN          = A3;  /* PC3 Digital: LOW = on, HIGH = off */
const int FAN_LOW_LED_PIN        = A4;  /* PC4 Digital: LOW = on, HIGH = off */
const int FAN_MED_LED_PIN        = A5;  /* PC5 Digital: LOW = on, HIGH = off */
const int FAN_HIGH_LED_PIN       =  0;  /* PD0 Digital: LOW = on, HIGH = off */
const int BUZZER_PIN             = 10;  /* PB2 Analog:  0-255 duty cycle     */
const int VIBRATOR_PIN           =  2;  /* PD2 Digital: LOW = off, HIGH = on */
const int BOARD_POWER_PIN        =  8; // unused pin PB0

const int LED_ON = LOW;
const int LED_OFF = HIGH;

const int BUZZER_ON = 64; // I tried different numbers and this one sounded the best :)
const int BUZZER_OFF = 0;

//================================================================
// INPUT PINS
const int BATTERY_VOLTAGE_PIN    = A7;  /* PC7 Analog:  values range from roughly 0x100 (6 volts) to 0x300 (24 volts) */
const int MONITOR_PIN            =  7;  /* PD7 Digital: LOW = power down button pushed */
const int FAN_RPM_PIN            =  3;  /* PD3 Digital: square wave, frequency proportional to RPM */
const int FAN_UP_PIN             =  1;  /* PD1 Digital: LOW = pushed, HIGH = released */
const int FAN_DOWN_PIN           =  9;  /* PB1 Digital: LOW = pushed, HIGH = released */
const int POWER_PIN              = MONITOR_PIN;
const int POWER_ON_PIN           = MONITOR_PIN;
const int POWER_OFF_PIN          = MONITOR_PIN;

const int BUTTON_PUSHED = LOW;
const int BUTTON_RELEASED = HIGH;
