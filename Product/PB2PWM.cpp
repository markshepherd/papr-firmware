#include "PB2PWM.h"
#include "Arduino.h"

void startPB2PWM(long frequencyHz, int dutyCyclePercent)
{
    const int pinNumber = 10; // 10 = BUZZER_PIN = PB2 = OC1B
    const int prescale = 1;
    pinMode(pinNumber, OUTPUT);
    TCCR1A = _BV(COM1B1) // Clear OC1B on Compare Match when up-counting, set OC1B on Compare Match when down-counting.
            | _BV(WGM10)  // PWM, Phase Correct, OCR1A, TOP, BOTTOM 
            | _BV(WGM11);
    TCCR1B = _BV(WGM13)
            | _BV(CS10); // no prescale
    OCR1A = F_CPU / prescale / (2 * frequencyHz);
    OCR1B = dutyCyclePercent * OCR1A / 100;

    // This code is roughly based on http://www.righto.com/2009/07/secrets-of-arduino-pwm.html
}

void stopPB2PWM() {
    TCCR1A = 0;
    TCCR1B = 0;
    OCR1A = 0;
    OCR1B = 0;
}

