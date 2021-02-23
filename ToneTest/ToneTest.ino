#undef USE_TIMER1
// speaker is slightly louder with USE_TIMER1, 94dB vs 92dB at ~0.5cm

#ifdef USE_TIMER1
#include <TimerOne.h>
#endif
#include <ButtonDebounce.h>
#include "PAPRHwDefs.h"
#include "Timer.h"
#include "MySerial.h"

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
    writeFrequency(currentFrequency);
}

void changeDutyCycle(float dutyCycle)
{
    currentDutyCycle = dutyCycle;
    #ifdef USE_TIMER1
        Timer1.setPwmDuty(BUZZER_PIN, dutyCycle * 1023);
    #endif
    writeDutyCycle(currentDutyCycle);
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

Timer heartbeatTimer;
bool heartbeatOn = false;

void writeFrequency(unsigned long frequency)
{
    myPrintf("Frequency %ld\r\n", frequency);
    //writeNumberToLights(frequency / 100);
}

void writeDutyCycle(float dutyCycle)
{
    myPrintf("Duty cycle %d\r\n", (int)(dutyCycle * 100));
    //writeNumberToLights((unsigned int)(dutyCycle * 10.0));
}

void writeMode(bool mode)
{
    myPrintf("Now in %s mode\r\n", mode ? "frequency" : "duty cycle");
}

void modeChangeTimeout()
{
    changeFrequency(prevFrequency);
    changeDutyCycle(prevDutyCycle);
    frequencyMode = !frequencyMode;
    writeMode(frequencyMode);
}

void sequenceModeTimeout()
{
    currentSequenceIndex = (currentSequenceIndex + 1) % numberOfSequences;
    myPrintf("Playing sequence #%d\r\n", currentSequenceIndex);
    playSequence(sequences[currentSequenceIndex]);
}

const int frequencyIncrement = 10;
const float dutyCycleIncrement = 0.10;

void onDownButtonChange(const int state)
{
    if (state == BUTTON_PUSHED) {
        prevDutyCycle = currentDutyCycle;
        prevFrequency = currentFrequency;
        modeChangeTimer.startMillis(modeChangeTimeout, 2000);

        if (frequencyMode) {
            changeFrequency(currentFrequency - frequencyIncrement);
        } else {
            changeDutyCycle(currentDutyCycle - dutyCycleIncrement);
        }
    } else { // BUTTON_RELEASED
        modeChangeTimer.cancel();
    }
}

void onUpButtonChange(const int state)
{
    if (state == BUTTON_PUSHED) {
        prevDutyCycle = currentDutyCycle;
        prevFrequency = currentFrequency;
        sequenceModeTimer.startMillis(sequenceModeTimeout, 2000);
        if (frequencyMode) {
            changeFrequency(currentFrequency + frequencyIncrement);
        } else {
            changeDutyCycle(currentDutyCycle + dutyCycleIncrement);
        }
    } else { // BUTTON_RELEASED
        sequenceModeTimer.cancel();
    }
}

void heartbeat()
{
    heartbeatOn = !heartbeatOn;
    digitalWrite(ERROR_LED_PIN, heartbeatOn ? LED_ON : LED_OFF);
    heartbeatTimer.startMillis(heartbeat, 1000);
}

void setup() {
    initSerial();
    pinMode(ERROR_LED_PIN, OUTPUT);
    analogWrite(FAN_PWM_PIN, 0);

    buttonUp.setCallback(onUpButtonChange);
    buttonDown.setCallback(onDownButtonChange);

    #ifdef USE_TIMER1
    Timer1.initialize(1000);
    Timer1.pwm(BUZZER_PIN, 511);
    Timer1.start();
    #endif

    myPrintf("In frequency mode, up/down buttons change the frequency.\r\n");
    myPrintf("In duty cycle mode, up/down buttons change the duty cycle.\r\n");
    myPrintf("Long press down button changes mode.\r\n");
    myPrintf("Long press up button plays the next sequence.\r\n");
    myPrintf("\r\n");

    writeMode(frequencyMode);
    changeFrequency(2110);
    changeDutyCycle(0.50);
    heartbeat();
}

void loop() {
    buttonUp.update();
    buttonDown.update();
    modeChangeTimer.update();
    sequenceModeTimer.update();
    sequenceTimer.update();
    heartbeatTimer.update();
}
