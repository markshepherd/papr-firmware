#include "PWM.h"
#include "Hardware.h"
#include "MySerial.h"
#include "PressDetector.h"
#include "PeriodicCallback.h"

long frequencyHz = 1500;
enum Mode { modePWM, /*modeAnalogWrite, modeManual,*/ modeTone };
Mode mode = modePWM;
const int defaultDutyCycle = 50;
bool playing = false;
const int SOUND_PIN = BUZZER_PIN; // FAN_PWM_PIN

// heartbeat
bool heartbeatLedOn = false;
unsigned long lastHeartbeatToggleMillis = 0;

// manual tone
//bool manualToneActive = false;
//unsigned long lastManualToneToggleMicros;
//unsigned long manualToneIntervalMicros;
//bool manualToneToggle;
//int manualTonePin;
//
//void manualToneUpdate() {
//    if (!manualToneActive) return;
//
//    unsigned long nowMicros = Hardware::instance.micros();
//    if (nowMicros - lastManualToneToggleMicros >= manualToneIntervalMicros) {
//        lastManualToneToggleMicros = nowMicros;
//        manualToneToggle = !manualToneToggle;
//        Hardware::instance.digitalWrite(manualTonePin, manualToneToggle ? LOW : HIGH);
//    }
//}
//
//void startManualTone(int pinNumber, long frequencyHz) {
//    manualTonePin = pinNumber;
//    Hardware::instance.pinMode(manualTonePin, OUTPUT);
//    manualToneActive = true;
//    manualToneToggle = false;
//    lastManualToneToggleMicros = Hardware::instance.micros();
//    manualToneIntervalMicros = 500000L / frequencyHz;
//}
//
//void stopManualTone() {
//    manualToneActive = false;
//}

char* modeName() {
    switch (mode) {
    case modePWM: return "PWM";
    //case modeAnalogWrite: return "analogWrite";
    //case modeManual: return "manual";
    case modeTone: return "tone";
    default: return "??";
    }
}

void startSound(long frequencyHz, int dutyCyclePercent) {
    switch (mode) {
    case modePWM:
        startPWM(frequencyHz, dutyCyclePercent);
        break;
    //case modeAnalogWrite:
    //    Hardware::instance.analogWrite(SOUND_PIN, dutyCyclePercent * 255 / 100);
    //    break;
    //case modeManual:
    //    startManualTone(SOUND_PIN, frequencyHz);
    //    break;
    case modeTone:
        tone(SOUND_PIN, frequencyHz);
        break;
    }

    playing = true;
    Hardware::instance.digitalWrite(BATTERY_LED_LOW_PIN, LED_ON);
    serialPrintf("Start Sound: Pin %d   Mode %s   Freq %ld   Duty %d OCR1A %x   OCR1B %x   TCCR1A %x   TCCR1B %x", SOUND_PIN, modeName(), frequencyHz, dutyCyclePercent, OCR1A, OCR1B, TCCR1A, TCCR1B);
}

void stopSound() {
    switch (mode) {
    case modePWM:
        stopPWM();
        break;
    //case modeAnalogWrite:
    //    Hardware::instance.analogWrite(SOUND_PIN, 0);
    //    break;
    //case modeManual:
    //    stopManualTone();
    //    break;
    case modeTone:
        noTone(SOUND_PIN);
        break;
    }

    playing = false;
    digitalWrite(BATTERY_LED_LOW_PIN, LED_OFF);
    serialPrintf("Stop");
}

void onDownButton() {
    if (playing) stopSound();
    if (Hardware::instance.digitalRead(POWER_ON_PIN) == BUTTON_PUSHED) {
        // change mode
        if (mode != modePWM) {
            mode = (Mode)(mode - 1);
        }
    } else {
        // change frequency
        if (frequencyHz > 300) frequencyHz -= 100;
    }
    startSound(frequencyHz, defaultDutyCycle);
}

void onUpButton() {
    if (playing) stopSound();
    if (Hardware::instance.digitalRead(POWER_ON_PIN) == BUTTON_PUSHED) {
        // change mode
        if (mode != modeTone) {
            mode = (Mode)(mode + 1);
        }
    }
    else {
        // change frequency
        if (frequencyHz < 3000) frequencyHz += 100;
    }
    startSound(frequencyHz, defaultDutyCycle);
}

void onPlayButton() {
    if (playing) {
        stopSound();
    } else {
        startSound(frequencyHz, defaultDutyCycle);
    }
}

PressDetector upButton(FAN_UP_PIN, 50, onUpButton/*, stopSound*/);

PressDetector downButton(FAN_DOWN_PIN, 50, onDownButton/*, stopSound*/);

PressDetector playButton(POWER_OFF_PIN, 50, onPlayButton);

PeriodicCallback heartBeat(500, []() {
    lastHeartbeatToggleMillis = Hardware::instance.millis();
    heartbeatLedOn = !heartbeatLedOn;
    Hardware::instance.digitalWrite(CHARGING_LED_PIN, heartbeatLedOn ? LED_ON : LED_OFF); 
});

void setup() {
    Hardware::instance.setup();
    Hardware::instance.analogWrite(FAN_PWM_PIN, 128);
    Hardware::instance.digitalWrite(FAN_ENABLE_PIN, FAN_OFF);
    serialInit();
    serialPrintf("Sound Test");
    serialPrintf("Fan  Down button: decrease the frequency");
    serialPrintf("Fan  Up   button: increase the frequency");
    serialPrintf("Fan  Down button while Power On button pushed: decrease the mode");
    serialPrintf("Fan  Up   button while Power On button pushed: increase the mode");
    serialPrintf("Power Off button: start/stop playing");
}

void loop() {
    upButton.update();
    downButton.update();
    playButton.update();
    //manualToneUpdate();
    heartBeat.update();
}
