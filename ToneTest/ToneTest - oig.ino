#include <TimerOne.h>
#include <ButtonDebounce.h>
#include "PAPRHwDefs.h"
#include "Timer.h"

// unsigned long toneEndMicros;
// unsigned long toneHalfPeriod;
// unsigned long toneOnPeriod;
// unsigned long toneOffPeriod;
// 
// bool toneToggle;
// Timer toneTimer;
//
// void toneTick()
// {
//     toneToggle = !toneToggle;
//     digitalWrite(BUZZER_PIN, toneToggle ? BUZZER_ON : BUZZER_OFF);
//     toneTimer.startMicros(toneTick, toneToggle ? toneOnPeriod : toneOffPeriod);
// }
// 

// A list of all the LEDs, from left to right.
const byte LEDpins[] = {
    BATTERY_LED_LOW_PIN,
    BATTERY_LED_MED_PIN,
    BATTERY_LED_HIGH_PIN,
    ERROR_LED_PIN,
    FAN_LOW_LED_PIN,
    FAN_MED_LED_PIN,
    FAN_HIGH_LED_PIN
};
const int numLEDs = sizeof(LEDpins) / sizeof(byte);

void writeNumberToLights(unsigned int number)
{
    digitalWrite(LEDpins[0], (number & 0x40) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[1], (number & 0x20) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[2], (number & 0x10) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[3], (number & 0x08) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[4], (number & 0x04) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[5], (number & 0x02) ? LED_ON : LED_OFF);
    digitalWrite(LEDpins[6], (number & 0x01) ? LED_ON : LED_OFF);
}

unsigned long currentFrequency;
float currentDutyCycle;

void playTone(unsigned long frequency, float dutyCycle)
{
    currentFrequency = frequency;
    currentDutyCycle = dutyCycle;

    Timer1.setPeriod(1000000.0 / frequency);
    Timer1.setPwmDuty(BUZZER_PIN, dutyCycle * 1023);

    // double tonePeriod = (1.0 / frequency) * 1000000.0;
    // toneOnPeriod = tonePeriod * dutyCycle;
    // toneOffPeriod = tonePeriod - toneOnPeriod;
    // toneToggle = false;
    // toneTimer.startMicros(toneTick, 1);
}

bool ledToggle = false;

ButtonDebounce buttonUp(FAN_UP_PIN, 100);
ButtonDebounce buttonDown(FAN_DOWN_PIN, 100);

const double semitoneUp = 1.0594630943592953;
const double semitoneDown = 1.0 / semitoneUp;

void onDownButtonChange(const int state)
{
    if (state == BUTTON_PUSHED) {
        if (buttonUp.state() == BUTTON_PUSHED) {
            playTone(currentFrequency, currentDutyCycle - 0.10);
            writeNumberToLights((unsigned int)(currentDutyCycle * 10.0));
        } else {
            playTone(currentFrequency - 100, currentDutyCycle);
            writeNumberToLights(currentFrequency / 100);
        }
    } else {
        writeNumberToLights(0);
    }
}

void onUpButtonChange(const int state)
{
    if (state == BUTTON_PUSHED) {
        if (buttonDown.state() == BUTTON_PUSHED) {
            playTone(currentFrequency, currentDutyCycle + 0.10);
            writeNumberToLights((unsigned int)(currentDutyCycle * 10.0));
        } else {
            playTone(currentFrequency + 100, currentDutyCycle);
            writeNumberToLights(currentFrequency / 100);
        }
    } else {
        writeNumberToLights(0);
    }
}

void ledToggler()
{
    digitalWrite(ERROR_LED_PIN, ledToggle ? LED_ON : LED_OFF);
    ledToggle = !ledToggle;
}

void setup() {
    pinMode(ERROR_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    analogWrite(FAN_PWM_PIN, 0);
    for (int i = 0; i < numLEDs; i += 1) {
        pinMode(LEDpins[i], OUTPUT);
        digitalWrite(LEDpins[i], LED_OFF);
    }

    buttonUp.setCallback(onUpButtonChange);
    buttonDown.setCallback(onDownButtonChange);

    const int dutyCycle = 50;
    const int pin = BUZZER_PIN;
    const int frequency = 100;
    Timer1.initialize(1000000.0 / frequency);
    Timer1.pwm(pin, (dutyCycle / 100) * 1023);
    Timer1.start();

    playTone(2400, 0.30);
}

unsigned int loopCount = 0;

void loop() {
    buttonUp.update();
    buttonDown.update();
    //toneTimer.update();
}
// 12.5 seconds for 100000 ==> 125 microseconds for one loop = loop frequency is 8 kHz, max sound freq is therefore 4 kHz