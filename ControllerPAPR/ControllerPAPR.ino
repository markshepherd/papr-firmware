//================================================================
// OUTPUT PINS
#define FAN_PWM_PIN                 3
#define FAN_TACHOMETER_PIN          4
#define BUZZER_PIN                  13
#define ALERT_LED_PIN               12

//================================================================
// INPUT PINS
#define POTENTIOMETER_PIN           A1
#define BATTERY_VOLTAGE_PIN         A0

//================================================================
// CONSTANTS
#define FAN_TACHOMETER_PULSES_PER_ROTATION    2

// Voltage divider as a ratio
#define BATTERY_VOLTAGE_DIVIDER_NUMERATOR     1       // This will be the pull down resistor value
#define BATTERY_VOLTAGE_DIVIDER_DENOMINATOR   6       // This is the cumulative resitance of both the pull up and pull down

#define BATTERY_VOLTAGE_DENOMINATOR            10     // Divide all voltages by this number
#define BATTERY_VOLTAGE_100_PERCENT           370     // 27.0V (actual rated max 37.6V for 8S)
#define BATTERY_VOLTAGE_75_PERCENT            320     // 32.0V
#define BATTERY_VOLTAGE_50_PERCENT            280     // 28.0V
#define BATTERY_VOLTAGE_25_PERCENT            240     // 24.0V
#define BATTERY_VOLTAGE_CRITICAL              200     // 20.0V (actual rated min 19.2V for 8S)

//================================================================
// CONTROLLER PLATFORM
//
// Different controller platforms have different methods to drive 25kHz PWM.
#define ATMEGA                                1
#define ATTINY                                2

#if defined(__AVR_ATmega328P__)
  // ATmega
  #define CONTROLLER_PLATFORM                 ATMEGA
  #define FAN_TACHOMETER_INTERRUPT            digitalPinToInterrupt(FAN_TACHOMETER_PIN)
#elif defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  //ATtiny
  #define CONTROLLER_PLATFORM                 ATTINY
  #define FAN_TACHOMETER_INTERRUPT            FAN_TACHOMETER_PIN
#endif

#define BATTERY_LEVEL_CRITICAL  0
#define BATTERY_LEVEL_LOW       1
#define BATTERY_LEVEL_MEDIUM    2
#define BATTERY_LEVEL_HIGH      3
#define BATTERY_LEVEL_FULL      4

//================================================================
// PROGRAM

#include <util/atomic.h>

uint16_t s_tachometer = 0;          // Tachometer pulse counter.
uint32_t s_tachometer_start = 0;    // Mills when the pulse counter started.

uint8_t s_battery_level = 0;
bool s_error = false;

/**
 * Program setup.
 */
void setup() {
  
  // Setup input pins.
  pinMode(POTENTIOMETER_PIN, INPUT);
  pinMode(BATTERY_VOLTAGE_PIN, INPUT);

  // Setup output pins.
  pinMode(FAN_PWM_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  s_tachometer_start = millis();
  attachInterrupt(FAN_TACHOMETER_INTERRUPT, tachometer_interrupt, RISING);

  // Setup PWM clock.
  pwm_begin_25kHz();
}

/**
 * Program loop.
 */
void loop() {

  // Read inputs
  uint32_t current_time = millis();
  uint16_t battery_voltage = analogRead(BATTERY_VOLTAGE_PIN) * BATTERY_VOLTAGE_DIVIDER_DENOMINATOR / BATTERY_VOLTAGE_DIVIDER_NUMERATOR;
  uint16_t potentiometer = analogRead(POTENTIOMETER_PIN);

  // Set the fan speed based on the potentiometer.
  pwn_set_duty(min(potentiometer / 4, 0xff));

  // Loop subfunctions.
  loop_battery_voltage(battery_voltage);
  loop_fan_tachometer(current_time);

  delay(10);
}

/**
 * Program loop: process tachometer.
 */
void loop_fan_tachometer(uint32_t current_time) {
  // Calculate tachometer speed.
  if (current_time - s_tachometer_start > 100) {
    uint16_t tachometer_rpm;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      tachometer_rpm = s_tachometer * 10 / FAN_TACHOMETER_PULSES_PER_ROTATION;
      s_tachometer = 0;
      s_tachometer_start = current_time;
    }

    // Do something with the tachometer
  }
}

