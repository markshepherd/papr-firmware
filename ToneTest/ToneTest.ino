#undef USE_TIMER1
// speaker is slightly louder with USE_TIMER1, 94dB vs 92dB at ~0.5cm

#ifdef USE_TIMER1
#include <TimerOne.h>
#endif
#include <ButtonDebounce.h>
#include "PAPRHwDefs.h"
#include "Timer.h"

struct SequenceItem {
    int frequency;
    int millis;
};

void changeFrequency(unsigned long frequency);
void sequenceDone();

int currentItemIndex;
int currentSequenceIndex = -1;
const SequenceItem* currentSequence;
Timer sequenceTimer;

void sequencePlayer()
{
    currentItemIndex += 1;
    const SequenceItem* item = &currentSequence[currentItemIndex];
    if (item->millis) {
        changeFrequency(item->frequency);
        sequenceTimer.startMillis(sequencePlayer, item->millis);
    } else {
        sequenceDone();
    }
}

void playSequence(const SequenceItem* sequence)
{
    currentItemIndex = -1;
    currentSequence = sequence;
    sequencePlayer();
}

const double semi = 1.0594630943592953; // 12th root of 2
const int A4Freq = 440;

#define DefineOctave(n, freq)                                                                                       \
const int A##n  = freq;                                                                                             \
const int Bb##n = ((float)freq) * semi;                                                                             \
const int B##n  = ((float)freq) * semi * semi;                                                                      \
const int C##n  = ((float)freq / 2.0) * semi * semi * semi;                                                         \
const int Db##n = ((float)freq / 2.0) * semi * semi * semi * semi;                                                  \
const int D##n  = ((float)freq / 2.0) * semi * semi * semi * semi * semi;                                           \
const int Eb##n = ((float)freq / 2.0) * semi * semi * semi * semi * semi * semi;                                    \
const int E##n  = ((float)freq / 2.0) * semi * semi * semi * semi * semi * semi * semi;                             \
const int F##n  = ((float)freq / 2.0) * semi * semi * semi * semi * semi * semi * semi * semi;                      \
const int Gb##n = ((float)freq / 2.0) * semi * semi * semi * semi * semi * semi * semi * semi * semi;               \
const int G##n  = ((float)freq / 2.0) * semi * semi * semi * semi * semi * semi * semi * semi * semi * semi;        \
const int Ab##n = ((float)freq / 2.0) * semi * semi * semi * semi * semi * semi * semi * semi * semi * semi * semi;

const int A1Freq = A4Freq / 8;
const int A2Freq = A4Freq / 4;
const int A3Freq = A4Freq / 2;
const int A5Freq = A4Freq * 2;
const int A6Freq = A4Freq * 4;
const int A7Freq = A4Freq * 8;

DefineOctave(_1, A1Freq)
DefineOctave(_2, A2Freq)
DefineOctave(_3, A3Freq)
DefineOctave(_4, A4Freq)
DefineOctave(_5, A5Freq)
DefineOctave(_6, A6Freq)
DefineOctave(_7, A7Freq)

const SequenceItem sequence1[] = { {E_6, 250}, {C_6, 1750}, {E_6, 250}, {C_6, 1750}, {E_6, 250}, {C_6, 1750}, {E_6, 250}, {C_6, 1750}, {E_6, 250}, {C_6, 1750}, {0, 0} };

const SequenceItem sequence2[] = { {C_6, 500}, {Gb_6, 500}, {C_6, 500}, {Gb_6, 500}, {C_6, 500}, {Gb_6, 500}, {C_6, 500}, {Gb_6, 500}, {C_6, 500}, {Gb_6, 500}, {0, 0} };

const SequenceItem sequence3[] = {
    {E_5, 450}, {0, 50}, {E_5, 500}, {F_5, 500}, {G_5, 450}, {0, 50}, { G_5, 500 }, {F_5, 500}, {E_5, 500}, {D_5, 500}, {C_5, 450}, {0, 50},
    { C_5, 450}, {0, 50}, { D_5, 500 }, { E_5, 500 }, { D_5, 750 }, {C_5, 200}, {0, 50}, { C_5, 1000 }, {0, 0} };

const SequenceItem* sequences[] = { sequence1, sequence2, sequence3 };
const int numberOfSequences = 3;

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

void setLEDs(int onOff)
{
    for (int i = 0; i < numLEDs; i += 1) {
        digitalWrite(LEDpins[i], onOff);
    }
}

