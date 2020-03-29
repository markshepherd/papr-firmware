// OUTPUT PINS
#define FAN_PWM_PIN                 3
#define FAN_TACHOMETER_PIN          4
#define BUZZER_PIN                  13
#define ALERT_LED_PIN               12

// INPUT PINS
#define POTENTIOMETER_PIN           A1
#define BATTERY_VOLTAGE_PIN         A0

// CONSTANTS
#define FAN_TACHOMETER_PULSES_PER_ROTATION  2

#define BATTERY_VOLTAGE_MULTIPLIER  6 * 5
#define BATTERY_VOLTAGE_THRESHOLD   15 * 1024

#define BATTERY_VOLTAGE_75_PERCENT 17
#define BATTERY_VOLTAGE_50_PERCENT 17
#define BATTERY_VOLTAGE_25_PERCENT 17
#define BATTERY_VOLTAGE_ALARM 17

// CONTROLLER PLATFORM
//
// Different controller platforms have different methods to drive 25kHz PWM.
#define ATMEGA                      1
#define ATTINY                      2

#if defined(__AVR_ATmega328P__)
// ATmega
#define CONTROLLER_PLATFORM         ATMEGA
#define FAN_TACHOMETER_INTERRUPT    digitalPinToInterrupt(FAN_TACHOMETER_PIN)

#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)

//ATtiny
#define CONTROLLER_PLATFORM         ATTINY
#define FAN_TACHOMETER_INTERRUPT    FAN_TACHOMETER_PIN

#endif

#include <util/atomic.h>

uint16_t s_tachometer = 0;
uint32_t s_tachometer_start = 0;

void setup() {
  
  // Setup input pins.
  pinMode(POTENTIOMETER_PIN, INPUT);
  pinMode(BATTERY_VOLTAGE_PIN, INPUT);

  // Setup output pins.
  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  s_tachometer_start = millis();
  attachInterrupt(FAN_TACHOMETER_INTERRUPT, interruptTachometer, RISING);

  // Setup PWM clock.
  beginPwm25kHz();
}

void loop() {
  // put your main code here, to run repeatedly:
  uint32_t current_time = millis();

  uint16_t battery_voltage = analogRead(BATTERY_VOLTAGE_PIN) * BATTERY_VOLTAGE_MULTIPLIER;
  uint16_t potentiometer = analogRead(POTENTIOMETER_PIN);

  setPwmDuty(min(potentiometer / 4, 0xff));

  uint16_t battery_alert = (battery_voltage < BATTERY_VOLTAGE_THRESHOLD) ? HIGH : LOW;
  digitalWrite(BUZZER_PIN, battery_alert);
  digitalWrite(ALERT_LED_PIN, battery_alert);

  if (current_time - s_tachometer_start > 100) {
    uint16_t tachometer_rpm;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      tachometer_rpm = s_tachometer * 10 / FAN_TACHOMETER_PULSES_PER_ROTATION;
      s_tachometer = 0;
      s_tachometer_start = current_time;
    }

    // Do something with the tachometer
  }

  delay(10);
}


// https://create.arduino.cc/projecthub/MyName1sSimon/control-pwm-fans-with-an-arduino-7bef86
// http://www.nomad.ee/micros/tiny_pwm/

/**
 * Setup the PWM for use with a 25kHz PWM fan.
 */
void beginPwm25kHz() {

#if CONTROLLER_PLATFORM == ATMEGA
  // Pin 3
  TCCR2A = 0;                               // TC2 Control Register A
  TCCR2B = 0;                               // TC2 Control Register B
  TIMSK2 = 0;                               // TC2 Interrupt Mask Register
  TIFR2 = 0;                                // TC2 Interrupt Flag Register

  // OC2B cleared/set on match when up/down counting, fast PWM
  TCCR2A |= (1 << COM2B1)
          | (1 << WGM21)
          | (1 << WGM20);  
  TCCR2B |= (1 << WGM22)
          | (1 << CS21);     // prescaler 8
  
  OCR2A = 79;                               // TOP overflow value (Hz)
  OCR2B = 0;
  
#elif CONTROLLER_PLATFORM == ATTINY

  #define PERIOD_uS_16MHz 2000

  // See http://www.technoblogy.com/show?LE0
  
  // Set timer 1 to interrupt at 1kHz [1000 us]
  TCCR1A = 0;                               // TC1 Control Register A
  TCCR1B = 0;                               // TC1 Control Register B
  TCNT1  = 0;                               // TC1 Counter Value

  // OCR1A = (16*10^6) / (1000[Hz]*8[/tick]) - 1 (must be <65536)
  OCR1A = 639;      // 16M / 25K - 1
  OCR1B = 0;
  
  TCCR1B |= (1 << WGM12); // turn on CTC mode (reset on compare)
  // TCCR1B |= (1 << CS11); // Set CS11 to slow 8x
  TCCR1B |= (1 << CS10); // Set CS10 to enable timer
  
  TIMSK1 |= (1 << OCIE1A); // enable rising edge timer compare interrupt
  TIMSK1 |= (1 << OCIE1B); // enable falling edge timer compare interrupt
#else
  #error beginPwm25kHz: CONTROLLER_PLATFORM invalid or absent.
#endif
  
}

/** 
 *  Sets the duty cycle of the PWM pin
 *  @param duty The target duty cycle from 0 to 255
 */
void setPwmDuty(byte duty) {
  
#if CONTROLLER_PLATFORM == ATMEGA
  OCR2B = duty;                             // PWM Width (duty)
#elif CONTROLLER_PLATFORM == ATTINY
  OCR1B = duty;
#else
  #error setPwmDuty: CONTROLLER_PLATFORM invalid or absent.
#endif
  
}

void interruptTachometer() {
  s_tachometer += 1;
}