/**
 * Program loop: process battery voltage.
 */
void loop_battery_voltage(uint16_t battery_voltage) {
  // Calculate battery indicator levels.

  // Lower the battery levels at the designated levels.
  switch (s_battery_level) {
    case BATTERY_LEVEL_FULL:
      if (battery_voltage < BATTERY_VOLTAGE_75_PERCENT * 1024 / BATTERY_VOLTAGE_DENOMINATOR) {
        s_battery_level = BATTERY_LEVEL_HIGH;
      }
      // Fall through.
    case BATTERY_LEVEL_HIGH:
      if (battery_voltage < BATTERY_VOLTAGE_50_PERCENT * 1024 / BATTERY_VOLTAGE_DENOMINATOR) {
        s_battery_level = BATTERY_LEVEL_MEDIUM;
      }
      // Fall through.
    case BATTERY_LEVEL_MEDIUM:
      if (battery_voltage < BATTERY_VOLTAGE_25_PERCENT * 1024 / BATTERY_VOLTAGE_DENOMINATOR) {
        s_battery_level = BATTERY_LEVEL_LOW;
      }
      // Fall through.
    case BATTERY_LEVEL_LOW:
      if (battery_voltage < BATTERY_VOLTAGE_CRITICAL * 1024 / BATTERY_VOLTAGE_DENOMINATOR) {
        s_battery_level = BATTERY_LEVEL_CRITICAL;
      }
      break;
    case BATTERY_LEVEL_CRITICAL:
      break;
    default:
      s_error = true;
      break;
  }

  // Raise the battery levels when above the midway mark to the next level.
  switch (s_battery_level) {
    case BATTERY_LEVEL_CRITICAL:
      if (battery_voltage > (BATTERY_VOLTAGE_CRITICAL + BATTERY_VOLTAGE_25_PERCENT) * 512 / BATTERY_VOLTAGE_DENOMINATOR) {
        s_battery_level = BATTERY_LEVEL_LOW;
      }
      // Fall through.
    case BATTERY_LEVEL_LOW:
      if (battery_voltage > (BATTERY_VOLTAGE_25_PERCENT + BATTERY_VOLTAGE_50_PERCENT) * 512 / BATTERY_VOLTAGE_DENOMINATOR) {
        s_battery_level = BATTERY_LEVEL_MEDIUM;
      }
      // Fall through.
    case BATTERY_LEVEL_MEDIUM:
      if (battery_voltage > (BATTERY_VOLTAGE_50_PERCENT + BATTERY_VOLTAGE_75_PERCENT) * 512 / BATTERY_VOLTAGE_DENOMINATOR) {
        s_battery_level = BATTERY_LEVEL_HIGH;
      }
      // Fall through.
    case BATTERY_LEVEL_HIGH:
      if (battery_voltage > (BATTERY_VOLTAGE_75_PERCENT + BATTERY_VOLTAGE_100_PERCENT) * 512 / BATTERY_VOLTAGE_DENOMINATOR) {
        s_battery_level = BATTERY_LEVEL_FULL;
      }
      break;
    case BATTERY_LEVEL_FULL:
      break;
    default:
      s_error = true;
      break;
  }

  // Alert if battery is critical.
  uint16_t battery_alert = (s_battery_level == BATTERY_LEVEL_CRITICAL) ? HIGH : LOW;
  digitalWrite(BUZZER_PIN, battery_alert);
  digitalWrite(ALERT_LED_PIN, battery_alert);
}

// https://create.arduino.cc/projecthub/MyName1sSimon/control-pwm-fans-with-an-arduino-7bef86
// http://www.nomad.ee/micros/tiny_pwm/

/**
 * Setup the PWM for use with a 25kHz PWM fan.
 */
void pwm_begin_25kHz() {

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
void pwn_set_duty(byte duty) {
  
  #if CONTROLLER_PLATFORM == ATMEGA
    OCR2B = duty;                             // PWM Width (duty)
  #elif CONTROLLER_PLATFORM == ATTINY
    OCR1B = duty;
  #else
    #error setPwmDuty: CONTROLLER_PLATFORM invalid or absent.
  #endif
  
}

/**
 * Interrupt when the tachometer pin detects a rising edge, indicating the start of a pulse.
 */
void tachometer_interrupt() {
  s_tachometer += 1;
}