void writeNumberToLights(unsigned int number)
{
    for (int i = 6; i >= 0; i -= 1) {
        digitalWrite(LEDpins[i], (number & 1) ? LED_ON : LED_OFF);
        number = number >> 1;
    }
}

unsigned long currentFrequency;
float currentDutyCycle;
unsigned long prevFrequency;
float prevDutyCycle;

void changeFrequency(unsigned long frequency)
{
    currentFrequency = frequency;
    #ifdef USE_TIMER1
        Timer1.setPeriod(1000000.0 / frequency);
    #else
        tone(BUZZER_PIN, frequency);
    #endif
}

void changeDutyCycle(float dutyCycle)
{
    currentDutyCycle = dutyCycle;
    #ifdef USE_TIMER1
        Timer1.setPwmDuty(BUZZER_PIN, dutyCycle * 1023);
    #endif
}

void sequenceDone()
{
    changeFrequency(prevFrequency);
    changeDutyCycle(prevDutyCycle);
}

ButtonDebounce buttonUp(FAN_UP_PIN, 100);
ButtonDebounce buttonDown(FAN_DOWN_PIN, 100);

bool frequencyMode = true;
Timer modeChangeTimer;
Timer sequenceModeTimer;

void writeFrequencyToLights(unsigned long frequency)
{
    writeNumberToLights(frequency / 100);
}

void writeDutyCycleToLights(float dutyCycle)
{
    writeNumberToLights((unsigned int)(dutyCycle * 10.0));
}

void modeChangeTimeout()
{
    changeFrequency(prevFrequency);
    changeDutyCycle(prevDutyCycle);
    if (frequencyMode) {
        writeFrequencyToLights(currentFrequency);
    } else {
        writeDutyCycleToLights(currentDutyCycle);
    }
    frequencyMode = !frequencyMode;
    delay(500);
    setLEDs(LED_ON);
    delay(500);
    setLEDs(LED_OFF);
}

void sequenceModeTimeout()
{
    currentSequenceIndex = (currentSequenceIndex + 1) % numberOfSequences;
    playSequence(sequences[currentSequenceIndex]);
}

void onDownButtonChange(const int state)
{
    if (state == BUTTON_PUSHED) {
        prevDutyCycle = currentDutyCycle;
        prevFrequency = currentFrequency;
        modeChangeTimer.startMillis(modeChangeTimeout, 2000);

        if (frequencyMode) {
            changeFrequency(currentFrequency - 100);
            writeFrequencyToLights(currentFrequency);
        } else {
            changeDutyCycle(currentDutyCycle - 0.10);
            writeDutyCycleToLights(currentDutyCycle);
        }
    } else { // BUTTON_RELEASED
        modeChangeTimer.cancel();
        setLEDs(LED_OFF);
    }
}

void onUpButtonChange(const int state)
{
    if (state == BUTTON_PUSHED) {
        prevDutyCycle = currentDutyCycle;
        prevFrequency = currentFrequency;
        sequenceModeTimer.startMillis(sequenceModeTimeout, 2000);
        if (frequencyMode) {
            changeFrequency(currentFrequency + 100);
            writeFrequencyToLights(currentFrequency);
        } else {
            changeDutyCycle(currentDutyCycle + 0.10);
            writeDutyCycleToLights(currentDutyCycle);
        }
    } else { // BUTTON_RELEASED
        sequenceModeTimer.cancel();
        setLEDs(LED_OFF);
    }
}

void setup() {
    pinMode(ERROR_LED_PIN, OUTPUT);
    analogWrite(FAN_PWM_PIN, 0);
    for (int i = 0; i < numLEDs; i += 1) {
        pinMode(LEDpins[i], OUTPUT);
        digitalWrite(LEDpins[i], LED_OFF);
    }

    buttonUp.setCallback(onUpButtonChange);
    buttonDown.setCallback(onDownButtonChange);

    #ifdef USE_TIMER1
    Timer1.initialize(1000);
    Timer1.pwm(BUZZER_PIN, 511);
    Timer1.start();
    #endif

    changeFrequency(2110);
    changeDutyCycle(0.50);
}

void loop() {
    buttonUp.update();
    buttonDown.update();
    modeChangeTimer.update();
    sequenceModeTimer.update();
    sequenceTimer.update();
}
